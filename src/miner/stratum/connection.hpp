#pragma once
#include "asio.hpp"
#include "asio/read_until.hpp"
#include "asio/write.hpp"
#include "connection_data.hpp"
#include "general/hex.hpp"
#include "spdlog/spdlog.h"
#include "messages.hpp"
#include <iostream>

namespace stratum {
    class ConnectionServer;

class Connection {
    void close_connection()
    {
        socket->close();
        closed = true;
    }
    void handle_msg(stratum::Result&& r);
    void handle_msg(stratum::Mining_Notify&&);
    void handle_msg(stratum::Mining_SetDifficulty&& d);
    void async_read_line();

    void write_line_private(std::unique_ptr<std::string>&& line)
    {
        *line += "\n";
        asio::const_buffer b(line->data(), line->size());
        asio::async_write(*socket, std::move(b),
            [&, l = std::move(line)](const auto& error, std::size_t /*bytes_transferred*/) {
                if (error) {
                    spdlog::warn("Write error: {}", error.message());
                    close_connection();
                }
            });
    }

    void process_lines()
    {
        std::vector<std::unique_ptr<std::string>> tmp;
        {
            std::lock_guard l(m);
            tmp = std::move(lines);
        }
        for (auto& l : tmp) {
            write_line_private(std::move(l));
        }
    }
    void stratum_subscribe(std::string miner)
    {
        subscribe_id = id_counter++;
        write_line(stratum::subscribe(subscribe_id, miner));
    }
    void stratum_authorize(std::string user, std::string pass)
    {
        write_line(stratum::authorize(id_counter++, user, pass));
    }

public:
    void submit(const stratum::Submission&, size_t connectionId);
    void write_line(std::string line)
    {
        std::cout << "Sent: " << line << std::endl;
        std::lock_guard l(m);
        lines.emplace_back(make_unique<std::string>(std::move(line)));
        context.post([&] { process_lines(); });
    }

    Connection(ConnectionServer& cs);
    ~Connection();
    void run();

private:
    static std::atomic<size_t> idCounter;
    const size_t id;
    ConnectionServer& server;
    const ConnectionData connectionData;
    asio::io_context context;
    std::optional<asio::ip::tcp::socket> socket;
    asio::streambuf response;
    uint32_t id_counter { 0 };
    uint32_t subscribe_id;
    bool closed { false };
    std::mutex m;
    std::vector<std::unique_ptr<std::string>> lines;
};
}
