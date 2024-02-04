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
struct QueuedJob {
    const TripleSha::MinedValues mined;
    size_t clean_index() const { return mined.clean_index(); }
    TargetV2 target() const { return mined.target(); }
};


struct CandidateBatch {
    uint32_t scaled_threshold(double d) const{
        return shared->mined.scaled_threshold(d);
    }
    std::shared_ptr<QueuedJob> shared;
    TargetV2 targetV2;
    const sha256t_results::spans& resultSpans;
    size_t offset;
    size_t len;
};

struct WorkerJob {
    std::vector<Verus::CandidateBatch> batches;
    double mineProportion;
    size_t total;
    size_t cleanIndex;
};

class JobQueue {
public:
    void push(QueuedJob j);
    [[nodiscard]] bool empty() const { return queue.empty(); }
    std::optional<Verus::WorkerJob> pop(size_t N);
    bool is_fresh() const { return fresh; }
    void clear(size_t newCleanIndex);
    void set_hashrate(size_t hr) { hashrate = hr; }
    [[nodiscard]] auto watermark() const { return _watermark; }

private:
    double mine_proportion();
    bool fresh = true;
    std::optional<size_t> hashrate;
    size_t cleanIndex { 0 };
    size_t cursor { 0 };
    std::queue<std::shared_ptr<QueuedJob>> queue;
    size_t _watermark { 0 };
};

struct Success {
    Hash hash;
    std::variant<Block, stratum::Submission> submission;
};

}
