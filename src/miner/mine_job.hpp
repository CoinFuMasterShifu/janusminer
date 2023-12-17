#pragma once
#include "block/block.hpp"
#include "block/header/difficulty_declaration.hpp"
struct MineJob {
    // data
    Block block;
    Target t;
    size_t cleanIndex;
    // methods
    void set_random_seed(uint32_t newSeed);
    std::array<uint8_t,76> get_header()const ;
};
