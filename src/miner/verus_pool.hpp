#pragma once
#include "cpu/efficiency_estimator.hpp"
#include "cpu/verus_job.hpp"
#include "cpu/verus_worker.hpp"
#include "synctools.hpp"
#include "verusopt/verus_clhash_opt.hpp"
#include <memory>
#include <vector>

namespace Verus {
struct HashrateEstimator2 {
    struct Val {
        ssize_t set(const Hashrate& hr){
            auto res{ssize_t(hr.val) - v};
            v = hr.val;
            return res;
        };
        ssize_t v{0};
    };
    public:
    void push(size_t deviceId, const Hashrate& hr)
    {
        total += perDevice[deviceId].set(hr);
    }
    Hashrate hashrate() const{
        return {size_t(total)};
    }
    void reset(){
        *this = {};
    }
    private:

    std::map<size_t,Val> perDevice;
    ssize_t total{0};
};
}

struct Block;
class DevicePool;

class AllCount {
public:
    AllCount(size_t s)
        : bv(s)
    {
    }
    void reset(size_t i)
    {
        bv.resize(i, {});
        reset();
    }
    void reset()
    {
        fresh = true;
        reset_no_fresh();
    }
    std::optional<std::pair<bool, PerSecond>> set_timing(size_t i, const PerSecond& perSecond)
    {
        if (bv[i].has_value() == false) {
            bv[i] = perSecond;
            count += 1;
            if (count == bv.size()) {
                std::pair res { fresh, total_per_second() };
                reset_no_fresh();
                fresh = false;
                return res;
            }
        }
        return {};
    }

private:
    void reset_no_fresh()
    {
        bv.assign(bv.size(), {});
        count = 0;
    }
    PerSecond total_per_second()
    {
        PerSecond res;
        for (auto& o : bv) {
            res.total.val += o.value().total;
            res.considered.val += o.value().considered;
        }
        return res;
    }
    size_t count { 0 };
    bool fresh = true;
    std::vector<std::optional<PerSecond>> bv;
};

class VerusPool {

public:
    VerusPool(DevicePool& parent, SyncTools& st, size_t nWorkers);

    std::pair<Hashrate,std::vector<Hashrate>> hashrates();
    void notify_verus_tuned(size_t rIndex);
    void process();
    bool locked_needs_wakeup() { return events.size() > 0; }
    void push_job(Verus::PoolJob);
    void stop_mining();
    void clean(TargetV2 nextTarget);

    void push_janus_mined(Verus::Success&& b);

    // async functions (with locks)
    std::optional<Verus::WorkerJob> pop_job(size_t N);
    void trace_verus(VerusTiming vt, size_t cleanIndex, size_t workerIndex)
    {
        push_event(TraceVerus { vt, cleanIndex, workerIndex });
    }
    auto get_clean_index() const { return cleanIndex; }

private:
    struct TraceVerus {
        VerusTiming timing;
        size_t cleanIndex;
        size_t workerIndex;
    };
    struct NoVerusInput {
    };
    using Event = std::variant<TraceVerus, NoVerusInput>;

    void handle_event(TraceVerus&& e);
    void handle_event(NoVerusInput&& e);
    void push_event(Event e);

    // rate adjustment functions
    void update_consume_per_second(Hashrate);
    void update_produce_per_second(Hashrate);
    void update_threshold();
    void set_threshold(Verus::MineThreshold);

    DevicePool& parent;
    SyncTools& st;
    AverageEstimator verusHashrateEstimator;
    // EfficiencyEstimator efficiencyEstimator;

    std::vector<Event> events;

    void benchmark_none();

    // private owned variables

    // job stuff
    std::mutex jobMutex;
    Verus::JobQueue jobQueue;

    std::optional<Hashrate> consumePerSecond; // consumer per second
    std::optional<Hashrate> producePerSecond; // producer per second
    Verus::HashrateEstimator2 hashrateEstimator;
                                              
    size_t cleanIndex { 0 };
    std::optional<TargetV2> target;
    double prevAlpha{1.0};
    std::optional<Target> prevThresholdTarget;
    Verus::MineThreshold threshold;

    AllCount allCount;

    std::vector<std::unique_ptr<Verus::Worker>> workers;
};
