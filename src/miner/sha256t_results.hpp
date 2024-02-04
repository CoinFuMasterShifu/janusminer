#pragma once
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>

namespace sha256t_results {
class span {
public:
    span(std::span<uint32_t> s)
        : data(s)
    {
        assert((s.size() & 1) == 0);
    }

    class ElementType {
    private:
        friend class span;
        const uint32_t* pos;
        ElementType(const uint32_t* pos)
            : pos(pos)
        {
        }

    public:
        uint32_t nonce() const
        {
            return *pos;
        }
        uint32_t hashStart() const
        {
            return *(pos + 1);
        }
    };

    size_t size() const
    {
        return data.size() / 2;
    }
    class iterator_t {
        friend class span;
        iterator_t(uint32_t* pos)
            : pos(pos)
        {
        }
        ElementType operator*()
        {
            return pos;
        }
        iterator_t& operator++()
        {
            pos += 2;
            return *this;
        }
        auto operator<=>(const iterator_t&) const = default;
        uint32_t* pos;
    };
    iterator_t begin()
    {
        return data.data();
    }
    iterator_t end()
    {
        return data.data() + data.size();
    }
    ElementType operator[](size_t i) const
    {
        return &data[2 * i];
    }

private:
    std::span<uint32_t> data;
};
struct spans {
    std::array<const span,2> spans;
    size_t size() const
    {
        return spans[0].size() + spans[1].size();
    }
};
}
