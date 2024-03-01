#pragma once
#include "spdlog/spdlog.h"
#include <memory>

extern std::shared_ptr<spdlog::logger> mining_log;
void initialize_mining_log();
