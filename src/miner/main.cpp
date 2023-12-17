#include "api_call.hpp"
#include "cmdline/cmdline.h"
#include "crypto/address.hpp"
#include "log/trace.hpp"
#include "crypto/verushash/verushash.hpp"
#include "general/hex.hpp"
#include "spdlog/spdlog.h"
#include <iostream>
using namespace std;

int start_miner(const Address& address, std::string host, uint16_t port, std::string gpus, size_t threads);

int test_gpu_miner2();

int process(gengetopt_args_info& ai)
{
    try {
        std::string host { ai.host_arg };
        uint16_t port(ai.port_arg);
        spdlog::info("Node RPC is {}:{}", host, port);
        if (ai.threads_arg < 0)
            throw std::runtime_error("Illegal value " + to_string(ai.threads_arg) + " for option --threads.");
        size_t threads{ai.threads_arg == 0 ? std::thread::hardware_concurrency(): ai.threads_arg};
        Address address(ai.address_arg);
        std::string gpus;
        if (ai.gpus_given) {
            gpus.assign(ai.gpus_arg);
        }
        start_miner(address, host, port, gpus, threads);
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
    // trace_log().info("Hello World");
    // trace_log().flush();
    // std::array<uint8_t,80> arr;
    // assert(parse_hex("9cf9453887413be274bf5ef7e57509b844c7055f4a64a5813c5a4e72e689d33c087fffff80a06ad2fea4aa42d9f6fa1ed05d3dfdf648f8e53e6aa45c5a2cee6d6dc6042c00000002657b67597f06a2e9",arr));
    // cout<<"Hash: "<< serialize_hex(verus_hash(arr));
    // return 0;

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
