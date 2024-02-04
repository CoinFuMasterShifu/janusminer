#include "verus_pool.hpp"
#include "block/header/difficulty.hpp"
#include "device_pool.hpp"
#include "helpers.hpp"
#include "log/trace.hpp"
#include "spdlog/spdlog.h"
using namespace std::chrono_literals;

// auto t2 { b.header.target_v2() };
// if (!bisecter) {
//     bisecter = ThresholdBisecter(t2);
//     auto ignoreBelow { bisecter->get_candidate() };
//     for (auto& w : verusWorkers) {
//         w->set_threshold(ignoreBelow, rateIndex);
//     }
// } else {
//     bisecter->set_target(t2);
// }

void VerusPool::push_janus_mined(Verus::Success&& b)
{
    parent.push_janus_mined(std::move(b));
}

void VerusPool::push_event(Event e)
{
    std::lock_guard l(st.m);
    events.push_back(std::move(e));
    st.wakeup = true;
    st.cv.notify_one();
}

VerusPool::VerusPool(MiningCoordinator& parent, SyncTools& st, size_t nWorkers)
    : parent(parent)
    , st(st)
    , allCount(nWorkers)
{
    for (size_t i = 0; i < nWorkers; ++i) {
        workers.push_back(std::make_unique<Verus::Worker>(i, *this));
    }
}

std::pair<Hashrate, std::vector<Hashrate>> VerusPool::hashrates()
{
    size_t total { 0 };
    std::vector<Hashrate> res;
    for (auto& w : workers) {
        auto hr { w->hashrate() };
        total += hr;
        res.push_back(hr);
    }
    return { total, std::move(res) };
};

std::optional<Verus::WorkerJob> VerusPool::pop_job(size_t N)
{
    std::unique_lock l(jobMutex);
    auto job { jobQueue.pop(N) };
    if (!job) {
        if (jobQueue.is_fresh() == false)
            push_event(NoVerusInput {});
    }
    return job;
}

void VerusPool::push_job(Verus::QueuedJob job)
{
    {
        std::unique_lock l(jobMutex);
        jobQueue.push(std::move(job));
    }
    for (auto& w : workers) {
        w->wake_up();
    }
}

void VerusPool::stop_mining()
{
    for (auto& w : workers) {
        w->stop_mining();
    }
};

void VerusPool::handle_event(NoVerusInput&&) {
    // spdlog::warn("NoVerusInput {}", cleanIndex);
    // std::lock_guard l(verus_mutex);
    // if (e.rateIndex != rateIndex)
    //     return;
    // assert(bisecter);
    // bisecter.value().candidate_too_big();

    // set_ignore_below();
};

void VerusPool::handle_event(TraceVerus&& e)
{
    if (e.timing.duration < 20ms) // bad hashrate estimate on too short timings
        return;
    using namespace std::chrono;
    auto ms { duration_cast<milliseconds>(e.timing.duration).count() };
    // if (ms < 100 || ms > 150) {
    //     spdlog::warn("Trace verus reset {}", ms);
    // } else {
    auto ps { e.timing.per_second() };
    if (ps) {
        auto o = allCount.set_timing(e.workerIndex, ps.value());
        if (o) {
            auto& [fresh, totalPerSecond] = *o;
            trace_log().debug("[VERUS/TRACE] {}, {}/s", ms, totalPerSecond.total.format().to_string());
            verusHashrateEstimator.push_hashrate(totalPerSecond.considered.val);

            {
                std::unique_lock l(jobMutex);
                jobQueue.set_hashrate(totalPerSecond.considered);
            }
            parent.set_hashrate(totalPerSecond.considered);
        }
    }
}

void VerusPool::process()
{
    decltype(events) ev;
    {
        std::lock_guard l(st.m);
        ev = std::move(events);
    }
    for (auto& e : ev) {
        std::visit([&](auto&& e) {
            handle_event(std::move(e));
        },
            std::move(e));
    }
};
