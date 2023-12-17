#pragma once
#include <cstdint>
#include <endian.h>
constexpr bool DEBUG_PARAMS = false;
constexpr bool ENABLE_DEFAULT_NODE = true;

constexpr uint64_t COINUNIT = 100000000; // 1 UNIT of the coin in the smallest representable quantity (i.e. implicit definition of precision)

/////////////
// Communication parameters
/////////////
constexpr uint16_t DEFAULT_ENDPOINT_PORT = 9186; // this value is also in cmdline/cmdoptions.ggo
constexpr uint32_t MAXBUFFER = 3 * 1000 * 1000; // 3 MB send buffer size bound (connection is closed if exceeded)

/////////////
// Genesis Block
/////////////
constexpr const char GENESISSEED[] = "The New York Times International Edition 29/06/2023: 'Water troubles: A preview'"; // use SHA256(GENESISSEED) to seed genensis block prev hash
constexpr uint8_t GENESISDIFFICULTYEXPONENT = (DEBUG_PARAMS ? 18 : 32); // 2^(<this number>) is the expected number of tries to mine the first block
constexpr uint64_t GENESISBLOCKREWARD = 3 * COINUNIT; // total reward mined in every block before halving occurs

/////////////
// Block parameters
/////////////
constexpr uint64_t BLOCKTIME = 20; // difficulty is rebalanced such that one block takes BLOCKTIME seconds
constexpr uint32_t MAXBLOCKSIZE = 35000; // max size per block (i.e. approximateily 1 MB every 10 minutes)

/////////////
// Batch parameters
/////////////
constexpr uint32_t HEADERBATCHSIZE = DEBUG_PARAMS ? 16 : 8640; // number of headers that are returned in one batch, reqestable from offsets equal to multiples of this number
constexpr uint32_t MAXBLOCKBATCHSIZE = 30; // maximal number of blocks that can be requested
constexpr uint32_t BLOCKBATCHSIZE = 30; // number of blocks that the node requests in one batch

/////////////
// Timestamp parameters
/////////////
constexpr uint32_t TOLERANCEMINUTES = 10; // tolerate block timestamps this much larger than current time
constexpr uint32_t MEDIAN_N = DEBUG_PARAMS ? 5 : 50; // the median of this many block timestamps must not decrease

/////////////
// Halving
/////////////
constexpr uint32_t HALVINTINTERVAL = DEBUG_PARAMS ? 400 : (2 * 365 * 24 * 60 * 60) / BLOCKTIME; // halving approx. every two years
constexpr uint64_t MAXSUPPLY = GENESISBLOCKREWARD * HALVINTINTERVAL * 2;

/////////////
// Difficulty adjustment heights
/////////////
inline constexpr uint32_t retarget_floor(uint32_t height)
{
    constexpr uint32_t fourhours = 4 * 60 * 60 / BLOCKTIME;
    constexpr uint32_t fivemonths = 5 * 30 * 24 * 60 * 60 / BLOCKTIME;
    static_assert(fivemonths % HEADERBATCHSIZE == 0);
    static_assert(60 % BLOCKTIME == 0);
    if (height < fivemonths) {
        const uint32_t val = (height / fourhours) * fourhours;
        if (val == 0)
            return 1;
        return val;
    } else {
        return (height / HEADERBATCHSIZE) * HEADERBATCHSIZE;
    }
}

/////////////
// Janushash Start
/////////////
constexpr bool JANUSENABLED = true;
constexpr uint32_t JANUSRETARGETSTART = retarget_floor(720);
