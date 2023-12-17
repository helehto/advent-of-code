#pragma once

#include <boost/container/static_vector.hpp>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <x86intrin.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define ASSERT(x)                                                                        \
    do {                                                                                 \
        if (!(x)) {                                                                      \
            fprintf(stderr,                                                              \
                    "\x1b[1;31m" __FILE__                                                \
                    ":" STRINGIFY(__LINE__) ": %s: Assertion `%s' failed.\x1b[m\n",      \
                    __PRETTY_FUNCTION__, #x);                                            \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

#define ASSERT_MSG(x, format, ...)                                                       \
    do {                                                                                 \
        if (!(x)) {                                                                      \
            fprintf(stderr,                                                              \
                    "\x1b[1;31m" __FILE__                                                \
                    ":" STRINGIFY(__LINE__) ": %s: Assertion `%s' failed:\x1b[m ",       \
                    __PRETTY_FUNCTION__, #x);                                            \
            fmt::print(stderr, format "\n", __VA_ARGS__);                                \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

constexpr uint64_t hash_mix(uint64_t x)
{
    // const auto m = UINT64_C((uint64_t(0xe9846af) << 32) + 0x9b1a615d;
    const uint64_t m = UINT64_C(0xe9846af9b1a615d);
    x ^= x >> 32;
    x *= m;
    x ^= x >> 32;
    x *= m;
    x ^= x >> 28;
    return x;
}

template <typename T, typename... Rest>
constexpr void hash_combine(std::size_t &h, const T &v, const Rest &...rest)
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

    constexpr bool operator==(const Point &other) const = default;
    constexpr bool operator!=(const Point &other) const = default;
};

template <typename T>
T manhattan(const Point<T> &a, const Point<T> &b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

template <typename T>
struct std::hash<Point<T>> {
    constexpr size_t operator()(const Point<T> &p) const
    {
        size_t h = 0;
        hash_combine(h, p.x, p.y);
        return h;
    }
};

template <typename T>
struct fmt::formatter<Point<T>> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

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

    bool heap_cmp(size_t a, size_t b) { return cmp_(heap_[a], heap_[b]); }

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
    explicit BinaryHeap(Compare cmp, size_t n)
        : cmp_(cmp)
    {
        heap_.reserve(n);

        for (size_t i = 0; i < n; i++)
            heap_.push_back(i);

        // make_heap builds a max-heap but we want a min-heap, hence the
        // reversed comparison.
        std::make_heap(begin(heap_), end(heap_),
                       [&cmp](size_t a, size_t b) { return cmp(b, a); });

        index_map_.resize(n);
        for (size_t i = 0; i < n; i++)
            index_map_[heap_[i]] = i;
    }

    bool empty() const { return heap_.empty(); }

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
    constexpr size_t chunk_size = 256;

    s.clear();
    size_t offset = 0;

    __m256i vzero = _mm256_setzero_si256();
    __m256i vnl = _mm256_set1_epi8('\n');

    while (true) {
        s.resize(s.size() + chunk_size);

    read_more:
        if (fgets(s.data() + offset, chunk_size, f) == nullptr) {
            s.resize(offset);
            return !s.empty();
        }

        // No break condition, the null byte added by fgets() will break the
        // loop if we reach the end of the chunk.
        for (size_t j = offset;; j += 32) {
            auto bytes = _mm256_lddqu_si256(reinterpret_cast<__m256i *>(s.data() + j));

            if (int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(bytes, vnl))) {
                offset = j + __builtin_ctz(mask);
                s.resize(offset);
                return true;
            }

            if (int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(bytes, vzero))) {
                offset = j + __builtin_ctz(mask);
                s.resize(offset + chunk_size);
                goto read_more;
            }
        }
    }
}

inline std::vector<std::string> getlines(FILE *f)
{
    std::vector<std::string> lines;
    std::string s;
    while (getline(f, s))
        lines.push_back(std::move(s));
    return lines;
}

template <typename T>
static void find_numbers(std::string_view s, std::vector<T> &result)
{
    result.clear();

    while (true) {
        while (!s.empty() && s.front() != '-' && !(s.front() >= '0' && s.front() <= '9'))
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

template <typename Predicate>
static inline std::vector<std::string_view> &
split(std::string_view s, std::vector<std::string_view> &out, Predicate &&predicate)
{
    out.clear();

    while (true) {
        while (!s.empty() && predicate(s.front()))
            s.remove_prefix(1);
        if (s.empty())
            break;

        size_t i = 0;
        while (i < s.size() && !predicate(s[i]))
            i++;
        out.emplace_back(s.data(), i);
        s.remove_prefix(i);
    }

    return out;
}

static inline std::vector<std::string_view> &split(std::string_view s,
                                                   std::vector<std::string_view> &out)
{
    return split(s, out, [](char c) { return isspace(c); });
}

static inline std::vector<std::string_view> split(std::string_view s)
{
    std::vector<std::string_view> out;
    split(s, out);
    return out;
}

struct Ndindex2DRange {
    size_t rows;
    size_t cols;
    size_t i = 0;
    size_t j = 0;

    struct sentinel {};

    Ndindex2DRange begin() { return *this; }
    sentinel end() { return {}; }

    Point<size_t> operator*() const { return {j, i}; }
    bool operator==(sentinel) const { return i >= rows; }

    Ndindex2DRange &operator++()
    {
        if (j < cols - 1) {
            j++;
        } else {
            i++;
            j = 0;
        }

        return *this;
    }

    Ndindex2DRange operator++(int) { return ++*this; }
};

template <typename T>
struct Matrix {
    std::unique_ptr<T[]> data;
    size_t rows;
    size_t cols;

    constexpr Matrix() = default;

    Matrix(size_t rows_, size_t cols_, T value = T())
        : data(new T[rows_ * cols_])
        , rows(rows_)
        , cols(cols_)
    {
        for (auto &v : *this)
            v = value;
    }

    Matrix(const Matrix &other)
        : data(new T[other.rows * other.cols])
        , rows(other.rows)
        , cols(other.cols)
    {
        std::copy(other.begin(), other.end(), data.get());
    }

    Matrix &operator=(const Matrix &other)
    {
        Matrix m(other);
        std::swap(m, *this);
        return *this;
    }

    Matrix(Matrix &&) = default;
    Matrix &operator=(Matrix &&) = default;

    template <typename Proj = std::identity>
    static Matrix from_lines(std::span<const std::string> lines, Proj proj = {})
    {
        Matrix m(lines.size(), lines[0].size());
        T *p = m.data.get();
        for (size_t i = 0; i < lines.size(); i++) {
            for (size_t j = 0; j < lines[0].size(); j++)
                *p++ = proj(lines[i][j]);
        }
        return m;
    }

    bool operator==(const Matrix &other) const noexcept
    {
        return std::equal(begin(), end(), other.begin());
    }
    bool operator!=(const Matrix &other) const noexcept { return !(*this == other); }

    constexpr T &operator()(size_t i, size_t j) { return data[i * cols + j]; }
    constexpr const T &operator()(size_t i, size_t j) const { return data[i * cols + j]; }

    template <typename U>
    constexpr T &operator()(Point<U> p)
    {
        return data[p.y * cols + p.x];
    }

    template <typename U>
    constexpr const T &operator()(Point<U> p) const
    {
        return data[p.y * cols + p.x];
    }

    constexpr size_t size() const { return rows * cols; }

    constexpr T *begin() { return data.get(); }
    constexpr T *end() { return data.get() + rows * cols; }
    constexpr const T *begin() const { return data.get(); }
    constexpr const T *end() const { return data.get() + rows * cols; }

    Ndindex2DRange ndindex() const { return {rows, cols}; }
};

template <typename T, typename Predicate>
static void erase_if(std::vector<T> &v, Predicate &&predicate)
{
    auto it = std::remove_if(begin(v), end(v), std::forward<Predicate>(predicate));
    v.erase(it, end(v));
}

static std::array<Point<size_t>, 4> neighbors4(Point<size_t> p)
{
    return {{
        {p.x, p.y - 1},
        {p.x + 1, p.y},
        {p.x, p.y + 1},
        {p.x - 1, p.y},
    }};
}

template <typename T>
static boost::container::static_vector<Point<size_t>, 4>
neighbors4(const Matrix<T> &chart, Point<size_t> p)
{
    boost::container::static_vector<Point<size_t>, 4> result;
    for (auto n : neighbors4(p))
        if (n.x < chart.cols && n.y < chart.rows)
            result.push_back(n);

    return result;
}

static std::array<Point<size_t>, 8> neighbors8(Point<size_t> p)
{
    return {{
        {p.x - 1, p.y - 1},
        {p.x - 1, p.y},
        {p.x - 1, p.y + 1},
        {p.x, p.y - 1},
        {p.x, p.y + 1},
        {p.x + 1, p.y - 1},
        {p.x + 1, p.y},
        {p.x + 1, p.y + 1},
    }};
}

template <typename T>
static boost::container::static_vector<Point<size_t>, 8> neighbors8(const Matrix<T> &grid,
                                                                    Point<size_t> p)
{
    boost::container::static_vector<Point<size_t>, 8> result;
    for (auto n : neighbors8(p))
        if (n.x < grid.cols && n.y < grid.rows)
            result.push_back(n);

    return result;
}
