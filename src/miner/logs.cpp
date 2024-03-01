#include "logs.hpp"
#include "spdlog/sinks/rotating_file_sink.h"

std::shared_ptr<spdlog::logger> mining_log;

namespace{
    auto create_mining_logger()
{
    auto max_size = 1048576 * 5; // 5 MB
    auto max_files = 3;
    return spdlog::rotating_logger_mt("stratum_connections", "stratum_connections.log", max_size, max_files);
}


}
void initialize_mining_log(){
    mining_log = create_mining_logger();;
    mining_log->flush_on(spdlog::level::info);
};

