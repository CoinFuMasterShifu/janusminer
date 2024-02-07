#pragma once
#include <string>
namespace stratum {
struct ConnectionData {
    std::string host;
    std::string port;
    size_t queuesizeGB{4};
    std::string user { "69ce24480e227ce13dddf3f849b5a5495c2177c9cfdab045" };
    std::string pass { "pass" };
};
}
