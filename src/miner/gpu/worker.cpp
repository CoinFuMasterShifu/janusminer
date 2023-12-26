#include "worker.hpp"
// #include <arpa/inet.h>
#include "block/header/header_impl.hpp"
#include "crypto/hasher_sha256.hpp"
#include "device_pool.hpp"
#include "helpers.hpp"
#include "log/trace.hpp"
#include <fstream>
#include <iostream>

size_t MinerDevice::set_triple_sha_batchsize(size_t hashesPerStep)
{
    reserved = std::max(hashesPerStep, reserved);
    reserved = mine_fun_triple_sha.override_reserve_outvector<0>(reserved);
    if (hashesPerStep> reserved) 
        hashesPerStep = reserved;
    mine_fun_triple_sha.generator<0>().elements = hashesPerStep;
    return hashesPerStep;
}

void DeviceWorker::init_mining()
{
    hashesTried = 0;
    auto& t = currentTask.value();
    t.set_random_seed((pool.blockSeed++) % 2000);
    randOffset = randuint32();

    if (!t.t.is_janushash()) {
        miner.set_double_sha_job({ t.t.binary(), t.get_header() });
    } else {
        miner.set_triple_sha_job({ std::get<TargetV2>(t.t.get()), t.get_header() });
    }
}

bool DeviceWorker::try_mine()
{
    using namespace std::chrono;
    if (!currentTask)
        return false;
    auto& t { *currentTask };
    if (t.t.is_janushash()) {
        mine_triple_sha();
    } else {
        mine_double_sha();
    }

    return true;
};

void DeviceWorker::mine_double_sha()
{
    if (hashesTried == std::numeric_limits<uint32_t>::max()) {
        init_mining();
    }
    auto nHashes { std::min(std::numeric_limits<uint32_t>::max() - hashesTried,
        hashesPerStep) };
    auto [args, hashes] = miner.mine(nHashes, hashesTried + randOffset);
    auto [found] = miner.reset_counter();
    using namespace std;
    if (found > 0) {
        auto& t { currentTask.value() };
        auto& h { t.block.header };
        h.set_nonce(hton32(args[0]));
        pool.notify_mined_double_sha(t.block); // TODO handle current task jobIndex
    }

    hashesTried += nHashes;
    hashCounter += nHashes;
};

std::optional<std::string> path;

void save_targets(const std::vector<uint32_t>& targets)
{
    using namespace std;
    using namespace std::chrono;
    if (!path) {
        auto usts { duration_cast<microseconds>(system_clock::now().time_since_epoch()).count() };
        path = to_string(usts / 1000) + ".bin";
    }
    std::ofstream f(*path, ios::app | ios::binary);
    for (uint32_t t : targets) {
        f.write((const char*)&t, sizeof(t));
    }
}

inline uint32_t compute_targetv1_invzeros(const uint32_t zeros10, const uint32_t bits22, const std::array<uint32_t, 8>& hash)
{
    uint32_t targetzeros = zeros10;
    uint32_t hashzeros = 0;
    size_t i = 0;
    for (; i < 8; ++i) {
        if (hash[i] != 0)
            break;
        hashzeros += 32;
    }
    if (i == 8)
        return 0xFFFFFFFFu;
    u64 hbits;
    if (i == 7) {
        hbits = 0;
        ((uint32_t*)&hbits)[1] = htobe32(hash[i]);
        // memcpy(&hbits, hash + i, 4);
    } else {
        ((uint32_t*)&hbits)[0] = htobe32(hash[i + 1]);
        ((uint32_t*)&hbits)[1] = htobe32(hash[i]);
        // memcpy(&hbits, hash + i, 8);
    }
    hashzeros += 32;
    while (hbits >= ((u64)1) << 32) {
        hbits >>= 1;
        hashzeros -= 1;
    }
    if (hbits < ((u64)1) << 31) {
        return 0xFFFFFFFFu;
    }
    // -> hbits in [2^31,2^32)
    if (targetzeros < hashzeros)
        return 0xFFFFFFFFu;
    uint32_t zeros = targetzeros - hashzeros;
    u64 bits64 = ((u64)bits22) << (10 + 32); // in [2^63,2^64)
    bits64 = bits64 / hbits; // in (2^31,2^33)
    if (bits64 >= ((u64)1) << 32) {
        if (zeros == 0) {
            return 0xFFFFFFFFu;
        }
        zeros -= 1;
        bits64 >>= 9; // in [2^23,2^24)
    } else {
        bits64 >>= 8; // in (2^23,2^24)
    }
    if (zeros >= 255)
        return (uint32_t)0x00800000u;
    return ((uint32_t)bits64) + ((~(uint32_t)zeros) << 24);
    /* return ((~(uint32_t)zeros)<<24); */
}
uint32_t compute_val(const std::array<uint8_t, 76>& header, uint32_t offset, TargetV2 t)
{
    std::array<uint8_t, 80> arr;
    std::copy(header.begin(), header.end(), arr.begin());
    static_assert(sizeof(offset) == 4);
    memcpy(arr.begin() + 76, &offset, 4);
    const auto h1 { hashSHA256(arr) };
    const auto h2 { hashSHA256(h1) };
    const auto h3 { hashSHA256(h2) };
    std::array<uint32_t, 8> hash;
    memcpy(hash.data(), h3.data(), 32);
    return compute_targetv1_invzeros(t.zeros10(), t.bits22(), hash);
}

void verify_targets(const std::vector<uint32_t>& v, uint32_t offset, const std::array<uint8_t, 76>& header, TargetV2 t)
{
    assert(v.size() < std::numeric_limits<uint32_t>::max());
    for (uint32_t i = 0; i < v.size(); ++i) {
        uint32_t index = htobe32(offset + i);
        assert(compute_val(header, index, t) == v[i]);
    }
}

void DeviceWorker::mine_triple_sha()
{
    using namespace std::chrono;
    if (!lastHashrateCheckpoint.has_value()) {
        lastHashrateCheckpoint = steady_clock::now();
        getHashrateCheckpoint = steady_clock::now();
    }

    if (hashesTried == std::numeric_limits<uint32_t>::max()) {
        init_mining();
    }
    using namespace std::chrono;


    // TODO
    auto nHashes { std::min(std::numeric_limits<uint32_t>::max() - hashesTried, hashesPerStep) };
    uint32_t offset = hashesTried + randOffset;

    auto [targets] = miner.mine_triple_sha(nHashes, offset);
    hashesTried += nHashes;
    hashCounter += nHashes;

    // verify_targets(targets, offset, currentTask->get_header(), tgt);
    // return;

    // save_targets(targets);
    assert(targets.size() == hashesPerStep);
    assert(targets.size() != 0);
    targets.resize(nHashes);
    auto& t { currentTask.value() };


    // time hash computation
    // scale to achieve 100ms
    auto now {steady_clock::now()};
    auto delta { steady_clock::now() - *lastHashrateCheckpoint };
    if (hashesPerStep == nHashes) {
        auto ms { duration_cast<milliseconds>(delta).count() };
        trace_log().debug("[SHA256T/MINED] {} in {}ms", nHashes, ms);
        hashesPerStep = ((nHashes*100ms)/delta);
        assert(hashesPerStep != 0);
        hashesPerStep = miner.set_triple_sha_batchsize(hashesPerStep);
    }
    lastHashrateCheckpoint = now;

    pool.notify_mined_triple_sha(TripleSha::MinedValues {
        .duration { delta },
        .invertedTargets { targets },
        .target{miner.get_job().t},
        .deviceId = deviceId,
        .offset = offset,
        .block { t.block },
        .cleanIndex = t.cleanIndex });
};

void DeviceWorker::handle_event(const Stop&)
{
    currentTask = {};
}

void DeviceWorker::handle_event(SetTask&& e)
{
    currentTask = e.task;
    init_mining();
}

void DeviceWorker::run()
{
    auto stop_token { thread.get_stop_token() };
    while (true) {
        decltype(events) tmpEvents;
        {
            std::unique_lock l(m);
            cv.wait(l, [&]() { return stop_token.stop_requested() || wakeup; });
            wakeup = false;
            tmpEvents.swap(events);
        }

        // handle events
        for (auto& e : tmpEvents) {
            std::visit([&](auto&& e) { handle_event(std::move(e)); }, std::move(e));
        }

        wakeup = try_mine();

        if (stop_token.stop_requested()) {
            return;
        }
    }
}

void DeviceWorker::push_event(Event e)
{
    std::lock_guard l(m);
    events.push_back(std::move(e));
    cv.notify_all();
    wakeup = true;
};
