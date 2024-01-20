#include "messages.hpp"
#include "general/hex.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

namespace {
std::string message(int32_t id, std::string_view method, nlohmann::json::array_t params)
{
    return nlohmann::json {
        { "id", id },
        { "method", method },
        { "params", params }
    }
        .dump();
}
template <typename... T>
std::string message(int32_t id, std::string_view method, T&&... args)
{
    return message(id, method, nlohmann::json::array_t { std::forward<T>(args)... });
}
}

namespace stratum {

std::string subscribe(int32_t id, std::string miner)
{
    return message(id, "mining.subscribe", miner);
}
std::string authorize(int32_t id, std::string user, std::string pass)
{
    return message(id, "mining.authorize", user, pass);
}

std::string submit(int32_t id, const Submission& s)
{
    const std::string& jobId { s.jobId };
    std::string extra2hex { serialize_hex(s.extra2) };
    std::string timehex { serialize_hex(s.time) };
    std::string noncehex { serialize_hex(s.nonce) };
    return message(id, "mining.submit", jobId, extra2hex, timehex, noncehex);
}

std::optional<Message> parse_line(std::string line)
{
    std::cout << "Line: " << line << std::endl;
    try {
        const auto parsed = nlohmann::json::parse(line);
        auto iter = parsed.find("result");
        if (iter != parsed.end()) { // result message
            auto a = parsed.at("error");
            auto id { parsed.at("id").get<int64_t>() };
            std::optional<Result::Error> e;
            if (!a.is_null()) {
                if (!a.is_array() || a.size() != 3)
                    return {};
                return Result {
                    .id = id,
                    .val {
                        Result::Error {
                            .code = a.at(0).get<int>(),
                            .message { a.at(1).get<std::string>() } } }
                };
            } else {
                return Result {
                    .id = id,
                    .val { *iter }
                };
            }
        } else {
            if (!parsed.at("id").is_null())
                return {};
            std::string method { parsed.at("method").get<std::string>() };
            auto params(parsed.at("params"));
            if (method == "mining.set_difficulty") {
                using namespace std;
                // cout<<"params is_array"<<params.is_array()<<endl;
                // cout<<"params size"<<params.size()<<endl;
                // cout<<"params[0] is_array"<<params[0].is_array()<<endl;
                // cout<<"params[0] size"<<params[0].size()<<endl;
                // cout<<"params[0][0] is_array"<<params[0][0].is_array()<<endl;
                // cout<<"params[0][0] is_number"<<params[0][0].is_number()<<endl;
                return Mining_SetDifficulty {
                    .difficulty = params.at(0).get<double>()
                };
            } else if (method == "mining.notify") {
                return Mining_Notify {
                    .jobId { params.at(0).get<std::string>() },
                    .prevHash { hex_to_arr<32>(params.at(1).get<std::string>()) },
                    .merklePrefix { hex_to_vec(params.at(2).get<std::string>()) },
                    .nversion{ hex_to_arr<4>(params.at(3).get<std::string>())},
                    .nbits { hex_to_arr<4>(params.at(4).get<std::string>())},
                    .ntime { hex_to_arr<4>(params.at(5).get<std::string>())},
                    .clean = params.at(6).get<bool>(),
                };
            }
        }
    } catch (...) {
    }
    return {};
}
}
