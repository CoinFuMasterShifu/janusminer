#include "connection_server.hpp"
#include "connection.hpp"
#include <chrono>

using namespace std::chrono_literals;

namespace stratum {

void ConnectionServer::handle_event(stratum::Event&& e)
{
    handler(std::move(e));
}

void ConnectionServer::submit(const stratum::Submission& s, size_t connectionId){
    std::lock_guard l(m);
    if (!connection) return;
    connection->submit(s,connectionId);

};

ConnectionServer::ConnectionServer(ConnectionData d, std::function<void(stratum::Event)> handler)
    : handler(std::move(handler))
    , connectionData(d)
{
    t = std::thread([&]{run();});
}

void ConnectionServer::run()
{
    while (true) {
        Connection c(*this);
        c.run();
        spdlog::info("Reconnecting in 1 second.");
        std::this_thread::sleep_for(1000ms);
    }
}

}
