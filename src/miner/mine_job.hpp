#pragma once
#include "block/block.hpp"
#include "block/header/difficulty_declaration.hpp"
#include <atomic>

class NodeJobGenerator;
 
struct MineJob {
    // data
    using generator_t = NodeJobGenerator;
    Block block;
    TargetV2 t;
    size_t cleanIndex;
    // methods
    std::array<uint8_t, 76> header() const;
    void set_random_seed(uint32_t newSeed);
    size_t clean_index() const {return cleanIndex;}
    TargetV2 target() const {return t;}
    Block submission(std::span<const uint8_t, 4> nonce) const
    {
        Block b { block };
        std::copy(nonce.begin(), nonce.end(), b.header.begin() + HeaderView::offset_nonce);
        return b;
    }
};

class NodeJobGenerator {
private:
    mutable std::atomic<uint32_t> nonce2;
    MineJob mineJob;
public:
    NodeJobGenerator(NodeJobGenerator&&);

    NodeJobGenerator(MineJob job);
    MineJob generate_job() const;
};
