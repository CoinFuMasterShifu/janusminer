#include "verus_job.hpp"
#include "log/trace.hpp"
#include "spdlog/spdlog.h"
#include "verusopt/verus_clhash_opt.hpp"
#include <cassert>

namespace Verus {

void JobQueue::clear(size_t newCleanIndex)
{
    *this = {};
    cleanIndex = newCleanIndex;
};

void JobQueue::push(QueuedJob j)
{
    if (j.clean_index() < cleanIndex)
        return;
    const size_t N { j.mined.size() };
    if (N == 0) {
        // it can happen that no tried sha256t hashess were within the 
        // selection band (c,c+cpu_hashrate/gpu_hashrate)
        // applied in the kernel 
        return;
    }
    fresh = false;
    _watermark += N;
    trace_log().debug("[QUEUE/PUSH] {}, {}", _watermark, N);
    queue.push(std::make_shared<QueuedJob>(std::move(j)));
}

void queue_drained()
{
    using namespace std::chrono;
    static std::optional<steady_clock::time_point> last;
    auto now { steady_clock::now() };
    if (!last.has_value() || (now - *last) > seconds(10)) {
        trace_log().warn("[QUEUE/DRAINED]");
        spdlog::warn("CPU queue drained");
        last = now;
    }
}

double JobQueue::mine_proportion()
{
    if (!hashrate.has_value())
        return 1.0;
    size_t a { _watermark * 10 };
    size_t b { *hashrate };
    if (b > a)
        return 1.0;
    return double(b) / double(a);
};

std::optional<Verus::WorkerJob> JobQueue::pop(size_t N)
{
    size_t total { 0 };
    std::vector<CandidateBatch> res;
    while (true) {
        if (queue.size() == 0)
            break;
        auto& j { *queue.front() };
        auto& rs { j.mined.result_spans() };
        assert(cursor < rs.size());
        size_t M = std::min(N - total, rs.size() - cursor);
        assert(M > 0);
        res.push_back(CandidateBatch {
            .shared { queue.front() },
            .targetV2 { j.target() },
            .resultSpans { rs },
            .offset = cursor,
            .len = M });

        cursor += M;
        total += M;
        assert(_watermark >= M);
        _watermark -= M;
        if (cursor == rs.size()) {
            cursor = 0;
            queue.pop();
        }
        if (total >= N) {
            assert(total == N);
            break;
        }
    }
    if (_watermark == 0) {
        queue_drained();
    }
    if (res.size() == 0)
        return {};
    assert(total > 0);
    trace_log().debug("[QUEUE/POP] {}, {}", _watermark, total);
    return Verus::WorkerJob {
        .batches { res },
        .mineProportion = mine_proportion(),
        .total = total,
        .cleanIndex = cleanIndex
    };
}
}
