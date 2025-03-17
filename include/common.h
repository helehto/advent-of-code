#pragma once

#include "inplace_vector.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <functional>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <x86intrin.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define GLUE_(x, y) x##y
#define GLUE(x, y) GLUE_(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define ASSERT(x)                                                                        \
    do {                                                                                 \
        if (!(x)) [[unlikely]] {                                                         \
            const char *_pretty_function = __PRETTY_FUNCTION__;                          \
            [&] [[gnu::cold, gnu::noinline]] () {                                        \
                fflush(stdout);                                                          \
                fprintf(stderr,                                                          \
                        "\x1b[1;31m" __FILE__                                            \
                        ":" STRINGIFY(__LINE__) ": %s: Assertion `%s' failed.\x1b[m\n",  \
                        _pretty_function, #x);                                           \
            }();                                                                         \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

#define ASSERT_MSG(x, format, ...)                                                       \
    do {                                                                                 \
        if (!(x)) [[unlikely]] {                                                         \
            const char *_pretty_function = __PRETTY_FUNCTION__;                          \
            [&] [[gnu::cold, gnu::noinline]] () {                                        \
                fflush(stdout);                                                          \
                fprintf(stderr,                                                          \
                        "\x1b[1;31m" __FILE__                                            \
                        ":" STRINGIFY(__LINE__) ": %s: Assertion `%s' failed:\x1b[m ",   \
                        _pretty_function, #x);                                           \
                fmt::print(stderr, format "\n" __VA_OPT__(, ) __VA_ARGS__);              \
            }();                                                                         \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

#ifdef NDEBUG
#define DEBUG_ASSERT(...)
#define DEBUG_ASSERT_MSG(...)
#else
#define DEBUG_ASSERT(...) ASSERT(__VA_ARGS__)
#define DEBUG_ASSERT_MSG(...) ASSERT_MSG(__VA_ARGS__)
#endif

#ifdef DEBUG
#define D(x, ...) fmt::print("[DEBUG] " x "\n" __VA_OPT__(, ) __VA_ARGS__)
#else
#define D(...)
#endif

#define ARG_COUNT_(x31, x30, x29, x28, x27, x26, x25, x24, x23, x22, x21, x20, x19, x18, \
                   x17, x16, x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3,   \
                   x2, x1, x0, ...)                                                      \
    x0
#define ARG_COUNT(...)                                                                   \
    ARG_COUNT_(__VA_ARGS__, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,  \
               16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define DV_FORMAT_1(x) #x " = \x1b[1m{}\x1b[m"
#define DV_FORMAT_2(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_1(__VA_ARGS__)
#define DV_FORMAT_3(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_2(__VA_ARGS__)
#define DV_FORMAT_4(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_3(__VA_ARGS__)
#define DV_FORMAT_5(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_4(__VA_ARGS__)
#define DV_FORMAT_6(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_5(__VA_ARGS__)
#define DV_FORMAT_7(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_6(__VA_ARGS__)
#define DV_FORMAT_8(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_7(__VA_ARGS__)
#define DV_FORMAT(...) GLUE(DV_FORMAT_, ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define DV(var, ...)                                                                     \
    fmt::print(                                                                          \
        "\x1b[1;30;43m {:>4d} \x1b[m " DV_FORMAT(var __VA_OPT__(, ) __VA_ARGS__) "\n",   \
        __LINE__, var __VA_OPT__(, ) __VA_ARGS__)

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
struct Vec2 {
    T x;
    T y;

    constexpr bool operator==(const Vec2 &other) const = default;
    constexpr bool operator!=(const Vec2 &other) const = default;

    template <typename U>
    constexpr Vec2<U> cast() const
    {
        ASSERT(static_cast<T>(x) == x);
        ASSERT(static_cast<T>(y) == y);
        return Vec2<U>{
            static_cast<U>(x),
            static_cast<U>(y),
        };
    }

    constexpr Vec2 cw() const { return Vec2(-y, x); }
    constexpr Vec2 ccw() const { return Vec2(y, -x); }

    template <typename U>
    constexpr Vec2 operator+(const Vec2<U> &other) const
    {
        return {
            static_cast<T>(x + other.x),
            static_cast<T>(y + other.y),
        };
    }

    template <typename U>
    constexpr Vec2 &operator+=(const Vec2<U> &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    template <typename U>
    constexpr Vec2 operator-(const Vec2<U> &other) const
    {
        return {
            static_cast<T>(x - other.x),
            static_cast<T>(y - other.y),
        };
    }

    template <typename U>
    constexpr Vec2 &operator-=(const Vec2<U> &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vec2 operator-()
        requires(std::is_signed_v<T>)
    {
        return {-x, -y};
    }

    template <typename Scalar>
    constexpr Vec2<std::common_type_t<T, Scalar>> operator*(const Scalar &scalar) const
    {
        return Vec2<std::common_type_t<T, Scalar>>(scalar * x, scalar * y);
    }
};

template <typename U, typename Scalar>
constexpr Vec2<std::common_type_t<U, Scalar>> operator*(const Scalar &scalar,
                                                        const Vec2<U> &v)
{
    return Vec2<std::common_type_t<U, Scalar>>(scalar * v.x, scalar * v.y);
}

using Vec2i8 = Vec2<int8_t>;
using Vec2u8 = Vec2<uint8_t>;
using Vec2i16 = Vec2<int16_t>;
using Vec2u16 = Vec2<uint16_t>;
using Vec2i32 = Vec2<int32_t>;
using Vec2u32 = Vec2<uint32_t>;
using Vec2i64 = Vec2<int64_t>;
using Vec2u64 = Vec2<uint64_t>;
using Vec2i = Vec2<int>;
using Vec2z = Vec2<size_t>;

template <typename T>
constexpr T manhattan(const Vec2<T> &a)
{
    return std::abs(a.x) + std::abs(a.y);
}

template <typename T>
constexpr T manhattan(const Vec2<T> &a, const Vec2<T> &b)
{
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

template <typename T>
struct std::hash<Vec2<T>> {
    constexpr size_t operator()(const Vec2<T> &p) const
    {
        size_t h = 0;
        hash_combine(h, p.x, p.y);
        return h;
    }
};

template <typename T>
struct fmt::formatter<Vec2<T>> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const Vec2<T> &p, FormatContext &ctx) const
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

constexpr inline uint64_t pow10i[] = {
    /* 10^0 */ UINT64_C(1),
    /* 10^1 */ UINT64_C(10),
    /* 10^2 */ UINT64_C(100),
    /* 10^3 */ UINT64_C(1000),
    /* 10^4 */ UINT64_C(10000),
    /* 10^5 */ UINT64_C(100000),
    /* 10^6 */ UINT64_C(1000000),
    /* 10^7 */ UINT64_C(10000000),
    /* 10^8 */ UINT64_C(100000000),
    /* 10^9 */ UINT64_C(1000000000),
    /* 10^10 */ UINT64_C(10000000000),
    /* 10^11 */ UINT64_C(100000000000),
    /* 10^12 */ UINT64_C(1000000000000),
    /* 10^13 */ UINT64_C(10000000000000),
    /* 10^14 */ UINT64_C(100000000000000),
    /* 10^15 */ UINT64_C(1000000000000000),
    /* 10^16 */ UINT64_C(10000000000000000),
    /* 10^17 */ UINT64_C(100000000000000000),
    /* 10^18 */ UINT64_C(1000000000000000000),
    /* 10^19 */ UINT64_C(10000000000000000000),
};

// For integers with `i` leading zero bits where 0 ≤ i ≤ 64, index `i` in
// this table gives floor(log10(2 ** i)).
constexpr std::array<uint8_t, 65> floor_log10_2exp = {
    19, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16, 16, 15, 15, 15,
    14, 14, 14, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10, 10,
    9,  9,  9,  8,  8,  8,  7,  7,  7,  7,  6,  6,  6,  5,  5,  5,  4,
    4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,
};

// For integers with `i` leading zero bits where 0 ≤ i ≤ 64, index `i` in this
// table gives the next power of 10.
constexpr auto next_power_of_10 = [] {
    std::array<uint64_t, 65> tab;
    for (size_t i = 0; i < tab.size(); i++)
        tab[i] = pow10i[floor_log10_2exp[i]];
    return tab;
}();

/// Computes the number of digits in `n` when written in base 10.
constexpr int digit_count_base10(uint64_t n)
{
    const int lzcnt = std::countl_zero(n);

    // For adjacent powers of two where there is a power of ten between them,
    // e.g. 64 ≤ 100 ≤ 128, we need to consult next_power_of_10 to determine
    // whether the number of digits must be adjusted by 1. For instance: the
    // numbers 99 and 101 both have 57 leading zero bits when written as 64-bit
    // integers in base 2, but 101 contains an extra digit when written in base
    // 10.
    const int extra = (n >= next_power_of_10[lzcnt]);

    return floor_log10_2exp[lzcnt] + extra;
}

/// Compute the modular inverse of `a` modulo `m`.
constexpr int64_t modinv(int64_t a, int64_t m)
{
    int64_t m0 = m;
    int64_t x0 = 0;
    int64_t x1 = 1;

    while (a > 1) {
        const int64_t q = a / m;
        std::tie(a, m) = std::pair(m, a % m);
        std::tie(x0, x1) = std::pair(x1 - q * x0, x0);
    }

    return x1 < 0 ? x1 + m0 : x1;
}

/// Implementation of a binary min-heap with a decrease-key operation as used
/// in Dijkstra's algorithm and A*. This only supports 32-bit integer keys and
/// has a fixed size by construction.
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

template <typename Predicate, typename Proj, typename... Args>
concept projected_predicate =
    std::predicate<Predicate, std::invoke_result_t<Proj, Args>...>;

/// Implementation of a d-ary min-heap with a decrease-key operation as used in
/// Dijkstra's algorithm and A*. This supports arbitrary (comparable) keys and
/// has a dynamic size.
template <typename T,
          std::invocable<T> Proj = std::identity,
          projected_predicate<Proj, T, T> Compare = std::less<>>
class DHeap {
    constexpr static uint32_t erased_handle_index = UINT32_MAX / 2;

public:
    constexpr static int D = 3;

    struct Handle {
        uint32_t index = erased_handle_index;
        explicit operator bool() const { return index != erased_handle_index; }
    };

private:
    std::vector<T> heap_;
    std::vector<uint32_t> heap_to_handle_;
    std::vector<uint32_t> handle_to_heap_;
    [[no_unique_address]] Proj proj_;
    [[no_unique_address]] Compare cmp_;

    void swap_elements(size_t i, size_t j)
    {
        std::swap(heap_[i], heap_[j]);
        std::swap(heap_to_handle_[i], heap_to_handle_[j]);
        handle_to_heap_[heap_to_handle_[i]] = i;
        handle_to_heap_[heap_to_handle_[j]] = j;
    }

    bool compare(size_t i, size_t j) { return cmp_(proj_(heap_[i]), proj_(heap_[j])); }

    void sift_down(size_t i)
    {
        while (true) {
            const auto c0 = D * i + 1;
            const auto c1 = D * i + 2;
            const auto c2 = D * i + 3;

            auto min_index = i;
            if (c2 < heap_.size()) {
                auto min01 = compare(c0, c1) ? c0 : c1;
                auto min23 = compare(c2, i) ? c2 : i;
                min_index = compare(min01, min23) ? min01 : min23;
            } else {
                if (c0 < heap_.size() && compare(c0, min_index))
                    min_index = c0;
                if (c1 < heap_.size() && compare(c1, min_index))
                    min_index = c1;
                if (c2 < heap_.size() && compare(c2, min_index))
                    min_index = c2;
            }

            if (min_index == i)
                break;

            swap_elements(min_index, i);
            i = min_index;
        }
    }

    void sift_up(size_t i)
    {
        while (i > 0) {
            auto p = (i - 1) / D;
            if (!compare(i, p))
                break;
            swap_elements(p, i);
            i = p;
        }
    }

    size_t get_handle_index_(Handle h)
    {
        ASSERT(h.index < handle_to_heap_.size());
        ASSERT(handle_to_heap_[h.index] != erased_handle_index);
        ASSERT(handle_to_heap_[h.index] < heap_.size());
        return handle_to_heap_[h.index];
    }

public:
    DHeap(Proj proj = {}, Compare cmp = {})
        : proj_(std::move(proj))
        , cmp_(std::move(cmp))
    {
    }

    bool empty() const { return heap_.empty(); }
    size_t size() const { return heap_.size(); }

    const T &top() const
    {
        ASSERT(!heap_.empty());
        return heap_.front();
    }

    void pop()
    {
        ASSERT(!heap_.empty());
        swap_elements(0, heap_.size() - 1);
        // handle_to_heap_[heap_.back().second] = erased_handle_index;
        heap_.pop_back();
        heap_to_handle_.pop_back();
        sift_down(0);
    }

    Handle push(T elem)
    {
        Handle handle{static_cast<uint32_t>(handle_to_heap_.size())};
        handle_to_heap_.push_back(heap_.size());
        heap_to_handle_.push_back(handle.index);
        heap_.push_back(std::move(elem));
        sift_up(heap_.size() - 1);
        return handle;
    }

    template <typename... Args>
    Handle emplace(Args &&...args)
    {
        Handle handle{static_cast<uint32_t>(handle_to_heap_.size())};
        handle_to_heap_.push_back(heap_.size());
        heap_to_handle_.push_back(handle.index);
        heap_.emplace_back(std::forward<Args>(args)...);
        sift_up(heap_.size() - 1);
        return handle;
    }

    T &get(Handle h) { return heap_[get_handle_index_(h)]; }
    void decrease_key(Handle h) { sift_up(get_handle_index_(h)); }
};

template <typename T>
constexpr T modulo(T x, T mod)
{
    auto r = x % mod;
    if (r < 0)
        r += mod;
    return r;
}

static inline std::vector<std::string_view> &
split(std::string_view s, std::vector<std::string_view> &out, char c);

template <typename T, typename Fn>
constexpr void find_numbers_impl(std::string_view s, Fn &&sink)
{
    while (true) {
        while (!s.empty() && s.front() != '-' && !(s.front() >= '0' && s.front() <= '9'))
            s.remove_prefix(1);
        if (s.empty())
            return;

        T value;
        auto r = std::from_chars(s.data(), s.data() + s.size(), value);
        assert(r.ec != std::errc::result_out_of_range);
        if (r.ec == std::errc()) {
            sink(static_cast<T &&>(value));
            s.remove_prefix(r.ptr - s.data());
        } else {
            s.remove_prefix(1);
        }
    }
}

template <typename T>
constexpr void find_numbers(std::string_view s, std::vector<T> &result)
{
    result.clear();
    find_numbers_impl<T>(s, [&](auto &&v) { result.push_back(static_cast<T &&>(v)); });
}

template <typename T, size_t N>
constexpr std::array<T, N> find_numbers_n(std::string_view s)
{
    std::array<T, N> result{};
    size_t i = 0;

    find_numbers_impl<T>(s, [&](auto &&v) {
        ASSERT(i < result.size());
        result[i++] = static_cast<T &&>(v);
    });

    ASSERT_MSG(i == N, "Expected {} numbers, but got only {}!", N, i);
    return result;
}

template <typename T>
constexpr std::vector<T> find_numbers(std::string_view s)
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
        __m256i m = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(s.data() + i));
        unsigned int mask =
            _mm256_movemask_epi8(_mm256_cmpeq_epi8(m, _mm256_set1_epi8(c)));

        for (; mask != 0; mask &= mask - 1) {
            int offset = std::countr_zero(mask);
            out.emplace_back(s.begin() + curr_field_start, s.begin() + i + offset);
            curr_field_start = i + offset + 1;
        }
    }

    for (; i + 15 < s.size(); i += 16) {
        __m128i m = _mm_loadu_si128(reinterpret_cast<const __m128i *>(s.data() + i));
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

static inline std::vector<std::string_view> split_lines(std::string_view s)
{
    std::vector<std::string_view> lines;
    split(s, lines, '\n');
    return lines;
}

template <typename Predicate>
constexpr std::vector<std::string_view> &
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

constexpr std::string_view strip(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\n'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\n'))
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

    constexpr Ndindex2DRange begin() { return *this; }
    constexpr sentinel end() { return {}; }

    constexpr Vec2<T> operator*() const { return {j, i}; }
    constexpr bool operator==(sentinel) const { return static_cast<size_t>(i) >= rows; }

    constexpr Ndindex2DRange &operator++()
    {
        if (static_cast<size_t>(j) < cols - 1) {
            j++;
        } else {
            i++;
            j = 0;
        }

        return *this;
    }

    constexpr Ndindex2DRange operator++(int) { return ++*this; }
};

template <typename T>
struct StridedIterator {
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using pointer = value_type *;
    using reference = value_type &;

    T *p;
    size_t stride;

    constexpr std::strong_ordering
    operator<=>(const StridedIterator &other) const = default;
    constexpr reference operator*() const { return *p; }
    constexpr pointer operator->() const { return p; }
    constexpr reference operator[](difference_type n) const { return p[stride * n]; }
    constexpr StridedIterator &operator++() { return p += stride, *this; }
    constexpr StridedIterator operator++(int)
    {
        StridedIterator copy(*this);
        p += stride;
        return copy;
    }
    constexpr StridedIterator &operator--() { return p -= stride, *this; }
    constexpr StridedIterator operator--(int)
    {
        StridedIterator copy(*this);
        p -= stride;
        return copy;
    }
    constexpr StridedIterator &operator+=(difference_type n)
    {
        return p += stride * n, *this;
    }
    constexpr StridedIterator operator+(difference_type n) const
    {
        return StridedIterator(*this) += n;
    }
    constexpr StridedIterator &operator-=(difference_type n)
    {
        return p -= stride * n, *this;
    }
    constexpr StridedIterator operator-(difference_type n) const
    {
        return StridedIterator(*this) -= n;
    }

    constexpr difference_type operator-(const StridedIterator &o) const
    {
        return (p - o.p) / static_cast<ssize_t>(stride);
    }

    constexpr friend StridedIterator operator+(difference_type n,
                                               const StridedIterator &it)
    {
        return it + n;
    }
};
static_assert(std::random_access_iterator<StridedIterator<int>>);

template <typename T>
struct StridedRange : std::ranges::view_interface<StridedRange<T>> {
    T *p;
    size_t n;
    size_t stride;

    constexpr StridedIterator<T> begin() { return {p, stride}; }
    constexpr StridedIterator<T> end() { return {p + n * stride, stride}; }
};
static_assert(std::ranges::random_access_range<StridedRange<int>>);

template <typename T>
struct Matrix {
    std::unique_ptr<T[]> data;
    size_t rows;
    size_t cols;

    constexpr Matrix() = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
    constexpr Matrix(size_t rows_, size_t cols_, T value = T())
        : data(new T[rows_ * cols_])
        , rows(rows_)
        , cols(cols_)
    {
        std::ranges::fill(all(), value);
    }
#pragma GCC diagnostic pop

    constexpr Matrix(const Matrix &other)
        : data(new T[other.rows * other.cols])
        , rows(other.rows)
        , cols(other.cols)
    {
        std::ranges::copy(other.all(), data.get());
    }

    constexpr Matrix &operator=(const Matrix &other)
    {
        Matrix m(other);
        std::swap(m, *this);
        return *this;
    }

    constexpr Matrix(Matrix &&) = default;
    constexpr Matrix &operator=(Matrix &&) = default;

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

    constexpr bool operator==(const Matrix &other) const noexcept
    {
        return std::ranges::equal(all(), other.all());
    }
    constexpr bool operator!=(const Matrix &other) const noexcept
    {
        return !(*this == other);
    }

    constexpr T &operator()(size_t i, size_t j)
    {
        DEBUG_ASSERT_MSG(i < rows && j < cols,
                         "({}, {}) is not a valid matrix entry (x<{}, y<{})", j, i, rows,
                         cols);
        return data[i * cols + j];
    }

    constexpr const T &operator()(size_t i, size_t j) const
    {
        DEBUG_ASSERT_MSG(i < rows && j < cols,
                         "({}, {}) is not a valid matrix entry (x<{}, y<{})", j, i, rows,
                         cols);
        return data[i * cols + j];
    }

    template <typename U>
    constexpr T &operator()(Vec2<U> p)
    {
        DEBUG_ASSERT_MSG(in_bounds(p), "{} is not a valid matrix entry (x<{}, y<{})", p,
                         cols, rows);
        return data[p.y * cols + p.x];
    }

    template <typename U>
    constexpr const T &operator()(Vec2<U> p) const
    {
        DEBUG_ASSERT_MSG(in_bounds(p), "{} is not a valid matrix entry (x<{}, y<{})", p,
                         cols, rows);
        return data[p.y * cols + p.x];
    }

    constexpr size_t size() const { return rows * cols; }

    constexpr std::span<T> all() { return {data.get(), data.get() + rows * cols}; }
    constexpr std::span<const T> all() const
    {
        return {data.get(), data.get() + rows * cols};
    }

    constexpr StridedRange<T> col(size_t i)
    {
        DEBUG_ASSERT_MSG(i < cols, "{} is not a valid column", i);
        return {{}, data.get() + i, rows, cols};
    }

    constexpr StridedRange<const T> col(size_t i) const
    {
        DEBUG_ASSERT_MSG(i < cols, "{} is not a valid column", i);
        return {{}, data.get() + i, rows, cols};
    }

    constexpr StridedRange<T> row(size_t i)
    {
        DEBUG_ASSERT_MSG(i < rows, "{} is not a valid row", i);
        return {{}, data.get() + i * cols, cols, 1};
    }

    constexpr StridedRange<const T> row(size_t i) const
    {
        DEBUG_ASSERT_MSG(i < rows, "{} is not a valid row", i);
        return {{}, data.get() + i * cols, cols, 1};
    }

    template <typename U = size_t>
    constexpr Ndindex2DRange<U> ndindex() const
    {
        return {rows, cols};
    }

    template <typename U>
    constexpr bool in_bounds(Vec2<U> p) const
    {
        using Unsigned = std::make_unsigned_t<U>;
        return static_cast<Unsigned>(p.x) < cols && static_cast<Unsigned>(p.y) < rows;
    }
};

template <typename T>
struct fmt::formatter<Matrix<T>> : fmt::formatter<T> {
    template <typename FormatContext>
    auto format(const Matrix<T> &m, FormatContext &ctx) const
    {
        for (size_t i = 0; i < m.rows; i++) {
            for (size_t j = 0; j < m.cols; j++)
                fmt::formatter<T>::format(m(i, j), ctx);
            fmt::format_to(ctx.out(), "\n");
        }

        return ctx.out();
    }
};

template <typename Container, typename Predicate>
static void erase_if(Container &v, Predicate &&predicate)
{
    auto it = std::remove_if(begin(v), end(v), std::forward<Predicate>(predicate));
    v.erase(it, end(v));
}

template <typename Container>
void erase_swap(Container &c, size_t i)
{
    std::swap(c[i], c.back());
    c.pop_back();
}

template <typename T>
constexpr static std::array<Vec2<T>, 4> neighbors4(Vec2<T> p)
{
    return {{
        p + Vec2<T>(0, -1),
        p + Vec2<T>(+1, 0),
        p + Vec2<T>(0, +1),
        p + Vec2<T>(-1, 0),
    }};
}

template <typename T, typename U>
static inplace_vector<Vec2<U>, 4> neighbors4(const Matrix<T> &chart, Vec2<U> p)
{
    inplace_vector<Vec2<U>, 4> result;
    for (auto n : neighbors4(p))
        if (chart.in_bounds(n))
            result.push_back(n);

    return result;
}

template <typename T>
constexpr static std::array<Vec2<T>, 8> neighbors8(Vec2<T> p)
{
    return {{
        p + Vec2<T>(-1, -1),
        p + Vec2<T>(-1, +0),
        p + Vec2<T>(-1, +1),
        p + Vec2<T>(+0, -1),
        p + Vec2<T>(+0, +1),
        p + Vec2<T>(+1, -1),
        p + Vec2<T>(+1, +0),
        p + Vec2<T>(+1, +1),
    }};
}

template <typename T, typename U>
static inplace_vector<Vec2<U>, 8> neighbors8(const Matrix<T> &grid, Vec2<U> p)
{
    inplace_vector<Vec2<U>, 8> result;
    for (auto n : neighbors8(p))
        if (grid.in_bounds(n))
            result.push_back(n);

    return result;
}

struct TuplelikeHasher {
    template <typename... Ts>
    size_t operator()(const std::tuple<Ts...> &tuple) const
    {
        size_t h = 0;
        std::apply([&]<typename... Elems>(
                       const auto &...elems) mutable { hash_combine(h, elems...); },
                   tuple);
        return h;
    }
};
