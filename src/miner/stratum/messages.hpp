#pragma once
#include "nlohmann/json.hpp"
#include <optional>
#include <string>
#include <variant>

namespace stratum {


// methods for inbound messages
struct Result {
    struct Error {
        int code;
        std::string message;
    };
    using ResultArgs = nlohmann::json;
    int64_t id;
    std::variant<Error, ResultArgs> val;
    const ResultArgs& get_result_args() const { return std::get<ResultArgs>(val); }
    const Error& get_error() const { return std::get<Error>(val); }
    bool is_error() const { return std::holds_alternative<Error>(val); }
};

struct Mining_SetDifficulty {
    double difficulty;
};

struct Mining_Notify {
    std::string jobId;
    std::array<uint8_t,32> prevHash;
    std::vector<uint8_t> merklePrefix;
    std::array<uint8_t,4> nversion;
    std::array<uint8_t,4> nbits;
    std::array<uint8_t,4> ntime;
    bool clean;
};

struct Submission{
    std::string jobId;
    std::vector<uint8_t> extra2;
    std::array<uint8_t,4> time;
    std::array<uint8_t,4> nonce;
};

// methods for outbound messages
std::string subscribe(int32_t id, std::string miner);
std::string authorize(int32_t id, std::string user, std::string pass);
std::string submit(int32_t id, const Submission& s);

using Message = std::variant<Result, Mining_Notify, Mining_SetDifficulty>;
struct ConnectionStart {
    size_t connectionId;
};

struct Subscription {
    std::vector<uint8_t> extranonce1;
    uint32_t extranonce2size;
};

struct ConnectionEnd {
    size_t connectionId;
};

using Event = std::variant<Mining_Notify,Mining_SetDifficulty, ConnectionStart, Subscription, ConnectionEnd>;

std::optional<Message> parse_line(std::string line);
}
