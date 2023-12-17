#include "trace.hpp"
#include "spdlog/sinks/rotating_file_sink.h"
#include <optional>
struct TraceLog {
    std::shared_ptr<spdlog::logger> logger;

    constexpr static auto max_size = 1048576 * 5; // 5 MB
    constexpr static auto max_files = 3;

    TraceLog()
        : logger(spdlog::rotating_logger_mt("trace", "logs/mine.trace", max_size, max_files)){
            logger->set_level(spdlog::level::err);
        }
};

namespace {
    std::optional<TraceLog> l;
}
spdlog::logger& trace_log()
{
    if (!l.has_value()) {
        l = TraceLog{};
    }
    return *l.value().logger;
};
