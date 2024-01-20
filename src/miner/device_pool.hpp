#pragma once
#include "api_call.hpp"
#include "block/block.hpp"
#include "block/header/header_impl.hpp"
#include "cpu/verus_job.hpp"
#include "crypto/address.hpp"
#include "general/hex.hpp"
#include "gpu/hashrate_estimator.hpp"
#include "gpu/worker.hpp"
#include "stratum/connection_server.hpp"
#include "stratum/messages.hpp"
#include "synctools.hpp"
#include "verus_pool.hpp"
#include <chrono>
#include <vector>
namespace Verus {
class Worker;
}
struct NodeConnectionData {
    std::string host;
    uint16_t port;
    Address address;
};

class StratumConnectionData {
    StratumJobDataBase jobdata;
    std::optional<stratum::Mining_Notify> _notify;
    std::optional<stratum::Subscription> _subscription;

private:
    std::optional<StratumJobGenerator> get_job();

public:
    auto get_connection_id() const
    {
        return jobdata.connectionId;
    }
    void clear(size_t connectionId = 0)
    {
        *this = {};
        jobdata.connectionId = connectionId;
    }
    [[nodiscard]] std::optional<StratumJobGenerator> update(const stratum::Mining_Notify&);
    [[nodiscard]] std::optional<StratumJobGenerator> update(const stratum::Mining_SetDifficulty&);
    void update(const stratum::Subscription&);
};

class HashrateEstimator { // TODO: estimate on producer side
    struct Node {
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point last;
        size_t sum;
        Node()
            : start(std::chrono::steady_clock::now())
            , last(start)
            , sum(0)
        {
        }
        void add(size_t v)
        {
            sum += v;
            last = std::chrono::steady_clock::now();
        }
        size_t hash_per_second() const
        {
            using namespace std::chrono;
            auto us { duration_cast<microseconds>(steady_clock::now() - start).count() };
            if (us == 0)
                return 0;
            return (sum * 1000 * 1000) / us;
        }
        void finalize()
        {
            start = last;
            sum = 0;
        }
    };

public:
    [[nodiscard]] std::optional<Hashrate> push(const TripleSha::MinedValues&);
    void reset()
    {
        *this = {};
    }

private:
    std::optional<Hashrate> hashes_per_second();
    std::map<size_t, Node> hashesPerDevice;
    std::optional<std::chrono::steady_clock::time_point> wakeup;
};

class DevicePool {
    friend class DeviceWorker;
    friend class Verus::Worker;

public:
    DevicePool(const std::vector<CL::Device>& devices, size_t verusWorkers, std::variant<stratum::ConnectionData, NodeConnectionData> connectionData);
    bool empty() const { return workers.empty(); }

    void notify_mined_triple_sha(TripleSha::MinedValues); // for Janushash
    void push_janus_mined(Verus::Success&& s)
    {
        push_event(OnJanusMined { std::move(s) });
    }

    void notify_shutdown()
    {
        shutdown = true;
        st.cv.notify_one();
    }

    void run();
    using OnJanusMined = Verus::Success;
    using StratumSetDiff = stratum::Mining_SetDifficulty;
    using StratumNotify = stratum::Mining_Notify;
    struct OnStratumJob {
        Block b;
    };
    using WorkerResult = TripleSha::MinedValues;

    using Event = std::variant<WorkerResult, OnJanusMined,
        StratumSetDiff, StratumNotify>;

    class JobStatus {
    public:
        auto get_key(const Header& h) const
        {
            std::array<uint8_t, 76> out;
            memset(out.data(), 0, 76);
            std::copy(h.data(), h.data() + 36, out.data());
            std::copy(h.data() + 68, h.data() + 76, out.data());
            return out;
        }
        [[nodiscard]] bool push_block(const Block& b)
        {
            bool cleared = false;
            if (prevHash != b.header.prevhash()) {
                blocks.clear();
                prevHash = b.header.prevhash();
                cleared = true;
            }
            auto key { get_key(b.header) };
            auto iter { blocks.find(key) };
            if (iter == blocks.end())
                blocks.emplace(key, b);
            return cleared;
        };

        bool valid_block(const Block& b)
        {
            auto key { get_key(b.header) };
            auto iter { blocks.find(key) };
            if (iter == blocks.end()) {
                // size_t i { 0 };
                // for (auto& b : blocks) {
                //     spdlog::debug("Key {}: {}", i++, serialize_hex(b.first));
                // }
                // spdlog::debug("Cannot find key {}.", serialize_hex(key));
                return false;
            }
            return true;
        };

    private:
        std::optional<Hash> prevHash;
        std::map<std::array<uint8_t, 76>, Block> blocks;
    };

private:
    void init_connection(const stratum::ConnectionData&);
    void init_connection(const NodeConnectionData&);
    void push_event(Event e)
    {
        std::lock_guard l(st.m);
        events.push_back(std::move(e));
        st.cv.notify_one();
    };
    void print_hashrate();
    void handle_event(const WorkerResult&);
    void handle_event(OnJanusMined&&);

    void handle_event(StratumNotify&&);
    void handle_event(StratumSetDiff&&);
    void handle_event(stratum::ConnectionStart&&);
    void handle_event(stratum::ConnectionEnd&&);
    void handle_event(stratum::Subscription&&);

    void poll();
    void set_ignore_below();
    void assign_work(const Block& b);
    void assign_work(StratumJobGenerator&& sj);
    void stop_mining();
    void submit(const Block& b);
    void submit(const stratum::Submission& b);

    // multithread stuff
    SyncTools st;
    std::atomic<bool> shutdown { false };

    // verus tuned counter
    std::mutex verus_mutex;
    size_t verusTunedCounter { 0 };

    // std::optional<ThresholdBisecter> bisecter;

    // events
    std::vector<Event> events;

    // mining properties
    std::atomic_int64_t blockSeed { 0 };
    uint64_t minedcount { 0 };

    std::mutex job_mutex;

    //
    std::optional<Block> task;
    std::vector<std::unique_ptr<DeviceWorker>> workers;

    // JobStatus
    JobStatus jobStatus;
    VerusPool verusPool;

    // pool properties
    bool needsPoll { false };
    const std::chrono::seconds printInterval { 10 };
    const std::chrono::milliseconds pollInterval { 500 };
    std::chrono::steady_clock::time_point nextPoll;

    // for direct node communication
    std::optional<Address> address;
    std::unique_ptr<API> api;

    std::unique_ptr<stratum::ConnectionServer> stratumConnection;
    StratumConnectionData stratumConnectionData;
};
