#pragma once

#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fmt/core.h>
#include <functional>
#include <type_traits>
#include <utility>

constexpr uint64_t hash_mix(uint64_t x)
{
        //const auto m = UINT64_C((uint64_t(0xe9846af) << 32) + 0x9b1a615d;
        const uint64_t m = UINT64_C(0xe9846af9b1a615d);
        x ^= x >> 32;
        x *= m;
        x ^= x >> 32;
        x *= m;
        x ^= x >> 28;
        return x;
}

template <typename T, typename... Rest>
constexpr void hash_combine(std::size_t& h, const T& v, const Rest&... rest)
{
    const std::uint64_t m = 0xc6a4a7935bd1e995ULL;
    std::uint64_t k = std::hash<T>{}(v);
    const int r = 47;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;

    // Completely arbitrary number, to prevent 0's
    // from hashing to 0.
    h += 0xe6546b64;

    (hash_combine(h, rest), ...);
}

template <typename T>
struct Point {
    T x;
    T y;

    constexpr bool operator==(const Point& other) const = default;
    constexpr bool operator!=(const Point& other) const = default;
};

template <typename T>
T manhattan(const Point<T> &a, const Point<T> &b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

template <typename T>
struct std::hash<Point<T>> {
    constexpr size_t operator()(const Point<T>& p) const
    {
        size_t h = 0;
        hash_combine(h, p.x, p.y);
        return h;
    }
};

template <typename T>
struct fmt::formatter<Point<T>> {
    constexpr auto parse(format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const Point<T> &p, FormatContext &ctx) const
    {
        // ctx.out() is an output iterator to write to.
        return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};

constexpr int signum(int x)
{
    if (x < 0)
        return -1;
    else if (x > 0)
        return 1;
    else
        return 0;
}

/// Implementation of a binary min-heap with a decrease-key operation as used
/// in Dijkstra's algorithm and A*.
template <typename Compare>
class BinaryHeap {
private:
    std::vector<uint32_t> heap_;
    std::vector<uint32_t> index_map_;
    Compare cmp_;

    void heap_swap(size_t a, size_t b)
    {
        std::swap(heap_[a], heap_[b]);
        index_map_[heap_[a]] = a;
        index_map_[heap_[b]] = b;
    }

    bool heap_cmp(size_t a, size_t b)
    {
        return cmp_(heap_[a], heap_[b]);
    }

    void sift_down(size_t i)
    {
        while (true) {
            const auto l = 2 * i + 1;
            const auto r = 2 * i + 2;

            auto min_index = i;
            if (l < heap_.size() && heap_cmp(l, min_index))
                min_index = l;
            if (r < heap_.size() && heap_cmp(r, min_index))
                min_index = r;
            if (min_index == i)
                break;

            heap_swap(min_index, i);
            i = min_index;
        }
    }

    void sift_up(size_t i)
    {
        while (i > 0) {
            auto p = (i - 1) / 2;
            if (!heap_cmp(i, p))
                break;
            heap_swap(p, i);
            i = p;
        }
    }

public:
    explicit BinaryHeap(Compare cmp, size_t n) : cmp_(cmp)
    {
        heap_.reserve(n);

        for (size_t i = 0; i < n; i++)
            heap_.push_back(i);

        // make_heap builds a max-heap but we want a min-heap, hence the
        // reversed comparison.
        std::make_heap(begin(heap_), end(heap_),
                       [&cmp](size_t a, size_t b) { return !cmp(a, b); });

        index_map_.resize(n);
        for (size_t i = 0; i < n; i++)
            index_map_[heap_[i]] = i;
    }

    bool empty() const
    {
        return heap_.empty();
    }

    size_t top() const
    {
        assert(!empty());
        return heap_.front();
    }

    void pop()
    {
        assert(!empty());
        std::swap(heap_.front(), heap_.back());
        index_map_[heap_.front()] = 0;
        heap_.pop_back();
        sift_down(0);
    }

    void decrease(size_t v)
    {
        // Ignore nodes already removed from the heap. This could be the case
        // if we are checking a neighbor that has already been processed once.
        if (const auto i = index_map_[v]; i < heap_.size())
            sift_up(i);
    }
};

template <typename T>
constexpr T modulo(T x, T mod)
{
    auto r = x % mod;
    if (r < 0)
        r += mod;
    return r;
}

inline bool getline(FILE *f, std::string &s)
{
    s.clear();

    int c = getc(f);
    if (c == EOF)
        return false;

    do {
        if (c == '\n')
            break;
        s.push_back(c);
        c = getc(f);
    } while (c != EOF);

    return true;
}

template <typename T>
static void find_numbers(std::string_view s, std::vector<T> &result)
{
    result.clear();

    while (true) {
        while (!s.empty() && s.front() != '-' && !isdigit(s.front()))
            s.remove_prefix(1);
        if (s.empty())
            return;

        T value;
        auto r = std::from_chars(s.data(), s.data() + s.size(), value);
        assert(r.ec != std::errc::result_out_of_range);
        if (r.ec == std::errc()) {
            result.push_back(value);
            s.remove_prefix(r.ptr - s.data());
        } else {
            s.remove_prefix(1);
        }
    }
}

template <typename T>
static std::vector<T> find_numbers(std::string_view s)
{
    std::vector<T> result;
    find_numbers(s, result);
    return result;
}
