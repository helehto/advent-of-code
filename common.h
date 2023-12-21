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
            fmt::print(stderr, format "\n" __VA_OPT__(, ) __VA_ARGS__);                  \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

#ifdef DEBUG
#define D(x, ...) fmt::print("[DEBUG] " x "\n" __VA_OPT__(,)__VA_ARGS__)
#else
#define D(...)
#endif

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

    template <typename U>
    constexpr Point<T> translate(U dx, U dy) const {
        return {static_cast<T>(x + dx), static_cast<T>(y + dy)};
    }

    template <typename U>
    constexpr Point<U> cast() const {
        ASSERT(static_cast<T>(x) == x);
        ASSERT(static_cast<T>(y) == y);
        return Point<U>{
            static_cast<U>(x),
            static_cast<U>(y),
        };
    }
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

struct SlurpResult {
    std::string contents;
    std::vector<std::string_view> lines;
};

static inline std::vector<std::string_view> &
split(std::string_view s, std::vector<std::string_view> &out, char c);

inline SlurpResult slurp_lines(FILE *f)
{
    int fd = fileno(f);

    off_t size = lseek(fd, 0, SEEK_END);
    ASSERT(size > 0);

    std::string buf;
    buf.resize(size);
    ASSERT(pread(fd, buf.data(), size, 0) == size);

    std::vector<std::string_view> lines;
    split(buf, lines, '\n');

    return {std::move(buf), std::move(lines)};
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

static inline std::vector<std::string_view> &
split(std::string_view s, std::vector<std::string_view> &out, char c)
{
    out.clear();

    size_t i = 0;
    size_t curr_field_start = 0;

    for (; i + 31 < s.size(); i += 32) {
        __m256i m = _mm256_lddqu_si256(reinterpret_cast<const __m256i *>(s.data() + i));
        unsigned int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(m, _mm256_set1_epi8(c)));

        for (; mask != 0; mask &= mask - 1) {
            int offset = std::countr_zero(mask);
            out.emplace_back(s.begin() + curr_field_start, s.begin() + i + offset);
            curr_field_start = i + offset + 1;
        }
    }

    for (; i + 15 < s.size(); i += 16) {
        __m128i m = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(s.data() + i));
        unsigned int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(m, _mm_set1_epi8(c)));

        for (; mask != 0; mask &= mask - 1) {
            int offset = std::countr_zero(mask);
            out.emplace_back(s.begin() + curr_field_start, s.begin() + i + offset);
            curr_field_start = i + offset + 1;
        }
    }

    for (; i < s.size(); i++) {
        if (s[i] == c) {
            out.emplace_back(s.begin() + curr_field_start, s.begin() + i);
            curr_field_start = i + 1;
        }
    }

    if (curr_field_start != s.size())
        out.emplace_back(s.begin() + curr_field_start, s.end());

    return out;
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

static inline std::string_view strip(std::string_view s)
{
    while (!s.empty() && s.front() == ' ')
        s.remove_prefix(1);
    while (!s.empty() && s.back() == ' ')
        s.remove_suffix(1);
    return s;
}

template <typename T>
struct Ndindex2DRange {
    size_t rows;
    size_t cols;
    T i = 0;
    T j = 0;

    struct sentinel {};

    Ndindex2DRange begin() { return *this; }
    sentinel end() { return {}; }

    Point<T> operator*() const { return {j, i}; }
    bool operator==(sentinel) const { return static_cast<size_t>(i) >= rows; }

    Ndindex2DRange &operator++()
    {
        if (static_cast<size_t>(j) < cols - 1) {
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
    static Matrix from_lines(std::span<const std::string_view> lines, Proj proj = {})
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

    template <typename U = size_t>
    Ndindex2DRange<U> ndindex() const
    {
        return {rows, cols};
    }

    template <typename U>
    bool in_bounds(Point<U> p) const {
        using Unsigned = std::make_unsigned_t<U>;
        return static_cast<Unsigned>(p.x) < cols && static_cast<Unsigned>(p.y) < rows;
    }
};

template <typename T, typename Predicate>
static void erase_if(std::vector<T> &v, Predicate &&predicate)
{
    auto it = std::remove_if(begin(v), end(v), std::forward<Predicate>(predicate));
    v.erase(it, end(v));
}

template <typename T>
constexpr static std::array<Point<T>, 4> neighbors4(Point<T> p)
{
    return {{
        p.translate(0, -1),
        p.translate(+1, 0),
        p.translate(0, +1),
        p.translate(-1, 0),
    }};
}

template <typename T, typename U>
static boost::container::static_vector<Point<U>, 4>
neighbors4(const Matrix<T> &chart, Point<U> p)
{
    boost::container::static_vector<Point<U>, 4> result;
    for (auto n : neighbors4(p))
        if (chart.in_bounds(n))
            result.push_back(n);

    return result;
}

template <typename T>
constexpr static std::array<Point<T>, 8> neighbors8(Point<T> p)
{
    return {{
        p.translate(-1, -1),
        p.translate(-1, +0),
        p.translate(-1, +1),
        p.translate(+0, -1),
        p.translate(+0, +1),
        p.translate(+1, -1),
        p.translate(+1, +0),
        p.translate(+1, +1),
    }};
}

template <typename T, typename U>
static boost::container::static_vector<Point<U>, 8> neighbors8(const Matrix<T> &grid,
                                                               Point<U> p)
{
    boost::container::static_vector<Point<U>, 8> result;
    for (auto n : neighbors8(p))
        if (grid.in_bounds(n))
            result.push_back(n);

    return result;
}
