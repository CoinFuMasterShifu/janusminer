#pragma once
#include "sha256t_results.hpp"
#include "spdlog/spdlog.h"
#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

class CyclicQueue : public std::enable_shared_from_this<CyclicQueue> {
public:
    class Buffer {
    public:
        friend class CyclicQueue;
        const std::span<uint32_t> first;
        const std::span<uint32_t> second;
        const size_t destructCursor;
        const std::shared_ptr<CyclicQueue> parent;

    private:
        std::shared_ptr<Buffer> next;

    public:
        Buffer(std::span<uint32_t> first, std::span<uint32_t> second, size_t destructCursor, std::shared_ptr<CyclicQueue> parent)
            : first(std::move(first))
            , second(std::move(second))
            , destructCursor(destructCursor)
            , parent(std::move(parent))
        {
            assert(first.size() > 0 && (first.size() & 1));
        };
        Buffer(const Buffer&) = delete;
        size_t result_size() const
        {
            return std::min(size_t(first[0]), (first.size() + second.size()) / 2);
        }
        sha256t_results::spans result_spans()
        {
            using namespace std;
            auto n(first[0]);
            auto nfirst = std::min(n, (uint32_t(first.size()) - 1) / 2);
            n -= nfirst;
            std::span<uint32_t> sfirst(first.data() + 1, first.data() + 1 + 2 * nfirst);
            std::span<uint32_t> ssecond;
            if (n > 0 && !second.empty()) {
                auto nsecond = std::min(n, uint32_t(second.size() / 2));
                ssecond = std::span<uint32_t>(second.data(), second.data() + 2 * nsecond);
            }

            return {
                sfirst,
                ssecond
            };
        }
        ~Buffer()
        {
            if (next.use_count() <= 1)
                parent->free(destructCursor);
        }
    };

    CyclicQueue(std::function<void()> freeCallback, size_t entries)
        : data(entries)
        , i0(data.size())
        , i1(data.size())
        , freeCallback(freeCallback)
    {
        assert(entries >= 3);
    }
    class Allocator {
        friend class CyclicQueue;
        std::shared_ptr<CyclicQueue> p;
        std::unique_lock<std::recursive_mutex> l;
        Allocator(CyclicQueue& cq)
            : p(cq.shared_from_this())
            , l(p->m)
        {
        }

    public:
        size_t available() const { return p->available(); }
        std::shared_ptr<Buffer> alloc(size_t nElements)
        {
            return p->alloc(nElements);
        }
    };
    Allocator allocator() { return *this; }

private:
    size_t odd_available(size_t n) const
    {
        if (n < 3)
            return 0;
        if (n & 1)
            n -= 1;
        return n;
    }
    size_t even_available(size_t n) const
    {
        if (n < 2)
            return 0;
        if ((n & 1) != 0)
            n -= 1;
        return n;
    }
    size_t available() const
    {
        if (i0 < i1) {
            return odd_available(i1 - i0);
        } else if (i0 == i1) {
            if (hasSpace)
                return odd_available(data.size());
            return 0;
        } else { // i0 > i1
            size_t s { 0 };
            s += odd_available(data.size() - i0);
            if (s == 0)
                return s + odd_available(i1);
            else
                return s + even_available(i1);
        }
    }
    [[nodiscard]] std::shared_ptr<Buffer> alloc(size_t nElements)
    {
        assert(nElements & 1 && nElements >= 3); // need odd number of elements
        std::shared_ptr<Buffer> res;
        if (i0 < i1) {
            auto s { alloc_part(i0, nElements, i1 - i0) };
            if (s.empty())
                return failed_alloc();

            res = std::make_shared<Buffer>(
                s, // first
                std::span<uint32_t> {}, // second
                i0,
                shared_from_this());
        } else { // i0 >= i1
            if (!hasSpace)
                return failed_alloc();
            assert(i1 != 0);

            auto s1 { alloc_part(i0, nElements, data.size() - i0) };
            std::span<uint32_t> s2;
            if (s1.empty())
                s1 = alloc_part(0, nElements, i1);
            else
                s2 = alloc_part(0, nElements, i1);
            if (s1.empty())
                return failed_alloc();
            res = std::make_shared<Buffer>(
                s1,
                s2,
                i0,
                shared_from_this());
        }

        assert(res);
        if (auto l { prev.lock() }; l)
            l->next = res;
        prev = res;
        return res;
    }
    std::shared_ptr<Buffer> failed_alloc()
    {
        failedAlloc = true;
        return {};
    }
    void free(size_t destructCursor)
    {
        std::lock_guard l(m);
        i1 = destructCursor;
        if (i0 == i1)
            hasSpace = true;
        if (failedAlloc) {
            failedAlloc = false;
            freeCallback();
        }
    }
    [[nodiscard]] std::span<uint32_t> alloc_part(size_t j0, size_t& nElements, size_t maxlen)
    {
        bool odd = (nElements & 1);
        size_t len { nElements };
        if (len > maxlen)
            len = maxlen;
        if (len == 0)
            return {};
        if ((len & 1) != odd)
            len -= 1;
        if (len < 2)
            return {};
        i0 = j0 + len;
        nElements -= len;
        hasSpace = i0 != i1;
        return { data.data() + j0, data.data() + i0 };
    }

private:
    using data_t = std::vector<uint32_t>;
    using iter_t = data_t::iterator;
    std::weak_ptr<Buffer> prev;
    data_t data;

    std::recursive_mutex m;
    size_t i0;
    size_t i1;
    bool hasSpace { true };
    bool failedAlloc { false };
    std::function<void()> freeCallback;
};
