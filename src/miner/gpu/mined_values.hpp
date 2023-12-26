#pragma once
#include <vector>
#include <cstdint>
#include <chrono>
#include "block/block.hpp"
#include "block/header/difficulty_declaration.hpp"
#include "hashrate.hpp"

namespace TripleSha{
struct MinedValues {
    std::chrono::steady_clock::duration duration;
    std::vector<uint32_t> invertedTargets;
    TargetV2 target;
    size_t deviceId;
    uint32_t offset;
    Block block;
    size_t cleanIndex;
    [[nodiscard]] Hashrate hashrate() const{
        return {invertedTargets.size(),duration};
    };
};

}
