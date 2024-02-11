#include "job.hpp"
#include "crypto/hasher_sha256.hpp"
#include "general/byte_order.hpp"

std::vector<uint8_t> StratumJobGenerator::gen_extra2() const
{
    assert(data->_extranonce);
    auto& e_ref(*data->_extranonce);
    const uint32_t e { hton32(e_ref++) };
    size_t s { data->subscription.extranonce2size };
    std::vector<uint8_t> extra2(s, 0);
    memcpy(extra2.data() + s - sizeof(e), &e, s);
    return extra2;
};

Hash StratumJobGenerator::gen_merkle_root(std::vector<uint8_t> extra2) const
{
    return HasherSHA256()
        << data->_notify.merklePrefix
        << data->subscription.extranonce1
        << extra2;
}

StratumJob StratumJobGenerator::generate_job() const
{
    return StratumJob(*this);
}

std::array<uint8_t, 76> StratumJobGenerator::gen_header(const std::vector<uint8_t>& extra2) const
{
    auto& n { data->_notify };
    std::array<uint8_t, 76> h;
    Hash merkleroot { gen_merkle_root(extra2) };

    memcpy(h.data() + HeaderView::offset_prevhash, n.prevHash.data(), 32);
    memcpy(h.data() + HeaderView::offset_target, n.nbits.data(), 4);
    memcpy(h.data() + HeaderView::offset_merkleroot, merkleroot.data(), 32);
    memcpy(h.data() + HeaderView::offset_version, n.nversion.data(), 4);
    memcpy(h.data() + HeaderView::offset_timestamp, n.ntime.data(), 4);
    return h;
};
