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
    assert(t.proportionOfRequested != 0);
    auto tt { t.duration / t.proportionOfRequested };

    double factor;
    if (tt > 150ms) {
        factor = 100ms/tt;
        return;
    }else if (tt< 50ms){
        factor = 120ms/tt;
    }else if (tt < 100ms){
        factor = 110ms/tt;
    }else{
        return;
    }
    // auto prev{batchSize};
    batchSize *= factor;
    // spdlog::error("scaling batchSize {}*{} -> {}", prev,factor,batchSize);
};

bool Worker::try_mining()
{
    auto p { pool.pop_job(batchSize) };
    if (!p)
        return false;

    auto res { miner.mine(p->jobs, p->threshold.minThreshold) };
    assert(res.total != 0);
    VerusTiming vt {
        .duration { res.duration },
        .total = res.total,
        .processed = res.processed,
        .proportionOfRequested = p->proportionOfWanted * res.total / p->total,
        .threshold = p->threshold
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

    trace_log().debug("[VERUS/MINED] {} of {}, {}/{} in {}ms", res.processed, res.total, double(res.processed) / res.total, vt.threshold.targetProportion, duration_cast<milliseconds>(vt.duration).count());
    pool.trace_verus(vt, p->cleanIndex, workerIndex);
    adjust_batch_size(vt);
    if (res.success) {
        auto& [hash, block] = *res.success;
        spdlog::info("header: {}", serialize_hex(block.header));
        spdlog::info("SHA256(header): {}", serialize_hex(hashSHA256(hashSHA256(hashSHA256(block.header)))));
        spdlog::info("verush(header): {}", serialize_hex(verus_hash(block.header)));
        assert(hash == verus_hash(block.header));
        pool.push_janus_mined(block);
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
