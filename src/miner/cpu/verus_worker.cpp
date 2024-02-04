#include "verus_worker.hpp"
#include "device_pool.hpp"
#include "log/trace.hpp"
#include "verusopt/verus_clhash_opt.hpp"
#include <chrono>
#include <optional>

using namespace std::chrono;

namespace Verus {
bool Worker::has_work()
{
    return shutdown || events.size() > 0;
}
Worker::Worker(size_t index, VerusPool& pool)
    : workerIndex(index)
    , pool(pool)
{
    t = std::thread([&] { work(); });
}

void Worker::adjust_batch_size(const VerusTiming& t)
{
    if (t.processed < 100 && t.duration<10ms) 
        batchSize *= 100;
    else
        batchSize = t.processed*(100.0ms/t.duration);
    // spdlog::error("scaling batchSize {} ms", t.duration/1.0ms); //TODO
};

bool Worker::try_mining()
{
    auto p { pool.pop_job(batchSize) };
    if (!p)
        return false;

    auto res { miner.mine(*p) };
    assert(res.total != 0);
    VerusTiming vt {
        .duration { res.duration },
        .total = res.total,
        .processed = res.processed,
    };

    { // update hashate info
        std::lock_guard l(hashrateMutex);
        auto now{std::chrono::steady_clock::now()};
        if (!hashrateState.has_value()) {
            hashrateState = HashrateState{now};
        }else{
            hashrateState->end = now;
        }
        hashrateState->hashes += res.processed;
    }

    // trace_log().debug("[VERUS/MINED] {} of {}, {}/{} in {}ms", res.processed, res.total, double(res.processed) / res.total, vt.threshold.targetProportion, duration_cast<milliseconds>(vt.duration).count());
    pool.trace_verus(vt, workerIndex);
    adjust_batch_size(vt);
    if (res.success) {
        pool.push_janus_mined(std::move(*res.success));
    }
    return true;
}

Hashrate Worker::hashrate(){
    std::lock_guard l(hashrateMutex);
    if (!hashrateState) 
        return {0};
    auto delta{ hashrateState->end-hashrateState->begin};
    if (delta == delta.zero()) 
        return {0};
    Hashrate hr{hashrateState->hashes,delta};
    hashrateState = {};
    return hr;
};
void Worker::stop_mining()
{
}

void Worker::work()
{
    while (true) {
        decltype(events) tmpEvents;
        {
            std::unique_lock l(m);
            cv.wait(l, [&] { return continueMining || has_work(); });
            tmpEvents.swap(events);
            if (shutdown)
                return;
        }

        // handle events
        for (auto& e : tmpEvents)
            std::visit([&](auto&& e) { handle_event(std::move(e)); }, std::move(e));

        // try mining
        if (continueMining) {
            continueMining = try_mining();
        }
    }
}
void Worker::push_event(Event e)
{
    std::lock_guard l(m);
    events.push_back(std::move(e));
    cv.notify_all();
}

void Worker::handle_event(const PauseMining&)
{
    spdlog::warn("Pause mining");
    continueMining = false;
}

void Worker::handle_event(const ContinueMining&)
{
    continueMining = true;
}

}
