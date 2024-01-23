#include "device_pool.hpp"
#include "block/header/difficulty.hpp"
#include "block/header/header_impl.hpp"
#include "cpu/verus_worker.hpp"
#include "helpers.hpp"

std::optional<StratumJobGenerator> StratumConnectionData::get_job()
{
    if (!jobdata._difficulty || !_notify || !_subscription)
        return {};
    return StratumJobGenerator { jobdata, *_notify, *_subscription };
}

std::optional<StratumJobGenerator> StratumConnectionData::update(const stratum::Mining_Notify& n)
{
    if (n.clean) {
        jobdata.cleanId++;
    }
    jobdata._extranonce = std::make_shared<std::atomic<uint32_t>>(0);
    _notify = n;
    return get_job();
}

std::optional<StratumJobGenerator> StratumConnectionData::update(const stratum::Mining_SetDifficulty& d)
{
    if (jobdata._difficulty) {
        *jobdata._difficulty = d.difficulty;
    } else {
        jobdata._difficulty = std::make_shared<std::atomic<double>>(d.difficulty);
        return get_job();
    }
    return {};
}

void StratumConnectionData::update(const stratum::Subscription& s)
{
    _subscription = s;
}

void DevicePool::notify_mined_triple_sha(TripleSha::MinedValues j)
{
    push_event(std::move(j));
}

void DevicePool::init_connection(const stratum::ConnectionData& cd)
{
    spdlog::info("Stratum mode enabled, host is {}:{}", cd.host, cd.port);
    stratumConnection = std::make_unique<stratum::ConnectionServer>(
        cd,
        [&](stratum::Event&& e) {
            std::visit([&](auto&& e) {
                handle_event(std::move(e));
            },
                std::move(e));
        });
};
void DevicePool::init_connection(const NodeConnectionData& cd)
{
    spdlog::info("Node RPC is {}:{}", cd.host, cd.port);
    address = cd.address;
    api = std::make_unique<API>(cd.host, cd.port);
};
DevicePool::DevicePool(const std::vector<CL::Device>& devices, size_t nVerusWorkers, std::variant<stratum::ConnectionData, NodeConnectionData> connectionData)
    : verusPool(*this, st, nVerusWorkers)
{
    std::visit([&](auto& d) { init_connection(d); }, connectionData);

    for (size_t i = 0; i < devices.size(); ++i) {
        auto& d { devices[i] };
        workers.push_back(std::make_unique<DeviceWorker>(i, d, *this));
    }
}

Hashrate janusscore(uint64_t verus, uint64_t sha256t)
{
    if (verus > sha256t || verus == 0) {
        return 0.0;
    }
    double c { 0.005 };
    return double(sha256t) * (pow(c + double(verus) / double(sha256t), 0.3) - pow(c, 0.3)) / (pow(c + 1, 0.3) - pow(c, 0.3));
}

void DevicePool::print_hashrate()
{
    std::vector<std::pair<std::string, uint64_t>> hashrates;
    uint64_t sum { 0 };
    uint64_t sumSha256t { 0 };
    for (auto& dt : workers) {
        auto hashrate = dt->get_hashrate();
        sum += hashrate;
        sumSha256t += hashrate;
        hashrates.push_back({ dt->deviceName, hashrate });
    }

    auto [sumVerus, verusHashrates] = verusPool.hashrates();

    // std::string durationstr;
    // if (task.has_value()) {
    //     uint32_t seconds(task->header.target_v1().difficulty() / sum);
    // durationstr = spdlog::fmt_lib::format("(~{} per block)", format_duration(seconds));
    // }
    spdlog::info("Total hashrate (GPU): {}/s", Hashrate(sumSha256t).format().to_string());

    for (auto& [name, hr] : hashrates) {
        spdlog::info("   {}: {}/s", name, Hashrate(hr).format().to_string());
    }
    spdlog::info("Total hashrate (CPU): {}/s", sumVerus.format().to_string());
    for (size_t i = 0; i < verusHashrates.size(); ++i) {
        spdlog::info("   Thread#{}: {}/s", i, verusHashrates[i].format().to_string());
    }
    spdlog::info("Janusscore: {}/s", janusscore(sumVerus.val, sumSha256t).format().to_string());
}

void DevicePool::handle_event(const WorkerResult& wr)
{
    verusPool.push_job(Verus::PoolJob { .mined { std::move(wr) } });
}

void DevicePool::handle_event(OnJanusMined&& e)
{
    std::visit([&](auto&& submission) { submit(std::move(submission)); }, std::move(e.submission));
}

void DevicePool::handle_event(StratumNotify&& n)
{
    if (auto j { stratumConnectionData.update(n) }; j.has_value())
        assign_work(std::move(*j));
}

void DevicePool::handle_event(StratumSetDiff&& sd)
{
    if (auto j { stratumConnectionData.update(sd) }; j.has_value())
        assign_work(std::move(*j));
}

void DevicePool::handle_event(stratum::ConnectionStart&& cs)
{
    stratumConnectionData.clear(cs.connectionId);
}

void DevicePool::handle_event(stratum::ConnectionEnd&&)
{
    stratumConnectionData.clear();
}

void DevicePool::handle_event(stratum::Subscription&& s)
{
    stratumConnectionData.update(std::move(s));
};

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
    static size_t rejected { 0 };
    static size_t accepted { 0 };
    if (!jobStatus.valid_block(b)) {
        spdlog::warn("Found outdated block :(");
        return;
    }
    spdlog::info("#{} Submitting mined block at height {}...", ++minedcount, b.height.value());
    assert(api);
    auto res { api->submit_block(b) };
    if (res.second == 0) {
        accepted += 1;
        spdlog::info("💰 #{} Mined block {}, (total accepted {}, rejected {})", minedcount, b.height.value(), accepted, rejected);
        // exit(0);
    } else {
        rejected += 1;
        spdlog::warn("⚠  #{} Mined block {} rejected: {}, (total accepted {}, rejected {})", minedcount, b.height.value(), res.first, accepted, rejected);
    }
    needsPoll = true;
};

void DevicePool::submit(const stratum::Submission& s)
{
    assert(stratumConnection);
    stratumConnection->submit(s, stratumConnectionData.get_connection_id());
}

void DevicePool::poll()
{
    if (!api)
        return;
    needsPoll = false;
    nextPoll = std::chrono::steady_clock::now() + pollInterval;
    auto block = api->get_mining(address.value());
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

void DevicePool::assign_work(StratumJobGenerator&& sj)
{
    if (sj.is_clean()) {
        verusPool.clean(sj.target());
    }
    for (auto& w : workers)
        w->set_job(sj);
};

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
