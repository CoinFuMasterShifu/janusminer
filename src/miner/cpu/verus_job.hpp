#pragma once
#include "block/block.hpp"
#include "block/header/difficulty_declaration.hpp"
#include "gpu/mined_values.hpp"
#include "hashrate.hpp"
#include <memory>
#include <optional>
#include <queue>
#include <vector>

namespace Verus {
struct PoolJob {
    TripleSha::MinedValues mined;
    TargetV2 target;
};

struct MineThreshold {
    double targetProportion { 1 };
    uint32_t minThreshold { 0 };
    bool operator==(const MineThreshold&) const = default;
};
struct MinerJob {
    using V = std::vector<uint32_t>;
    uint32_t end_index() const { return job_end - vec_begin; }
    uint32_t begin_index() const { return job_begin - vec_begin; }
    TargetV1 target(uint32_t i) const;
    bool ignore_index(uint32_t i, uint32_t threshold) const;
    std::shared_ptr<PoolJob> shared;
    uint32_t nonceOffset;
    TargetV2 targetV2;
    V::const_iterator vec_begin;
    V::const_iterator job_begin;
    V::const_iterator job_end;
};

struct WorkerJob {
    std::vector<Verus::MinerJob> jobs;
    Verus::MineThreshold threshold;
    double proportionOfWanted;
    size_t total;
    size_t cleanIndex;
};

class JobQueue {
    struct ActiveJob {
        size_t cursor { 0 };
    };

public:
    void push(PoolJob j);
    [[nodiscard]] bool empty() const { return queue.empty(); }
    std::optional<Verus::WorkerJob> pop(size_t N, Verus::MineThreshold);
    bool is_fresh() const { return fresh; }
    void clear(size_t newCleanIndex);
    // auto get_threshold() const {return minThreshold;}
    void set_job_accept(size_t jai) { jobAcceptIndex = jai; }
    auto get_clean_index() const { return cleanIndex; }
    bool compabitlbe(const Verus::WorkerJob&);
    [[nodiscard]] auto watermark() const {return _watermark;}

private:
    void drop();

private:
    bool fresh = true;
    size_t jobAcceptIndex { 0 };
    size_t cleanIndex { 0 };
    size_t cursor { 0 };
    std::queue<std::shared_ptr<PoolJob>> queue;
    size_t _watermark { 0 };
};
};
