#include "opencl_hasher.hpp"
#include "crypto/hasher_sha256.hpp"
#include "general/hex.hpp"
#include "helpers.hpp"
#include "kernel.hpp"
#include "spdlog/spdlog.h"
#include <iostream>

JobNonceTracker::JobNonceTracker(job::Job j)
    : j(std::move(j))
    , remaining(size_t(std::numeric_limits<uint32_t>::max()) + 1)
    , offset(randuint32())
{
}

auto JobNonceTracker::get_job(size_t N) -> std::optional<JobNonceRange>
{
    N = std::min(N, remaining);
    if (N == 0)
        return {};
    remaining -= N;
    auto tmpOffset { offset };
    offset += uint32_t(N);
    return JobNonceRange {
        .j { j },
        .offset = tmpOffset,
        .N = uint32_t(N)
    };
};

namespace {
[[nodiscard]] auto build_program(cl::Context context)
{
    std::string code { kernel, strlen(kernel) - 1 };
    try {
        cl::Program program(context, std::string(code.begin(), code.end()));
        program.build("-cl-std=CL2.0 -DC_CONSTANT=" C_CONSTANT_STR);
        // program.build("-cl-std=CL2.0");
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
}

Sha256tGPUHasher::Sha256tGPUHasher(Sha256tOpenclHasher& parent, CL::Device& device)
    : deviceIndex(device.index())
    , deviceName { device.name() }
    , parent(parent)
    , context(device)
    , program(build_program(context))
    , functor(program, "h")
    , queue(context, device)
    , runner1(*this, N)
    , runner2(*this, N)
{
}
std::shared_ptr<CyclicQueue::Buffer> Sha256tGPUHasher::Allocator::allocateElements(size_t elements)
{
    assert(elements > 0);
    if (auto allocated = alloc(1 + 2 * elements); allocated) {
        assert((allocated->first.size() + allocated->second.size()) / 2 <= elements);
        return allocated;
    }
    return {};
}

using namespace std;

// void Sha256tGPUHasher::start_mining(Header h)
// {
//     cout << "Start mining" << endl;
//     std::lock_guard l(m);
//     active = true;
//     runner1.set_next_header(h);
//     runner2.set_next_header(h);
//     try_start();
// }
HashrateDelta Sha256tGPUHasher::stop_mining()
{
    std::lock_guard l(m);
    hashrateWatcher.reset();
    active = false;
    HashrateDelta hr { prevHashrate };
    prevHashrate = 0;
    return hr;
}

HashrateDelta Sha256tGPUHasher::set_zero()
{

    std::lock_guard l(m);
    HashrateDelta hd {
        .delta = -prevHashrate,
    };
    prevHashrate = 0;
    return hd;
};

double Sha256tGPUHasher::fraction() const
{
    return parent.fraction;
}
auto Sha256tGPUHasher::allocator() -> Allocator
{
    return parent.cyclicQueue->allocator();
}

void Sha256tGPUHasher::handle_finished_job(TripleSha::MinedValues mv) // is called by opencl callback thread
{

    // {
    //     union {
    //         uint32_t u32[20];
    //         uint8_t u8[80];
    //     } header;
    //     std::memcpy(header.u8, mv.header().data(), 76);
    //     // std::memset(header.u8, 0, 80);
    //     // cout<<"Current header: "<<serialize_hex(mv.header(),80)<<endl;
    //     // auto s = mv.sha256tValues->first;
    //     // cout<<"Read buffer: "<< serialize_hex((uint8_t*)s.data(),std::min(s.size(),10ul));
    //     auto sps { mv.result_spans() };
    //     for (auto& sp : sps.spans) {
    //         for (size_t i = 0; i < sp.size() && i < 1; ++i) {
    //             cout<<"Nonce: "<<sp[i].nonce()<<endl;
    //             header.u32[19] = hton32(sp[i].nonce());
    //             auto hs { hashSHA256(hashSHA256(hashSHA256(header.u8,80))) };
    //             uint32_t swapped = sp[i].hashStart();
    //             cout<<serialize_hex(hs)<<" "<<serialize_hex(swapped)<<"\n";
    //         }
    //     }
    //     return;
    // }

    // auto spans { mv.result_spans() };
    // uint32_t nsuccess(spans.size());
    // cout << "Success.size(): " << nsuccess << " N: " << N << endl;
    std::lock_guard l(m);
    auto duration { hashrateWatcher.register_hashes(N) };
    // if (duration) {
    //     cout << "duration: " << *duration / 1.0ms << " milliseconds" << endl;
    // }
    auto [currentHashrate, delta] = hashrate();
    N = currentHashrate.val / 20;
    if (N < 1000)
        N = 1000;
    parent.handle_finished_job(std::move(mv), { delta });
    try_start();
}

std::pair<Hashrate, ssize_t> Sha256tGPUHasher::hashrate()
{
    ssize_t currentHashrate(hashrateWatcher.hashrate().val);
    ssize_t delta = ssize_t(currentHashrate) - prevHashrate;
    prevHashrate = currentHashrate;
    return { currentHashrate, delta };
}

void Sha256tGPUHasher::reset_start()
{
    {
        std::unique_lock l(m);
        active = true;
    }
    job = {};
    try_start();
}

std::optional<JobNonceRange> Sha256tGPUHasher::get_worker_range(size_t N)
{
    // try update job new
    if (!job.has_value()) {
        if (auto j = parent.generate_job(); j.has_value()) {
            job = JobNonceTracker { j.value() };
        } else
            return {};
    }

    if (auto j { job->get_job(N) }; j.has_value()) {
        if (job->exhausted())
            job.reset();
        return j;
    } else {
        job.reset();
        return {};
    }
}

void Sha256tGPUHasher::try_start()
{
    std::unique_lock l(m);
    if (active) {
        double f { fraction() };
        runner1.try_start(queue, f, N, functor);
        runner2.try_start(queue, f, N, functor);
    }
}

void Sha256tOpenclHasher::update_fraction_locked()
{
    using namespace std;
    using namespace std::chrono;
    if (!verushashrate)
        return;
    auto& vh(*verushashrate);

    if (totalHashrate == 0)
        fraction = 1.0;

    double tmp = double(verushashrate.value()) / double(totalHashrate);
    if (tmp > 1.0) {
        static optional<steady_clock::time_point> lastReported;
        auto now { steady_clock::now() };
        if (!lastReported || *lastReported + 10s < now) {
            spdlog::warn("CPU ({}/s verus v2.1) outruns GPUs ({}/s sha256t)", vh.format().to_string(), Hashrate(totalHashrate).format().to_string());
            lastReported = now;
        }
    }
    assert(tmp != 0.0);
    fraction = tmp;
}

void Sha256tOpenclHasher::allocation_possible()
{
    cleanSum = 0;
    for (auto& h : hashers) {
        h->try_start();
    }
}

std::pair<uint64_t, std::vector<std::tuple<uint32_t, std::string, uint64_t>>> Sha256tOpenclHasher::hashrates()
{
    std::lock_guard l(m);
    std::vector<std::tuple<uint32_t, std::string, uint64_t>> v;
    for (auto& h : hashers) {
        auto [hashrate, delta] = h->hashrate();
        totalHashrate += delta;
        v.push_back({ h->deviceIndex, h->deviceName, hashrate });
    }
    return { totalHashrate, std::move(v) };
};

void Sha256tOpenclHasher::update_verushashrate(Hashrate hr)
{
    assert(hr.val != 0);
    std::lock_guard l(m);
    verushashrate = hr;
    update_fraction_locked();
}

void Sha256tOpenclHasher::set_work(job::GeneratorArg a)
{
    {
        std::lock_guard l(m);
        std::visit([&](auto& a) { jobGenerator.from_arg(std::move(a)); }, a);
    }
    for (auto& h : hashers)
        h->reset_start();
}
void Sha256tOpenclHasher::wakeup()
{
    for (auto& h : hashers) {
        h->try_start();
    }
};

void Sha256tOpenclHasher::handle_finished_job(TripleSha::MinedValues mined, HashrateDelta hd)
{
    {
        std::lock_guard l(m);
        totalHashrate += hd.delta;
    }
    on_mined(std::move(mined));
}

auto Sha256tOpenclHasher::generate_job() -> std::optional<Job>
{
    std::lock_guard l(m);
    return jobGenerator.generate();
}
