#pragma once
#include "verusopt/verus_clhash_opt.hpp"
#include <array>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

class VerusPool;
struct VerusTiming;

namespace Verus {

class Worker {
    using headerdata = std::array<uint8_t, 76>;
    using duration_t = std::chrono::steady_clock::duration;

public:
    Worker(size_t index, VerusPool& pool);
    Worker(const Worker&) = delete;
    ~Worker()
    {
        std::lock_guard l(m);
        shutdown = true;
        cv.notify_one();
    }
    Hashrate hashrate();

    void wake_up()
    {
        push_event(ContinueMining {});
    }

    void stop_mining();

private: // event stuff
    struct PauseMining { };
    struct ContinueMining { };
    using Event = std::variant<PauseMining, ContinueMining>;
    std::vector<Event> events;
    void push_event(Event);

    void handle_event(const PauseMining&);
    void handle_event(const ContinueMining&);

private:
    void work();
    bool has_work();
    bool try_mining();
    void adjust_batch_size(const VerusTiming&);

private:
    const size_t workerIndex;
    VerusPool& pool;
    std::thread t;
    JanusMinerOpt miner;
    size_t batchSize { 1000 };
    size_t ignoreBelow { 0 };
    bool continueMining { false };

    // for hashrate
    struct HashrateState {
        using tp = std::chrono::steady_clock::time_point;
        size_t hashes{0};
        tp begin;
        tp end;
        HashrateState(tp p)
            :begin(p), end(p)
        {}
    };
    std::optional<HashrateState> hashrateState;
    std::mutex hashrateMutex;

    std::mutex m;
    std::condition_variable cv;

    bool shutdown { false };
    bool wakeup { false };
    std::optional<headerdata> header;
};
}
