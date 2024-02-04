#pragma once
#include "api_call.hpp"
#include "block/block.hpp"
#include "block/header/header_impl.hpp"
#include "cpu/verus_job.hpp"
#include "crypto/address.hpp"
#include "general/hex.hpp"
#include "gpu/hashrate_estimator.hpp"
#include "gpu/opencl_hasher.hpp"
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
    std::optional<StratumGeneratorArgs> generator_args();

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
    [[nodiscard]] std::optional<StratumGeneratorArgs> update(const stratum::Mining_Notify&);
    [[nodiscard]] std::optional<StratumGeneratorArgs> update(const stratum::Mining_SetDifficulty&);
    void update(const stratum::Subscription&);
};

class MiningCoordinator {
    friend class DeviceWorker;
    friend class Verus::Worker;

public:
    MiningCoordinator(const std::vector<CL::Device>& devices, size_t verusWorkers, std::variant<stratum::ConnectionData, NodeConnectionData> connectionData);

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


    void set_hashrate(Hashrate hahsrate);
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
    void assign_work(StratumGeneratorArgs& sj);
    void stop_mining();
    void submit(const Block& b);
    void submit(const stratum::Submission& b);

    // multithread stuff
    Sha256tOpenclHasher sha256tHasher;
    SyncTools st;
    std::atomic<bool> shutdown { false };

    // verus tuned counter
    std::mutex verus_mutex;
    size_t verusTunedCounter { 0 };

    // std::optional<ThresholdBisecter> bisecter;

    // events
    std::vector<Event> events;

    // mining properties
    uint64_t minedcount { 0 };

    std::mutex job_mutex;

    

    // Job status for node mining
    std::optional<Header> currentHeader;
    std::optional<Hash> prevHash; // to determine clean/outdated blocks
    size_t cleanIndex{0};
                                  
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
