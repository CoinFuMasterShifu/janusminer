#include "api_call.hpp"
#include "cmdline/cmdline.h"
#include "crypto/address.hpp"
#include "crypto/verushash/verushash.hpp"
#include "device_pool.hpp"
#include "general/hex.hpp"
#include "log/trace.hpp"
#include "spdlog/spdlog.h"
#include "stratum/connection.hpp"
#include <iostream>
#include <variant>
using namespace std;

int start_miner(std::string gpus, size_t threads, std::variant<stratum::ConnectionData, NodeConnectionData> connectionData);

int test_gpu_miner2();

int process(gengetopt_args_info& ai)
{
    try {
        std::string host { ai.host_arg };
        uint16_t port(ai.port_arg);
        if (ai.threads_arg < 0)
            throw std::runtime_error("Illegal value " + to_string(ai.threads_arg) + " for option --threads.");
        size_t threads { ai.threads_arg == 0 ? std::thread::hardware_concurrency() : ai.threads_arg };
        // Address address(ai.address_arg);
        std::string gpus;
        if (ai.gpus_given) {
            gpus.assign(ai.gpus_arg);
        }
        if (ai.address_given && strlen(ai.address_arg) > 0) {
            if (ai.user_given && strlen(ai.user_arg) > 0)
                spdlog::warn("Stratum parameter '-u' is ignored because direct-to-node mining is enabled via '-a'");
            start_miner(gpus, threads,
                NodeConnectionData {
                    .host { host },
                    .port = port,
                    .address { ai.address_arg } });
        } else { // stratum
            start_miner(gpus, threads,
                stratum::ConnectionData {
                    .host { ai.host_arg },
                    .port { std::to_string(ai.port_arg) },
                    .user { ai.user_arg },
                    .pass { ai.password_arg } });
        }
    } catch (std::runtime_error& e) {
        spdlog::error("{}", e.what());
        return -1;
    } catch (Error& e) {
        spdlog::error("{}", e.strerror());
        return -1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    srand(time(0));
    cout << "Janushash Miner (By CoinFuMasterShifu) ⚒ ⛏" << endl;
    gengetopt_args_info ai;
    if (cmdline_parser(argc, argv, &ai) != 0) {
        return -1;
    }
    int i = process(ai);
    cmdline_parser_free(&ai);
    return i;
}
