#pragma once
#include <cassert>
#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <string>

struct FormattedHashrate {
    FormattedHashrate(size_t hr)
    {
        using namespace std::literals;
        if (hr > 1000000000) {
            val = double(hr) / 1000000000;
            unit = "gh"sv;
        } else if (hr > 1000000) {
            val = double(hr) / 1000000;
            unit = "mh"sv;
        } else if (hr > 1000) {
            val = double(hr) / 1000;
            unit = "kh"sv;
        } else {
            val = hr;
            unit = "h"sv;
        }
    }
    double val;
    std::string_view unit;
    std::string to_string() const
    {
        return std::to_string(val) + std::string(" ") + std::string(unit);
    }
};

struct Hashrate {
    using Duration = std::chrono::steady_clock::duration;
    Hashrate(size_t N, Duration d = std::chrono::seconds(1))
        : val(N * std::chrono::seconds(1) / d)
    {
    }

    double hash_per(Duration d) const
    {
        return val * d / std::chrono::seconds(1);
    }

    auto format() const { return FormattedHashrate(val); }
    size_t val;
    operator double() const { return val; }
};

class HashrateWatcher {
public:
    Hashrate hashrate()
    {
        prune();
        using namespace std::chrono;
        if (snapshots.size() < 2) {
            assert(totalWithoutFirst == 0);
            return { 0 };
        }
        return { totalWithoutFirst, steady_clock::now() - snapshots.front().t };
    }
    [[nodiscard]] std::optional<std::chrono::steady_clock::duration> register_hashes(size_t n)
    {
        std::optional<std::chrono::steady_clock::duration> res;
        auto now { std::chrono::steady_clock::now() };
        if (snapshots.size() > 0) {
            totalWithoutFirst += n;
            res = now - snapshots.back().t;
        }
        snapshots.push_back({ now, n });
        return res;
    }
    void reset(){
        *this = {};
    }

    void prune()
    {
        using namespace std::chrono;
        auto discardBefore { steady_clock::now() - 2s };
        while (snapshots.size() > 0 && snapshots.front().t < discardBefore) {
            snapshots.pop_front();
            if (snapshots.size() > 0)
                totalWithoutFirst -= snapshots.front().n;
        }
    }

private:
    struct Snapshot {
        std::chrono::steady_clock::time_point t;
        size_t n;
    };
    std::deque<Snapshot> snapshots;
    size_t totalWithoutFirst { 0 };
};
