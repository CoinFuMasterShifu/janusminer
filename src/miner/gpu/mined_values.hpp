#pragma once
#include "block/block.hpp"
#include "block/header/difficulty_declaration.hpp"
#include "hashrate.hpp"
#include "mine_job.hpp"
#include "stratum/job.hpp"
#include <chrono>
#include <cstdint>
#include <vector>

namespace TripleSha {
struct MinedValues {
    std::chrono::steady_clock::duration duration;
    std::vector<uint32_t> invertedTargets;
    size_t deviceId;
    uint32_t offset;
    std::variant<MineJob, StratumJob> job;
    std::variant<Block, stratum::Submission> submit(std::span<const uint8_t, 4> nonce) const
    {
        return std::visit([&](auto& j) -> std::variant<Block, stratum::Submission> { return j.submission(nonce); }, job);
    }

    [[nodiscard]] Hashrate hashrate() const
    {
        return { invertedTargets.size(), duration };
    };
    auto header() const
    {
        return std::visit([&](auto& j) { return j.header(); }, job);
    };
};

}
