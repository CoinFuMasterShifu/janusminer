#include "view.hpp"
// #include "crypto/crypto.hpp"
#include "crypto/hasher_sha256.hpp"
#include "general/hex.hpp"
#include "general/reader.hpp"
#include <iostream>
#include <vector>
using namespace std;

BodyView::BodyView(std::span<const uint8_t> s)
    : s(s)
{
    Reader rd { s };

    if (s.size() > MAXBLOCKSIZE)
        return;
    // Read new address section
    rd.skip(10); // for mining
    nAddresses = rd.uint16();
    offsetAddresses = rd.cursor() - s.data();
    if (rd.remaining() < nAddresses * AddressSize)
        return;
    rd.skip(nAddresses * AddressSize);

    // Read reward section
    nRewards = 1;
    if (rd.remaining() < RewardSize * nRewards)
        return;
    offsetRewards = rd.cursor() - s.data();
    rd.skip(16 * nRewards);

    // Read payment section
    if (rd.remaining() != 0) {
        nTransfers = rd.uint32();
        // Make sure that it has correct length
        if (rd.remaining() != (TransferSize)*nTransfers)
            return;
    }
    offsetTransfers = rd.cursor() - s.data();
    isValid = true;
};

Hash BodyView::merkleRoot() const
{
    assert(isValid);
    std::vector<Hash> hashes(nAddresses + nRewards + nTransfers);

    // hash addresses
    size_t idx = 0;
    for (size_t i = 0; i < nAddresses; ++i)
        hashes[idx++] = hashSHA256(s.data() + offsetAddresses + i * AddressSize, AddressSize);

    // hash payouts
    for (size_t i = 0; i < nRewards; ++i)
        hashes[idx++] = hashSHA256(data() + offsetRewards + i * RewardSize, RewardSize);
    // hash payments
    for (size_t i = 0; i < nTransfers; ++i)
        hashes[idx++] = hashSHA256(data() + offsetTransfers + i * TransferSize, TransferSize);
    std::vector<Hash> tmp, *from, *to;
    from = &hashes;
    to = &tmp;
    do {
        to->resize((from->size() + 1) / 2);
        size_t j = 0;
        for (size_t i = 0; i < (from->size() + 1) / 2; ++i) {
            HasherSHA256 hasher {};
            hasher.write((*from)[j].data(), 32);
            if (j + 1 < from->size()) {
                hasher.write((*from)[j + 1].data(), 32);
            }

            if (to->size() == 1)
                hasher.write(data(), 4); // first 4 bytes in block are for extranonce I think?
            (*to)[i] = std::move(hasher);
            j += 2;
        }
        std::swap(from, to);
    } while (from->size() > 1);
    return from->front();
}
