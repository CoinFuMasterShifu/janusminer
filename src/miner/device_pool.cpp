#include "device_pool.hpp"
#include "block/header/difficulty.hpp"
#include "block/header/header_impl.hpp"
#include "cpu/verus_worker.hpp"
#include "helpers.hpp"
#include "logs.hpp"

std::optional<StratumGeneratorArgs> StratumConnectionData::generator_args()
{
    if (!jobdata._difficulty || !_notify || !_subscription)
        return {};
    return StratumGeneratorArgs { jobdata, *_notify, *_subscription };
}

std::optional<StratumGeneratorArgs> StratumConnectionData::update(const stratum::Mining_Notify& n)
{
    if (n.clean) {
        jobdata.cleanIndex++;
    }
    jobdata._extranonce = std::make_shared<std::atomic<uint32_t>>(0);
    _notify = n;
    return generator_args();
}

std::optional<StratumGeneratorArgs> StratumConnectionData::update(const stratum::Mining_SetDifficulty& d)
{
    if (jobdata._difficulty) {
        *jobdata._difficulty = d.difficulty;
    } else {
        jobdata._difficulty = std::make_shared<std::atomic<double>>(d.difficulty);
        return generator_args();
    }
    return {};
}

void StratumConnectionData::update(const stratum::Subscription& s)
{
    _subscription = s;
}

void MiningCoordinator::notify_mined_triple_sha(TripleSha::MinedValues j)
{
    push_event(std::move(j));
}

void MiningCoordinator::init_connection(const stratum::ConnectionData& cd)
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
void MiningCoordinator::init_connection(const NodeConnectionData& cd)
{
    spdlog::info("Node RPC is {}:{}", cd.host, cd.port);
    address = cd.address;
    api = std::make_unique<API>(cd.host, cd.port);
};
MiningCoordinator::MiningCoordinator(const std::vector<CL::Device>& devices, size_t nVerusThreads, const ConnectionArg& connectionArg)
    : sha256tHasher(
        devices, [&](TripleSha::MinedValues mined) {
            push_event(std::move(mined));
        },
        connectionArg.queuesize_gb())
    , verusPool(*this, st, nVerusThreads)
{
    std::visit([&](auto& d) { init_connection(d); }, connectionArg);
}

Hashrate janusscore(uint64_t verus, uint64_t sha256t)
{
    if (verus > sha256t || verus == 0) {
        return 0.0;
    }
    double c { double(C_CONSTANT) / double(0xFFFFFFFF) };
    return (10.0 / 3.0) * double(sha256t) * (pow(c + double(verus) / double(sha256t), 0.3) - pow(c, 0.3));
}

void MiningCoordinator::print_hashrate()
{
    auto [sumSha256t, sha256tHashrates] = sha256tHasher.hashrates();

    auto [sumVerus, verusHashrates] = verusPool.hashrates();

    // std::string durationstr;
    // if (task.has_value()) {
    //     uint32_t seconds(task->header.target_v1().difficulty() / sum);
    // durationstr = spdlog::fmt_lib::format("(~{} per block)", format_duration(seconds));
    // }
    spdlog::info("Total hashrate (GPU): {}/s", Hashrate(sumSha256t).format().to_string());

    for (auto& [index, name, hr] : sha256tHashrates) {
        spdlog::info("   [{}] {}: {}/s", index, name, Hashrate(hr).format().to_string());
    }
    spdlog::info("Total hashrate (CPU): {}/s", sumVerus.format().to_string());
    for (size_t i = 0; i < verusHashrates.size(); ++i) {
        spdlog::info("   Thread#{}: {}/s", i, verusHashrates[i].format().to_string());
    }
    spdlog::info("Janusscore: {}/s", janusscore(sumVerus.val, sumSha256t).format().to_string());
}

void MiningCoordinator::handle_event(const WorkerResult& wr)
{
    verusPool.push_job(Verus::QueuedJob { .mined { std::move(wr) } });
}

void MiningCoordinator::handle_event(OnJanusMined&& e)
{
    std::visit([&](auto&& submission) { submit(std::move(submission)); }, std::move(e.submission));
}

void MiningCoordinator::handle_event(StratumNotify&& n)
{
    if (auto j { stratumConnectionData.update(n) }; j.has_value())
        assign_work(*j);
}

void MiningCoordinator::handle_event(StratumSetDiff&& sd)
{
    if (auto j { stratumConnectionData.update(sd) }; j.has_value())
        assign_work(*j);
}

void MiningCoordinator::handle_event(stratum::ConnectionStart&& cs)
{
    stratumConnectionData.clear(cs.connectionId);
}

void MiningCoordinator::handle_event(stratum::ConnectionEnd&&)
{
    stratumConnectionData.clear();
}

void MiningCoordinator::handle_event(stratum::Subscription&& s)
{
    stratumConnectionData.update(std::move(s));
};

void MiningCoordinator::set_ignore_below() {
    // uint32_t ignoreBelow { bisecter->get_candidate() };
    // rateWatcher.reset();
    // verusTunedCounter = 0;
    // rateIndex += 1;
    // for (auto& w : verusWorkers) {
    //     w->set_threshold(ignoreBelow, rateIndex);
    // }
};

void MiningCoordinator::submit(const Block& b)
{
    static size_t rejected { 0 };
    static size_t accepted { 0 };
    if (b.header.prevhash() != prevHash) {
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

void MiningCoordinator::submit(const stratum::Submission& s)
{
    assert(stratumConnection);
    stratumConnection->submit(s, stratumConnectionData.get_connection_id());
    stratum_mining_log->info("Submitting to pool");
}

void MiningCoordinator::poll()
{
    if (!api)
        return;
    needsPoll = false;
    nextPoll = std::chrono::steady_clock::now() + pollInterval;
    auto p = api->get_mining(address.value());
    if (p) {
        auto& [block, testnet] = *p;
        assign_work(block, testnet);
    } else
        stop_mining();
}

void MiningCoordinator::assign_work(const Block& b, bool testnet)
{
    bool clean = false;
    auto ph { b.header.prevhash() };
    if (prevHash != ph) {
        prevHash = ph;
        clean = true;
        ++cleanIndex;
    }
    if (clean) {
        verusPool.clean(cleanIndex);
    }
    MineJob mj {
        .block { b },
        .t { b.header.target_v2() },
        .cleanIndex = cleanIndex,
        .testnet = testnet
    };
    if (clean) {
        spdlog::info("Difficulty {}", mj.t.difficulty());
    }
    if (currentHeader == b.header)
        return;
    currentHeader = b.header;

    sha256tHasher.set_work(mj);
}

void MiningCoordinator::assign_work(StratumGeneratorArgs& sj)
{
    if (sj.n.clean) {
        cleanIndex = sj.base.cleanIndex;
        verusPool.clean(cleanIndex);
    }
    sha256tHasher.set_work(sj);
}

void MiningCoordinator::stop_mining()
{
    sha256tHasher.stop_mining();
    verusPool.stop_mining();
};

void MiningCoordinator::run()
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

void MiningCoordinator::set_hashrate(Hashrate hashrate)
{
    sha256tHasher.update_verushashrate(hashrate);
};
