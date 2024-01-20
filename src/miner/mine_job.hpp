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
    std::array<uint8_t, 76> header() const;
    Block submission(std::span<const uint8_t,4> nonce) const{
        Block b { block };
        std::copy(nonce.begin(),nonce.end(),b.header.begin()+HeaderView::offset_nonce);
        return b;
    }
};
