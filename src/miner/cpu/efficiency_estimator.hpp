#pragma once
#include "cpu/verus_job.hpp"
#include "hashrate.hpp"
#include "spdlog/spdlog.h"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>

struct PerSecond {
    Hashrate total { 0 };
    Hashrate considered { 0 };
};

// Xb = 1
// X = (total/sec considered/sec)
// b = (plain s/try)
//     (extra s/try)
//
// (X'X)^(-1) X'1
// (X'X) = (A B)
//         (B D)
// (X'X)^(-1) = 1/(AD-BB) (D -B)
//                        (-B A)

struct VerusTiming {
    [[nodiscard]] std::optional<PerSecond> per_second() const
    {
        using namespace std::chrono;
        size_t us = duration_cast<microseconds>(duration).count();
        if (us == 0)
            return {};
        if (processed < 100) { // TODO: remove
            return {};
        }
        return PerSecond {
            {(total * 1000 * 1000) / us},
                {(processed * 1000 * 1000) / us}
        };
    }

    using tp = std::chrono::steady_clock::time_point;
    std::chrono::steady_clock::duration duration;
    size_t total;
    size_t processed;
    double proportionOfRequested;
    Verus::MineThreshold threshold;
};

class AverageEstimator {
public:
    void push_hashrate(size_t perSecond)
    {
        vals.push_back(perSecond);
        total += perSecond;
        while (size() > windowWidth) {
            drop();
        }
    }
    std::optional<Hashrate> average_hashrate() const
    {
        if (total == 0)
            return {};
        return Hashrate{total / size()};
    }
    size_t size() const { return vals.size(); }

private:
    void drop()
    {
        assert(size() > 0);
        total -= vals.front();
        vals.pop_front();
    }
    size_t total { 0 };
    size_t windowWidth = 10;
    std::deque<size_t> vals;
};

class EfficiencyEstimator {
public:
    struct Estimate {
        double scan;
        double process;
    };
    [[nodiscard]] size_t size() const { return d.size(); }
    void push(const PerSecond& ps)
    {
        d.push_back(ps);
        const auto tt { ps.total.val };
        const auto cs { ps.considered.val };
        sumtt += tt;
        sumcs += cs;
        A += tt * tt;
        B += cs * tt;
        D += cs * cs;
        if (size() > maxSize) {
            drop_front();
        }
    };
    void update_last(const PerSecond& ps)
    {
        drop_back();
        d.push_back(ps);
        const auto tt { ps.total.val };
        const auto cs { ps.considered.val };
        sumtt += tt;
        sumcs += cs;
        A += tt * tt;
        B += cs * tt;
        D += cs * cs;
        if (size() > maxSize) {
            drop_front();
        }
    };
    std::optional<Estimate> estimate()
    {
        if (d.size() < 2)
            return {};
        double det = double(A) * double(D) - double(B) * double(B);
        if (det <= 0)
            return {};
        double invdet = 1 / double(det);
        return Estimate {
            .scan = invdet * (D * sumtt - B * sumcs),
            .process = invdet * (-B * sumtt + A * sumcs)
        };
    }

private:
    void drop_back()
    {
        assert(size() > 0);
        subtract(d.back());
        d.pop_back();
    }
    void drop_front()
    {
        assert(size() > 0);
        subtract(d.front());
        d.pop_front();
    }
    void subtract(const PerSecond& ps)
    {
        const auto tt { ps.total.val };
        const auto cs { ps.considered.val };
        sumtt -= tt;
        sumcs -= cs;
        A -= tt * tt;
        B -= cs * tt;
        D -= cs * cs;
    }

    uint64_t sumcs { 0 };
    uint64_t sumtt { 0 };
    uint64_t A { 0 };
    uint64_t B { 0 };
    uint64_t D { 0 };
    size_t maxSize = 10;
    std::deque<PerSecond> d;
};
