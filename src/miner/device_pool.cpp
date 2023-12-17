#include "device_pool.hpp"
#include "block/header/difficulty.hpp"
#include "block/header/header_impl.hpp"
#include "cpu/verus_worker.hpp"
#include "helpers.hpp"

// std::optional<Hashrate> HashrateEstimator::push(const TripleSha::MinedValues& v)
// {
//     size_t nHashes { v.invertedTargets.size() };
//     auto [iter, inserted] = hashesPerDevice.try_emplace(v.deviceId);
//     if (inserted) { // don't add first time to measure time span
//         iter->second.add(nHashes);
//     }
//     hashesPerDevice[v.deviceId].add(nHashes);
//     return hashes_per_second();
// }
//
// std::optional<Hashrate> HashrateEstimator::hashes_per_second()
// {
//     using namespace std::chrono;
//     if (!wakeup.has_value()) {
//         wakeup = steady_clock::now() + seconds(1);
//         return {};
//     }
//     if (*wakeup > steady_clock::now()) {
//         return {};
//     }
//     wakeup = steady_clock::now() + seconds(1);
//     size_t hps { 0 };
//     for (auto& [_, node] : hashesPerDevice) {
//         hps += node.hash_per_second();
//         node.finalize();
//     }
//     return Hashrate(hps);
// }

void DevicePool::notify_mined_triple_sha(TripleSha::MinedValues j)
{
    push_event(std::move(j));
}
DevicePool::DevicePool(const Address& address, const std::vector<CL::Device>& devices, std::string host, uint16_t port, size_t nVerusWorkers)
    : verusPool(*this, st,nVerusWorkers)
    , address(address)
    , api(host, port)
{
    for (size_t i = 0; i < devices.size(); ++i) {
        auto& d { devices[i] };
        workers.push_back(std::make_unique<DeviceWorker>(i, d, *this));
    }
    for (size_t i = 0; i < nVerusWorkers; ++i) {
        // verusWorkers.push_back(std::make_unique<Verus::Worker>(*this));
    }
}

void DevicePool::print_hashrate()
{
    std::vector<std::pair<std::string, uint64_t>> hashrates;
    uint64_t sum { 0 };
    for (auto& dt : workers) {
        auto hashrate = dt->get_hashrate();
        sum += hashrate;
        hashrates.push_back({ dt->deviceName, hashrate });
    }

    auto [sumVerus,verusHashrates] = verusPool.hashrates();

    // std::string durationstr;
    // if (task.has_value()) {
    //     uint32_t seconds(task->header.target_v1().difficulty() / sum);
        // durationstr = spdlog::fmt_lib::format("(~{} per block)", format_duration(seconds));
    // }
    spdlog::info("Total hashrate (GPU): {}/s", Hashrate(sum).format().to_string());

    for (auto& [name, hr] : hashrates) {
        spdlog::info("   {}: {}/s", name, Hashrate(hr).format().to_string());
    }
    spdlog::info("Total hashrate (CPU): {}/s", sumVerus.format().to_string());
    for (size_t i=0; i< verusHashrates.size(); ++i){
        spdlog::info("   Thread#{}: {}/s", i, verusHashrates[i].format().to_string());
    }
}

void DevicePool::handle_event(const WorkerResult& wr)
{
    verusPool.push_job(Verus::PoolJob { .mined { std::move(wr) } });
}

void DevicePool::handle_event(const DoSubmit& e)
{
    auto& b { e.b };
    if (!task || b.height != task.value().height)
        return;
    submit(b);
}

void DevicePool::handle_event(OnJanusMined&& e)
{
    if (!jobStatus.valid_block(e.b)) {
        spdlog::warn("Found outdated block :(");
        return;
    }
    submit(e.b);
}

void DevicePool::set_ignore_below() {
    // uint32_t ignoreBelow { bisecter->get_candidate() };
    // rateWatcher.reset();
    // verusTunedCounter = 0;
    // rateIndex += 1;
    // for (auto& w : verusWorkers) {
    //     w->set_threshold(ignoreBelow, rateIndex);
    // }
};

void DevicePool::submit(const Block& b)
{
    spdlog::info("#{} Submitting mined block at height {}...", ++minedcount, b.height.value());
    auto res { api.submit_block(b) };
    if (res.second == 0) {
        spdlog::info("💰 #{} Mined block {}.", minedcount, b.height.value());
        // exit(0);
    } else {
        spdlog::warn("⚠  #{} Mined block {} rejected: {}", minedcount, b.height.value(), res.first);
    }
    needsPoll = true;
};

void DevicePool::poll()
{
    needsPoll = false;
    nextPoll = std::chrono::steady_clock::now() + pollInterval;
    auto block = api.get_mining(address);
    if (block)
        assign_work(*block);
    else
        stop_mining();
}

void DevicePool::assign_work(const Block& b)
{
    bool clean = jobStatus.push_block(b);
    if (clean) {
        verusPool.clean(b.header.target_v2());
    }
    MineJob mj {
        .block { b },
        .t { b.header.target(b.height) },
        .cleanIndex = verusPool.get_clean_index(),
    };
    if (task.has_value() && b.header == task->header)
        return;
    blockSeed = randuint32();
    task = b;
    for (auto& w : workers)
        w->set_job(mj);
}

void DevicePool::stop_mining()
{
    for (auto& w : workers)
        w->stop_mining();
    verusPool.stop_mining();
};

void DevicePool::run()
{
    using namespace std::literals::chrono_literals;
    using namespace std::chrono;

    auto nextPrint = steady_clock::now() + printInterval;
    nextPoll = steady_clock::now() + pollInterval;
    while (true) {
        bool verusWakeup { false };
        decltype(events) tmpEvents;
        {
            std::unique_lock ul(st.m);
            while (true) {
                if (shutdown)
                    return;

                bool handle_stuff { false };
                if (steady_clock::now() >= nextPrint) {
                    print_hashrate();
                    nextPrint = steady_clock::now() + printInterval;
                }
                if (steady_clock::now() >= nextPoll) {
                    needsPoll = true;
                    handle_stuff = true;
                }
                if (events.size() > 0) {
                    tmpEvents.swap(events);
                    handle_stuff = true;
                }
                if (verusPool.locked_needs_wakeup()) {
                    verusWakeup = true;
                    handle_stuff = true;
                }
                if (handle_stuff)
                    break;
                st.cv.wait_until(ul, std::min(nextPrint, nextPoll));
            }
        }
        for (auto& e : tmpEvents) {
            std::visit([&](auto&& e) {
                handle_event(std::move(e));
            },
                std::move(e));
        }
        if (verusWakeup) {
            verusPool.process();
        }
        if (needsPoll)
            poll();
    }
    std::cout << "End" << std::endl;
}
