#pragma once
#include <chrono>
#include <cstddef>
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
    Hashrate(size_t N, Duration d=std::chrono::seconds(1))
        :val(N*std::chrono::seconds(1)/d)
    {
    }

    double hash_per(Duration d) const{
        return val*d/std::chrono::seconds(1);
    }

    auto format() const { return FormattedHashrate(val); }
    size_t val;
    operator double() const { return val; }
};
