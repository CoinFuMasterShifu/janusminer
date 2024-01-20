#include "connection.hpp"
#include "connection_server.hpp"

std::atomic<size_t> stratum::Connection::idCounter = { 0 };

using asio::ip::tcp;
namespace {
bool isprint(const std::string& s)
{
    for (auto c : s) {
        if (!std::isprint(c))
            return false;
    }
    return true;
}
}

namespace stratum {
Connection::Connection(ConnectionServer& cs)
    : id { idCounter++ }
    , server(cs)
    , connectionData(cs.connectionData)
{
    std::lock_guard l(server.m);
    server.connection = this;
    server.handle_event(stratum::ConnectionStart { id });
}

Connection::~Connection()
{
    std::lock_guard l(server.m);
    server.connection = nullptr;
    server.handle_event(stratum::ConnectionEnd { id });
}

void Connection::handle_msg(stratum::Result&& r)
{

    if (r.is_error()) {
        auto error = r.get_error();
        spdlog::error("Stratum error code {}: {}", error.code, error.message);
    } else {
        if (uint32_t(r.id == subscribe_id)) { // this is subscribe reply
            try {
                auto& args { r.get_result_args() };
                server.handle_event({ stratum::Subscription {
                    .extranonce1 = hex_to_vec(args.at(1).get<std::string>()),
                    .extranonce2size = args.at(2).get<uint32_t>() } });
            } catch (...) {
            }
        } else {
            spdlog::info("Stratum OK id: {}", r.id);
        }
    }
}
void Connection::handle_msg(stratum::Mining_Notify&& mn)
{
    spdlog::info("Mining notify");
    server.handle_event(mn);
}
void Connection::handle_msg(stratum::Mining_SetDifficulty&& d)
{
    spdlog::info("Set difficulty {}", d.difficulty);
    server.handle_event(d);
}

void Connection::submit(const stratum::Submission& submission, size_t connectionId)
{
    if (connectionId != id)
        return;
    write_line(stratum::submit(id_counter++, submission));
};

void Connection::run()
{
    id_counter = 0;
    closed = false;
    try {
        socket = tcp::socket(context);
        tcp::resolver resolver(context);
        spdlog::info("Connecting to pool {}, port {}...",connectionData.host, connectionData.port);
        asio::connect(*socket, resolver.resolve(connectionData.host, connectionData.port));
        spdlog::info("Stratum connected.");
        async_read_line();

        stratum_subscribe("janusminer/0.0.1");
        stratum_authorize(connectionData.user, connectionData.pass);
    } catch (std::exception& e) {
        spdlog::error(e.what());
    }
    context.run();
    spdlog::info("Stratum disconnected.");
}
void Connection::async_read_line()
{
    asio::async_read_until(*socket, response, "\n",
        [&](const std::error_code& e, std::size_t) {
            if (e) {
                if (e != asio::error::operation_aborted) {
                    spdlog::error("Read error: {}", e.message());
                    close_connection();
                }
                return;
            }
            std::string line;
            std::getline(std::istream(&response), line);
            // cout << line << endl;
            auto parsed = stratum::parse_line(line);
            try {
                if (parsed) {
                    std::visit([&](auto&& msg) {
                        handle_msg(std::move(msg));
                    },
                        std::move(*parsed));

                    if (!closed)
                        async_read_line();
                    return;
                }
            } catch (...) {
            }
            spdlog::error("Cannot parse line {}", isprint(line) ? line : "");
            close_connection();
        });
}

}
