#pragma once
#include "connection_data.hpp"
#include "messages.hpp"
#include <mutex>
#include <string>
#include <thread>
namespace stratum {
class Connection;
class ConnectionServer {
    friend class Connection;
public:
    ConnectionServer(ConnectionData d, std::function<void(stratum::Event)> handler);
    void run();
    void submit(const stratum::Submission&, size_t connectionId);
private:
    void handle_event(stratum::Event&&);
private:
    std::function<void(stratum::Event)> handler;
    Connection* connection{nullptr};
    std::mutex m;
    const ConnectionData connectionData;
    std::thread t;
};
}
