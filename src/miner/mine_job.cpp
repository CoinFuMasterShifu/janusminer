#include "mine_job.hpp"
#include "block/body/view.hpp"
#include "block/header/header_impl.hpp"
#include<cstring>
void MineJob::set_random_seed(uint32_t newSeed){
    memcpy(block.body.data().data(), &newSeed, 4);
    BodyView bv(block.body.view());
    block.header.set_merkleroot(bv.merkleRoot());
}

std::array<uint8_t,76> MineJob::header() const {
    std::array<uint8_t, 76> h;
    std::copy(block.header.data(), block.header.data() + 76, h.data());
    return h;
};
