#pragma once
#include "block/header/difficulty.hpp"
#include "block/header/header.hpp"
#include "crypto/hash.hpp"
#include "messages.hpp"
#include <atomic>
#include <span>

template <typename T, size_t N>
std::array<std::remove_cv_t<T>, N> from_span(const std::span<T, N> s)
{

    std::array<std::remove_cv_t<T>, N> out;
    std::copy(s.begin(), s.end(), out.begin());
    return out;
}
struct StratumJobDataBase {
    std::shared_ptr<std::atomic<double>> _difficulty;
    std::shared_ptr<std::atomic<uint32_t>> _extranonce;
    size_t cleanId { 0 };
    size_t connectionId { 0 };
};

struct StratumJobData : public StratumJobDataBase {
    stratum::Mining_Notify _notify;
    stratum::Subscription subscription;
};

class StratumConnectionData;
class StratumJob;
class StratumJobGenerator {
    friend class StratumConnectionData;
    std::shared_ptr<StratumJobData> data;

protected:
    StratumJobGenerator(StratumJobDataBase base, stratum::Mining_Notify n, stratum::Subscription s)
        : data { std::make_shared<StratumJobData>(base, n, s) }
    {
    }

    Hash gen_merkle_root(std::vector<uint8_t>) const;

public:
    StratumJob generate_job() const;
    std::shared_ptr<std::atomic<double>> get_difficulty() const
    {
        return data->_difficulty;
    }
    auto& job_id() const { return data->_notify.jobId; }
    std::vector<uint8_t> gen_extra2() const;
    bool is_clean() const { return data->_notify.clean; }
    std::array<uint8_t, 76> gen_header(const std::vector<uint8_t>&) const;
    TargetV2 target() const
    {
        return double(*data->_difficulty);
    }
};

class StratumJob {
    std::string _jobId;
    TargetV2 _target;
    std::shared_ptr<std::atomic<double>> _difficulty;
    std::vector<uint8_t> _extra2;
    std::array<uint8_t, 76> _header;

public:
    auto& get_job_id() const { return _jobId; }
    double difficulty() const { return *_difficulty; }
    auto extra2() const { return _extra2; }
    auto target() const { return _target; }
    auto header() const { return _header; }
    stratum::Submission submission(std::span<const uint8_t, 4> nonce) const
    {
        std::array<uint8_t, 4> time;
        memcpy(&time, _header.data() + HeaderView::offset_timestamp, 4);
        return {
            .jobId { _jobId },
            .extra2 { _extra2 },
            .time { time },
            .nonce { from_span(nonce) }
        };
    }

    StratumJob(const StratumJobGenerator& stratumJob)
        : _jobId(stratumJob.job_id())
        , _target(stratumJob.target())
        , _difficulty(stratumJob.get_difficulty())
        , _extra2(stratumJob.gen_extra2())
        , _header(stratumJob.gen_header(_extra2))
    {
    }
};
