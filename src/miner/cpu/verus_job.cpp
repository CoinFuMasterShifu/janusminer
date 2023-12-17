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

void JobQueue::push(PoolJob j)
{
    auto& v { j.mined.invertedTargets };
    const size_t N { v.size() };
    assert(N > 0);
    fresh = false;
    _watermark += N;
    trace_log().debug("[QUEUE/PUSH] {}, {}", _watermark, N);
    auto a { std::make_shared<PoolJob>(j) };
    assert(a.get());
    queue.push(std::move(a));
    while (_watermark >= 1000000000) {
        drop();
    }
}
void JobQueue::drop()
{
    assert(queue.size() > 0);
    auto& j { *queue.front() };
    const size_t n { j.mined.invertedTargets.size() };
    assert(n > cursor);
    const size_t s { n - cursor };
    assert(_watermark >= s);
    _watermark -= s;
    trace_log().warn("[QUEUE/DROP] {}, {}", _watermark, s);
    spdlog::warn("CPU queue full, dropping {} entries, new watermark is {} ", s, _watermark);
    queue.pop();
    cursor = 0;
};

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

std::optional<Verus::WorkerJob> JobQueue::pop(size_t N, Verus::MineThreshold t)
{
    size_t total { 0 };
    std::vector<MinerJob> res;
    while (true) {
        if (queue.size() == 0)
            break;
        auto& j { *queue.front() };
        auto& v { j.mined.invertedTargets };
        assert(cursor < v.size());
        size_t M = std::min(N - total, v.size() - cursor);
        assert(M > 0);
        res.push_back({
            .shared { queue.front() },
            .nonceOffset = j.mined.offset,
            .vec_begin { v.begin() },
            .job_begin { v.begin() + cursor },
            .job_end { v.begin() + cursor + M },
        });
        auto& mj = res.back();

        // TODO: remove
        const uint32_t end_index(mj.end_index());
        uint32_t i = mj.begin_index();
        assert(i < end_index);

        cursor += M;
        total += M;
        assert(_watermark >= M);
        _watermark -= M;
        if (cursor == v.size()) {
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
        .jobs { res },
        .threshold { t },
        .proportionOfWanted = double(total) / N,
        .total = total,
        .cleanIndex = cleanIndex
    };
}
}
