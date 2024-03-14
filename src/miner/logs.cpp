#include "logs.hpp"
#include "spdlog/sinks/rotating_file_sink.h"

std::shared_ptr<spdlog::logger> stratum_mining_log;
std::shared_ptr<spdlog::logger> node_mining_log;

namespace{
    auto create_mining_logger(const std::string &logger_name, const std::string &filename)
{
    auto max_size = 1048576 * 5; // 5 MB
    auto max_files = 3;
    return spdlog::rotating_logger_mt(logger_name, filename, max_size, max_files);
}


}
void initialize_mining_log(){
    stratum_mining_log = create_mining_logger("stratum_connections", "stratum_connections.log");
    stratum_mining_log->flush_on(spdlog::level::info);
    node_mining_log = create_mining_logger("node_mining_log", "node_mining.log");
    node_mining_log->flush_on(spdlog::level::info);
};

