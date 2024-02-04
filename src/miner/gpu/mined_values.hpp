#pragma once
#include "c_constant.hpp"
#include "cyclic_queue.hpp"
#include "job.hpp"

namespace TripleSha {
class MinedValues {
public:
    std::variant<Block, stratum::Submission> submit(std::span<const uint8_t, 4> nonce) const
    {
        return std::visit([&](auto& j) -> std::variant<Block, stratum::Submission> { return j.submission(nonce); }, job);
    }

    auto header() const
    {
        return std::visit([&](auto& j) { return j.header(); }, job);
    };
    MinedValues(job::Job job, uint32_t threshold, std::shared_ptr<CyclicQueue::Buffer> values)
        : job(std::move(job))
        , threshold(threshold)
        , sha256tValues(std::move(values))
        , resultSpan(sha256tValues->result_spans())
    {
    }
    uint32_t scaled_threshold(double d) const
    {
        assert(d <= 1.0 && d > 0.0);
        return uint32_t(d * (threshold - c_constant)) + c_constant;
    }
    [[nodiscard]] auto& result_spans() const { return resultSpan; }
    size_t size() const
    {
        return resultSpan.size();
    }
    size_t clean_index() const { return job.clean_index(); }
    auto target() const { return job.target(); }

// private: TODO
    job::Job job;
    uint32_t threshold;
    std::shared_ptr<CyclicQueue::Buffer> sha256tValues;
    sha256t_results::spans resultSpan;
};

}
