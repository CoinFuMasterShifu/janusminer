#pragma once
#include "block/header/header.hpp"
#include "cyclic_queue.hpp"
#include "general/hex.hpp"
#include "gpu/cl_helper.hpp"
#include "gpu/mined_values.hpp"
#include "hashrate.hpp"
#include "job.hpp"
#include "mine_job.hpp"
#include "spdlog/spdlog.h"
#include "stratum/job.hpp"
#include <CL/opencl.hpp>
#include <atomic>
#include <iostream>
#include <string>
#include <vector>

struct HashrateDelta {
    ssize_t delta;
};

class Sha256tOpenclHasher;

struct JobNonceRange {
    job::Job j;
    uint32_t offset;
    uint32_t N;
};

class JobNonceTracker {
    job::Job j;
    size_t remaining;
    uint32_t offset;

public:
    bool exhausted() const { return remaining == 0; }
    JobNonceTracker(job::Job j);
    std::optional<JobNonceRange> get_job(size_t N);
};

class Sha256tGPUHasher {
    friend class Sha256tOpenclHasher;
    using Functor = cl::KernelFunctor<CL::Buffer, uint32_t, uint32_t, CL::Buffer>;
    class JobLock {
    private:
        auto lock()
        {
            std::lock_guard l(operation_m);
            nLocks += 1;
            if (nLocks == 1) {
                ol = std::unique_lock(m);
            }
        }
        void unlock()
        {
            std::lock_guard l(operation_m);
            nLocks -= 1;
            if (nLocks == 0) {
                ol = {};
            }
        }
        struct Locker {
            JobLock& jl;
            Locker(JobLock& jl)
                : jl(jl)
            {
                jl.lock();
            }
            Locker(const Locker&) = delete;
            ~Locker()
            {
                jl.unlock();
            }
        };

    public:
        using locker_t = std::unique_ptr<Locker>;
        auto acquire()
        {
            return std::make_unique<Locker>(*this);
        }
        JobLock() {};
        JobLock(const JobLock&) = delete;
        ~JobLock()
        {
            std::unique_lock l(m);
        }

    private:
        friend struct Locker;
        std::mutex operation_m;
        std::mutex m;
        size_t nLocks { 0 };
        std::optional<std::unique_lock<std::mutex>> ol;
    };
    struct KernelRunner {
        using time_point = std::chrono::steady_clock::time_point;
        Sha256tGPUHasher& parent;
        CL::Buffer headerBuffer;
        CL::Buffer resultsBuffer;
        std::mutex m;
        struct CurrentJob {
            JobNonceRange jr;
            std::shared_ptr<CyclicQueue::Buffer> sha256tValues;
            JobLock::locker_t parentLock;
            uint32_t threshold;
        };
        std::optional<CurrentJob> currentJob;
        time_point start;
        std::optional<std::array<uint8_t, 76>> currentHeader;
        std::vector<cl::Event> events;
        cl::Event finalEvent;
        // size_t results_size() const { return resultsBuffer.size() / 2 / sizeof(uint32_t); }
        static uint32_t threshold(size_t M, size_t N)
        {
            constexpr uint32_t c = 0x00FFFFFF;
            return c + (M * size_t(0xFFFFFFFF - c)) / N;
        }

        void on_complete()
        {
            decltype(currentJob) tmp;
            TripleSha::MinedValues minedValues { [&]() {
                std::lock_guard l(m);
                TripleSha::MinedValues res(
                    std::move(currentJob->jr.j),
                    currentJob->threshold,
                    std::move(currentJob->sha256tValues));
                currentJob.reset();
                return res;
            }() };
            parent.handle_finished_job(std::move(minedValues));
        }
        void try_start(CL::CommandQueue& queue, double fraction, size_t N, Sha256tGPUHasher::Functor& functor)
        {
            assert(fraction > 0.0); // TODO: failed
            using namespace std;
            size_t M;
            {
                std::lock_guard l(m);
                if (currentJob.has_value())
                    return;
                auto allocator { parent.allocator() };
                size_t maxM { allocator.available() };
                if (maxM == 0)
                    return;
                constexpr double a { 1.005 }; // add 0.5% extra space due to variance in number of good candidates
                auto NN { double(maxM) / a / fraction };
                if (NN < maxM)
                    NN = maxM;
                if (NN < N)
                    N = NN;
                M = std::min(size_t(N * fraction * 1.005), N);
                if (M == 0)
                    M = 1;
                auto jr { parent.get_worker_range(N) };
                if (!jr)
                    return;
                auto allocated = allocator.allocateElements(M);
                if (!allocated)
                    return;
                auto t { threshold(M, jr->N) };
                currentJob = CurrentJob {
                    .jr = *jr,
                    .sha256tValues = std::move(allocated),
                    .parentLock { parent.jobLock.acquire() },
                    .threshold = t
                };
            }

            using namespace std::chrono;
            start = steady_clock::now();
            size_t offset { currentJob->jr.offset };
            N = currentJob->jr.N;

            check_update_header(queue);
            if ((resultsBuffer.size() - 1) / 2 / sizeof(uint32_t) < M)
                reserve_results(queue.context(), M);
            reset_results_count(queue);
            // cl::EnqueueArgs eargs(queue, events, cl::NDRange(1), cl::NDRange(1), cl::NullRange);
            cl::EnqueueArgs eargs(queue, events, cl::NDRange(offset), cl::NDRange(N), cl::NullRange);

            std::vector<cl::Event> v(1);
            v[0] = functor(eargs, headerBuffer, currentJob->threshold, uint32_t(M), resultsBuffer);

            // buf1
            auto& hostResults { currentJob->sha256tValues };
            auto& span1 { hostResults->first };
            size_t size1 { span1.size() * sizeof(span1[0]) };
            cl::Event e;
            queue.enqueueReadBuffer(resultsBuffer, false, 0, size1, span1.data(), &v, &e);
            if (!hostResults->second.empty()) {
                auto& span2 { hostResults->second };
                size_t size2 { span2.size() * sizeof(span2[0]) };
                std::vector<cl::Event> v { e };
                queue.enqueueReadBuffer(resultsBuffer, false, size1, size2, span2.data(), &v, &finalEvent);
            } else {
                finalEvent = e;
            }
            using funtype = std::function<void()>;
            finalEvent.setCallback(
                CL_COMPLETE, [](cl_event, cl_int, void* user_data) {
                    auto pfun = reinterpret_cast<funtype*>(user_data);
                    (*pfun)();
                    delete pfun;
                },
                new funtype([&]() {
                    on_complete();
                }));
        }
        void reserve_results(cl::Context context, size_t nResults)
        {
            resultsBuffer = CL::Buffer(context, CL_MEM_READ_WRITE, sizeof(uint32_t) * (2 * nResults + 1));
        }
        KernelRunner(Sha256tGPUHasher& parent, size_t N)
            : parent(parent)
            , headerBuffer(parent.context, CL_MEM_READ_WRITE, 80)
            , events(2)
        {
            reserve_results(parent.context, N);
        }
        void check_update_header(CL::CommandQueue queue)
        {
            std::lock_guard l(m);
            auto h { currentJob.value().jr.j.header() };
            if (currentHeader != h) {
                using namespace std;
                currentHeader = h;
                queue.enqueueWriteBuffer(headerBuffer, false, 0, h.size(), currentHeader->data(), nullptr, &events[0]);
            }
        }
        void reset_results_count(CL::CommandQueue queue)
        {
            queue.enqueueFillBuffer(resultsBuffer, 0u, 0, 4, nullptr, &events[1]);
        }
    };
    friend struct KernelRunner;

public:
    Sha256tGPUHasher(Sha256tOpenclHasher& parent, CL::Device& device);
    Sha256tGPUHasher(const Sha256tGPUHasher&) = delete;
    ~Sha256tGPUHasher() {};

    void start_mining(Header h);
    [[nodiscard]] HashrateDelta stop_mining();
    [[nodiscard]] HashrateDelta set_zero();

    const std::string deviceName;

private:
    struct Allocator : protected CyclicQueue::Allocator {
        Allocator(CyclicQueue::Allocator a)
            : CyclicQueue::Allocator(std::move(a))
        {
        }
        size_t available() const { return CyclicQueue::Allocator::available(); }
        std::shared_ptr<CyclicQueue::Buffer> allocateElements(size_t elements);
    };
    auto allocator() -> Allocator;

    double fraction() const;
    void handle_finished_job(TripleSha::MinedValues);
    std::pair<Hashrate, ssize_t> hashrate();
    void try_start();
    void reset_start();
    std::optional<JobNonceRange> get_worker_range(size_t N);

    Sha256tOpenclHasher& parent;
    cl::Context context;
    cl::Program program;
    Functor functor;

    CL::CommandQueue queue;

    std::recursive_mutex m;

    // job related
    std::optional<JobNonceTracker> job;
    uint32_t offset;
    size_t hashesTried { 0 };

    size_t N { 10000000 };
    bool active { false };
    HashrateWatcher hashrateWatcher;
    ssize_t prevHashrate { 0 };

    KernelRunner runner1;
    KernelRunner runner2;
    JobLock jobLock;
};

class Sha256tOpenclHasher {
public:
    using Job = job::Job;
    using JobGenerator = job::Generator;
    friend class Sha256tGPUHasher;
    using callback_t = std::function<void(TripleSha::MinedValues)>;
    Sha256tOpenclHasher(std::vector<CL::Device> devices, callback_t callback, size_t queuesizeGB)
        : on_mined(std::move(callback))
    {
        cyclicQueue = std::make_shared<CyclicQueue>([&] { allocation_possible(); }, (queuesizeGB* 1000000000)/4 );
        for (auto& d : devices) {
            hashers.push_back(std::make_unique<Sha256tGPUHasher>(*this, d));
        }
    }
    ~Sha256tOpenclHasher()
    {
        stop_mining();
    }
    std::pair<uint64_t, std::vector<std::pair<std::string, uint64_t>>> hashrates();
    // void start_mining(const Header& header)
    // {
    //     for (auto& h : hashers) {
    //         h->start_mining(header);
    //     }
    // }
    void stop_mining()
    {
        std::lock_guard l(m);
        std::cout << "Stop mining" << std::endl;
        for (auto& h : hashers) {
            totalHashrate += h->stop_mining().delta;
        }
        assert(totalHashrate == 0);
    }

    void update_verushashrate(Hashrate);
    void set_work(job::GeneratorArg);

private:
    void handle_finished_job(TripleSha::MinedValues, HashrateDelta hd);
    void wakeup();
    void update_fraction_locked();
    void allocation_possible();
    std::optional<Job> generate_job();

private:
    callback_t on_mined;
    std::atomic<double> fraction { 0.1 };
    std::shared_ptr<CyclicQueue> cyclicQueue;

    std::mutex m;
    job::Generator jobGenerator;
    ssize_t totalHashrate { 0 };
    size_t cleanSum { 0 };
    std::optional<Hashrate> verushashrate;

    std::vector<std::unique_ptr<Sha256tGPUHasher>> hashers;
};
