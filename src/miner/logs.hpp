#pragma once
#include "spdlog/spdlog.h"
#include <memory>

extern std::shared_ptr<spdlog::logger> stratum_mining_log;
extern std::shared_ptr<spdlog::logger> node_mining_log;
void initialize_mining_log();
