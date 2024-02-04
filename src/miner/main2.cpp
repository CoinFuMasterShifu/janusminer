#include "block/header/custom_float.hpp"
#include "cmdline/cmdline.h"
#include "crypto/address.hpp"
#include "crypto/verushash/verushash.hpp"
#include "cyclic_queue.hpp"
#include "device_pool.hpp"
#include "general/hex.hpp"
#include "gpu/opencl_hasher.hpp"
#include "log/trace.hpp"
#include "spdlog/spdlog.h"
#include "stratum/connection.hpp"
#include <chrono>
#include <iostream>
#include <variant>
using namespace std;
CL::Device get_device()
{
    for (auto& p : CL::Platform::all()) {
        for (auto& d : p.devices(CL_DEVICE_TYPE_GPU)) {
            return std::move(d);
        }
    }
    throw std::runtime_error("No device");
}

int main()
{
    auto device { get_device() };
    // {
    //     Sha256tOpenclHasher hasher({ device });
    //     Header h;
    //     memset(&h, 0, sizeof(h));
    //     hasher.start_mining(h);
    //     sleep(300);
    //     hasher.stop_mining();
    //     sleep(3);
    //     hasher.start_mining(h);
    // }
    cout << "Can quit." << endl;
    return 0;
}
