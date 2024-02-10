#include "container.hpp"
#include "block/body/view.hpp"
#include "general/errors.hpp"
#include "general/params.hpp"
#include "general/reader.hpp"
#include "general/writer.hpp"

BodyContainer::BodyContainer(std::span<const uint8_t> s)
{
    if (s.size() > MAXBLOCKSIZE) {
        throw Error(EBLOCKSIZE);
    }
}

BodyView BodyContainer::view(NonzeroHeight h, bool testnet) const
{
    return { bytes, h, testnet };
};

BodyContainer::BodyContainer(Reader& r)
{
    auto s { r.span() };
    bytes.assign(s.begin(), s.end());
};

Writer& operator<<(Writer& r, const BodyContainer& b)
{
    return r << (uint32_t)b.bytes.size() << Range(b.bytes);
};
