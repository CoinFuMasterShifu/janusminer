#include "mine_job.hpp"
#include "block/body/view.hpp"
#include "block/header/header_impl.hpp"
#include "helpers.hpp"
#include <cstring>

NodeJobGenerator::NodeJobGenerator(MineJob job)
    : mineJob(std::move(job))
{
    nonce2 = randuint32();
}

MineJob NodeJobGenerator::generate_job() const
{
    MineJob out { mineJob };
    out.set_random_seed(nonce2++);
    return out;
}

// NodeJobGenerator::NodeJobGenerator(NodeJobGenerator&& ng)
//     :nonce2(ng.nonce2.load()), mineJob(ng.mineJob)
// {};

void set_random_seed(uint32_t newSeed);
void MineJob::set_random_seed(uint32_t newSeed)
{
    memcpy(block.body.data().data(), &newSeed, 4);
    BodyView bv(block.body.view(block.height,testnet));
    
    block.header.set_merkleroot(bv.merkleRoot(block.height,testnet));
}

std::array<uint8_t, 76> MineJob::header() const
{
    std::array<uint8_t, 76> h;
    std::copy(block.header.data(), block.header.data() + 76, h.data());
    return h;
};
