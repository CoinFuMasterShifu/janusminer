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

void VerusPool::clean(TargetV2 nextTarget)
{
    target = nextTarget;
    cleanIndex += 1;
    std::lock_guard l(jobMutex);
    jobQueue.clear(cleanIndex);
};

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

VerusPool::VerusPool(DevicePool& parent, SyncTools& st, size_t nWorkers)
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
    return {total,std::move(res)};
};

std::optional<Verus::WorkerJob> VerusPool::pop_job(size_t N)
{
    auto t { threshold };

    size_t M = double(N) / t.targetProportion;
    trace_log().debug("[VERUS/POP] {}, {}, {}", M, N, t.targetProportion);
    std::lock_guard l(jobMutex);
    auto job { jobQueue.pop(M, t) };
    if (!job) {
        if (jobQueue.is_fresh() == false)
            push_event(NoVerusInput {});
    }
    return job;
}

void VerusPool::push_job(Verus::PoolJob job)
{
    auto hr { job.mined.hashrate() };

    hashrateEstimator.push(job.mined.deviceId, hr);
    update_produce_per_second(hashrateEstimator.hashrate());

    if (job.mined.cleanIndex != cleanIndex)
        return;
    {
        std::lock_guard l(jobMutex);
        jobQueue.push(std::move(job));
    }
    for (auto& w : workers) {
        w->wake_up();
    }
};

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
            // auto
            // kf
            trace_log().debug("[VERUS/TRACE] {}, {}, {}/s", fresh, ms, totalPerSecond.total.format().to_string());
            if (fresh) {
                // now first time all are active
                // efficiencyEstimator.push(totalPerSecond);
                verusHashrateEstimator.push_hashrate(totalPerSecond.considered.val);
            } else {
                // update last pushed
                // efficiencyEstimator.update_last(totalPerSecond);
                verusHashrateEstimator.push_hashrate(totalPerSecond.considered.val);
            }
            update_consume_per_second(totalPerSecond.total);
        }
    }
    // }
}

void VerusPool::update_consume_per_second(Hashrate ps)
{
    trace_log().debug("[VERUS/CONSUMERATE] {}/s", ps.format().to_string());
    assert(ps.val > 0);
    consumePerSecond = ps;
    update_threshold();
}

void VerusPool::update_produce_per_second(Hashrate ps)
{
    trace_log().debug("[SHA256T/PRODUCERATE] {}/s", ps.format().to_string());
    assert(ps.val > 0);
    producePerSecond = ps;
    update_threshold();
};

void VerusPool::update_threshold()
{
    auto hr { verusHashrateEstimator.average_hashrate() };

    if (!producePerSecond.has_value()
        || !consumePerSecond.has_value()
        || !hr.has_value()) {
        trace_log().debug("[UPDATETHRESHOLD/FAILED]");
        return; // cannot estimate
    }

    Hashrate currentPerSecond = *hr;
    Hashrate targetPerSecond(*producePerSecond);
    assert(targetPerSecond > 0.0 && *consumePerSecond > 0.0);

    // double p { e->process };
    // double s { e->scan };
    // if (s <= p) {
    //     spdlog::warn("Invalid CPU performance estimate.");
    //     return false;
    // }
    // if (targetPerSecond > s) {
    //     spdlog::error("GPU outruns CPU.");
    //     return false;
    // }

    assert(target.has_value());
    // double alpha = (targetPerSecond - p) / (s - p); // 0 < alpha < 1
    //
    trace_log().debug("[UPDATETHRESHOLD/HASHRATES] {}/s, {}/s", currentPerSecond.format().to_string(), targetPerSecond.format().to_string());
    if (targetPerSecond < currentPerSecond) {
        trace_log().warn("[UPDATETHRESHOLD/CPUOUTRUNSGPU] {}/s, {}/s", currentPerSecond.format().to_string(), targetPerSecond.format().to_string());
        spdlog::warn("CPU outruns GPU. {}/s > {}/s", currentPerSecond.format().to_string(), targetPerSecond.format().to_string());
    }
    double correctionFactor { 1.0 };
    if (producePerSecond) {
        const auto lowerHard = producePerSecond->hash_per(100ms);
        const auto lowerSoft = producePerSecond->hash_per(200ms);
        const auto upperSoft = producePerSecond->hash_per(300ms);
        const auto upperHard = producePerSecond->hash_per(400ms);
        const auto val = jobQueue.watermark();
        if (val < lowerHard)
            correctionFactor = 1.3;
        else if (val < lowerSoft)
            correctionFactor = 1.1;
        else if (val > upperSoft)
            correctionFactor = 0.9;
        else if (val > upperHard)
            correctionFactor = 0.7;
    }
    double alpha = std::min(1.0, correctionFactor * hr->val / targetPerSecond); // 0 < alpha < 1
    double tau { 1 / (*target).difficulty() }; // target
    double c(alpha < tau ? 1.0 // only try already successful candidates
                         : tau / alpha);
    TargetV1 t(1 / c);
    trace_log().debug("[UPDATETHRESHOLD/FACTOR] {}, {}, {}", correctionFactor, alpha, t.difficulty());

    if (t == prevThresholdTarget)
        return; // no change, nothing to do
    prevThresholdTarget = t;
    prevAlpha = alpha;

    // form threshold
    uint32_t threshold = ((~uint32_t(t.zeros8())) << 24) + t.bits24();
    set_threshold({ alpha, threshold });

    return;
};

void VerusPool::set_threshold(Verus::MineThreshold mt)
{
    trace_log().debug("[VERUS/THRESHOLD] {}, {}", mt.minThreshold, mt.targetProportion);

    // save new threshold
    threshold = mt;

    // clear old worker timings
    allCount.reset();
};

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
