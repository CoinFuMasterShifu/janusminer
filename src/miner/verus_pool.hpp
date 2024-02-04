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
        ssize_t set(const Hashrate& hr)
        {
            auto res { ssize_t(hr.val) - v };
            v = hr.val;
            return res;
        };
        ssize_t v { 0 };
    };

public:
    void push(size_t deviceId, const Hashrate& hr)
    {
        total += perDevice[deviceId].set(hr);
    }
    Hashrate hashrate() const
    {
        return { size_t(total) };
    }
    void reset()
    {
        *this = {};
    }

private:
    std::map<size_t, Val> perDevice;
    ssize_t total { 0 };
};
}

struct Block;
class MiningCoordinator;

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
    VerusPool(MiningCoordinator& parent, SyncTools& st, size_t nWorkers);

    std::pair<Hashrate, std::vector<Hashrate>> hashrates();
    void notify_verus_tuned(size_t rIndex);
    void process();
    bool locked_needs_wakeup() { return events.size() > 0; }
    void push_job(Verus::QueuedJob);
    void stop_mining();

    void push_janus_mined(Verus::Success&& b);

    void clean(size_t index)
    {
        cleanIndex = index;
    }

    // async functions (with locks)
    std::optional<Verus::WorkerJob> pop_job(size_t N);
    void trace_verus(VerusTiming vt, size_t workerIndex)
    {
        push_event(TraceVerus { vt, workerIndex });
    }
    auto get_clean_index() const { return cleanIndex; }

private:
    struct TraceVerus {
        VerusTiming timing;
        size_t workerIndex;
    };
    struct NoVerusInput {
    };

    using Event = std::variant<TraceVerus, NoVerusInput>;

    void handle_event(TraceVerus&& e);
    void handle_event(NoVerusInput&& e);

    void push_event(Event e);

    MiningCoordinator& parent;
    SyncTools& st;
    AverageEstimator verusHashrateEstimator;

    std::vector<Event> events;

    // private owned variables

    // job stuff
    // template <typename T>
    // class MutexProtected {
    //     T data;
    //     std::mutex m;
    //     class Session {
    //         friend class MutexProtected;
    //         std::unique_lock<std::mutex> l;
    //         T& t;
    //         Session(std::mutex& m, T& t)
    //             : l(m)
    //             , t(t)
    //         {
    //         }
    //
    //     public:
    //         T& operator()() { return t; }
    //     };
    //
    // public:
    //     auto session() { return Session(m, data); }
    // };
    // struct QueueData {
    //     Verus::JobQueue jobQueue;
    // };
    // MutexProtected<QueueData> queueData;
    std::mutex jobMutex;
    Verus::JobQueue jobQueue;

    size_t cleanIndex { 0 };

    AllCount allCount;

    std::vector<std::unique_ptr<Verus::Worker>> workers;
};
