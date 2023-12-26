#pragma once
#include "cl_function.hxx"
#include "cl_helper.hpp"
#include "kernel.hpp"
#include "mine_job.hpp"
#include "spdlog/spdlog.h"
#include <arpa/inet.h>
#include <condition_variable>
#include <iostream>
#include <optional>
#include <span>
#include <thread>
#include <variant>

class MinerDevice {
    static constexpr size_t numSlots = 8;
    static cl::Program::Sources fetch_sources()
    {
        std::string code { kernel, sizeof(kernel) };
        // auto code{read_file("kernel.cl")};
        return cl::Program::Sources { { code.data(), code.size() } };
    };
    auto build_program(cl::Context context)
    {
        cl::Program program(context, fetch_sources());
        try {
            program.build(
                "-cl-std=CL2.0 -DVECT_SIZE=2 -DDGST_R0=3 -DDGST_R1=7 -DDGST_R2=2 "
                "-DDGST_R3=6 -DDGST_ELEM=8 -DKERNEL_STATIC");
            return program;
        } catch (cl::BuildError& e) {
            auto logs { e.getBuildLog() };
            assert(logs.size() == 1);
            auto& log = logs[0].second;
            std::cerr << " Build error: " << log << std::endl
                      << std::flush;

            throw e;
        }
    }

public:
    struct DoubleShaJob {
        uint32_t target;
        std::array<uint8_t, 76> header;
    };
    struct TripleShaJob {
        TargetV2 t;
        std::array<uint8_t, 76> header;
    };
    MinerDevice(cl::Device device)
        : context({ device })
        , program(build_program(context))
        , queue(context, device)
        , reset_counter_fun(program, "iojefwf23fsdf")
        , set_target_fun(program, "set_target")
        , mine_fun_double_sha(program, "mine")
        // , mine_fun_triple_sha(program, "mine2") {};
        , mine_fun_triple_sha(program, "ij49280gd") {};

    auto mine(uint32_t nHashes, uint32_t offset)
    {
        cl::EnqueueArgs eargs(queue,
            offset == 0 ? cl::NullRange : cl::NDRange(offset),
            cl::NDRange(nHashes), cl::NullRange);
        return mine_fun_double_sha.run(queue, eargs, blockHeader);
    }
    auto mine_triple_sha(uint32_t nHashes, uint32_t offset)
    {
        cl::EnqueueArgs eargs(queue, cl::NullRange, cl::NDRange(nHashes), cl::NullRange);
        auto t { std::get<TripleShaJob>(job).t };
        return mine_fun_triple_sha.run(queue, eargs, blockHeader, offset, t.zeros10(), t.bits22());
    }
    auto reset_counter()
    {
        cl::EnqueueArgs nd1(queue, cl::NDRange(1));
        return reset_counter_fun.run(queue, nd1);
    }

    void set_double_sha_job(DoubleShaJob j)
    { // for lecacy algorithm
        set_block_header(j.header);
        set_target(j.target);
        job = j;
    }
    void set_triple_sha_job(TripleShaJob j)
    { // for lecacy algorithm
        set_block_header(j.header);
        job = j;
    }
    size_t set_triple_sha_batchsize(size_t tsbs);

    const TripleShaJob& get_job() const{return std::get<TripleShaJob>(job);}
private:
    void set_block_header(std::span<uint8_t, 76> h)
    {
        memcpy(blockHeader.data(), h.data(), h.size());
    }
    void set_target(uint32_t v)
    {
        cl::EnqueueArgs nd1(queue, cl::NDRange(1));
        set_target_fun.run(queue, nd1, v);
    }
    size_t reserved { 5000000ul };
    cl::Context context;
    cl::Program program;

public: // TODO private
    std::variant<DoubleShaJob, TripleShaJob> job;

private:
    std::array<uint8_t, 76> blockHeader;
    TargetV2 target;
    CL::CommandQueue queue;
    CLFunction<>::Returning<uint32_t> reset_counter_fun;
    CLFunction<uint32_t>::Returning<> set_target_fun;
    // CLFunction<std::array<uint8_t, 76>>::Returning<> set_block_header_fun;
    CLFunction<std::array<uint8_t, 76>>::Returning<std::array<uint32_t, numSlots>,
        std::array<std::array<uint8_t, 32>, numSlots>>
        mine_fun_double_sha;
    CLFunction<std::array<uint8_t, 76>, uint32_t, uint32_t, uint32_t>::Returning<std::vector<uint32_t>>
        mine_fun_triple_sha;
};

class DevicePool;
class DeviceWorker {
public:
    DeviceWorker(size_t deviceId, const CL::Device& device, DevicePool& pool)
        : deviceId(deviceId)
        , pool(pool)
        , miner(device)
        , deviceName(device.name())
    {
        thread = std::jthread([=, this]() { run(); });
        hashesPerStep = miner.set_triple_sha_batchsize(hashesPerStep);
    };
    void set_triple_sha_batchsize(size_t tsbs);
    DeviceWorker(const DeviceWorker&) = delete;
    DeviceWorker(DeviceWorker&&) = delete;
    ~DeviceWorker()
    {
        thread.request_stop();
        cv.notify_one();
    }

    uint64_t get_hashrate()
    {
        if (!getHashrateCheckpoint.has_value())
            return 0;
        using namespace std::chrono;
        auto hashes { hashCounter.exchange(0) };
        auto now { steady_clock::now() };
        if (hashes == 0)
            return 0;
        auto us = duration_cast<microseconds>(now - *getHashrateCheckpoint).count();
        getHashrateCheckpoint = now;
        if (us == 0)
            return 0;
        return (hashes * 1000 * 1000) / us;
    }

    void set_job(const MineJob& b)
    {
        push_event(SetTask { b });
    };
    void stop_mining()
    {
        push_event(Stop {});
    }

private:
    void run();
    bool try_mine();
    void mine_double_sha();
    void mine_triple_sha();

    void init_mining();

    //////////////////////////////
    /// event properties/methots
    struct Stop {
    };
    struct SetTask {
        MineJob task;
    };
    using Event = std::variant<Stop, SetTask>;
    std::vector<Event> events;
    void push_event(Event);
    void handle_event(const Stop&);
    void handle_event(SetTask&&);

    // thread variables
    uint32_t hashesPerStep { 500000u };
    uint32_t hashesTried { 0 };
    uint32_t randOffset { 0 };

    size_t deviceId;
    DevicePool& pool;
    MinerDevice miner;

    // external variables
    std::optional<std::chrono::steady_clock::time_point> lastHashrateCheckpoint;
    std::optional<std::chrono::steady_clock::time_point> getHashrateCheckpoint;

    // atomic shared variables
    std::atomic<uint64_t> hashCounter { 0 };

    // shared variables
    std::mutex m;
    bool wakeup = false;
    std::condition_variable cv;
    std::optional<MineJob> currentTask;
    std::jthread thread;

    // const variables
public:
    const std::string deviceName;
};
