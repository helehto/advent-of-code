#pragma once

#include "inplace_vector.h"
#include "macros.h"
#include "small_vector.h"
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
    size_t operator()(const Vec2<T> &p) const noexcept
    {
        if constexpr (sizeof(T) == 1) {
            return _mm_crc32_u64(0, std::bit_cast<uint16_t>(p));
        } else if constexpr (sizeof(T) == 2) {
            return _mm_crc32_u64(0, std::bit_cast<uint32_t>(p));
        } else if constexpr (sizeof(T) == 4) {
            return _mm_crc32_u64(0, std::bit_cast<uint64_t>(p));
        } else if constexpr (sizeof(T) == 8) {
            return _mm_crc32_u64(_mm_crc32_u64(0, p.x), p.y);
        } else {
            static_assert(false);
        }
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

/// Compute the next lexicographic permutation of the bits in `v`.
///
/// Taken from <https://graphics.stanford.edu/~seander/bithacks.html>.
constexpr size_t next_bit_permutation(size_t v)
{
    const auto t = v | (v - 1);
    return (t + 1) | (((~t & -~t) - 1) >> (std::countr_zero(v) + 1));
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

/// Compute a^b mod m.
constexpr int64_t modexp(int64_t a, int64_t b, const int64_t m)
{
    int64_t result = 1;
    for (; b; b >>= 1) {
        if (b & 1)
            result = (a * result) % m;
        a = (a * a) % m;
    }
    return result;
}

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

template <typename T>
constexpr void find_numbers_impl(std::string_view s, auto &&sink)
{
    const char *p = s.data();
    const char *end = p + s.size();

    while (true) {
        [[maybe_unused]] int mul_overflow, add_overflow;

        while (true) {
            if (p == end) [[unlikely]]
                return;
            if (*p >= '0' && *p <= '9')
                break;
            p++;
        }

        const char *first = p;

        T value{};
        do {
            mul_overflow = __builtin_mul_overflow(value, 10, &value);
            DEBUG_ASSERT_MSG(!mul_overflow, "Overflow: {} * {}", value, 10);
            add_overflow = __builtin_add_overflow(value, *p - '0', &value);
            DEBUG_ASSERT_MSG(!add_overflow, "Overflow: {} + {}", value, *p - '0');
            p++;
        } while (p != end && *p >= '0' && *p <= '9');

        if constexpr (std::is_signed_v<T>) {
            if (first != s.data() && first[-1] == '-') {
                DEBUG_ASSERT(!__builtin_sub_overflow_p(0, value, T{}));
                value = -value;
            }
        }

        sink(value);
    }
}

template <typename T>
constexpr void find_numbers(std::string_view s, small_vector_base<T> &result)
{
    result.clear();
    find_numbers_impl<T>(s, [&](auto &&v) { result.push_back(static_cast<T &&>(v)); });
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

template <typename T>
constexpr small_vector<T> find_numbers_small(std::string_view s)
{
    small_vector<T> result;
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
    operator<=>(const StridedIterator &other) const noexcept = default;
    constexpr reference operator*() const noexcept { return *p; }
    constexpr pointer operator->() const noexcept { return p; }
    constexpr reference operator[](difference_type n) const noexcept
    {
        return p[stride * n];
    }
    constexpr StridedIterator &operator++() noexcept { return p += stride, *this; }
    constexpr StridedIterator operator++(int) noexcept
    {
        StridedIterator copy(*this);
        p += stride;
        return copy;
    }
    constexpr StridedIterator &operator--() noexcept { return p -= stride, *this; }
    constexpr StridedIterator operator--(int) noexcept
    {
        StridedIterator copy(*this);
        p -= stride;
        return copy;
    }
    constexpr StridedIterator &operator+=(difference_type n) noexcept
    {
        return p += stride * n, *this;
    }
    constexpr StridedIterator operator+(difference_type n) const noexcept
    {
        return StridedIterator(*this) += n;
    }
    constexpr StridedIterator &operator-=(difference_type n) noexcept
    {
        return p -= stride * n, *this;
    }
    constexpr StridedIterator operator-(difference_type n) const noexcept
    {
        return StridedIterator(*this) -= n;
    }

    constexpr difference_type operator-(const StridedIterator &o) const noexcept
    {
        return (p - o.p) / static_cast<ssize_t>(stride);
    }

    constexpr friend StridedIterator operator+(difference_type n,
                                               const StridedIterator &it) noexcept
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

    constexpr StridedIterator<T> begin() const noexcept { return {p, stride}; }
    constexpr StridedIterator<T> end() const noexcept { return {p + n * stride, stride}; }
};
static_assert(std::ranges::random_access_range<StridedRange<int>>);

/// Base class for shared functionality between Matrix and MatrixView. Assumes
/// that data is stored in row-major order and that each row is contiguous in
/// memory (i.e. no padding between rows).
template <typename Derived>
struct MatrixBase {
    constexpr bool operator==(this auto &&self, const Derived &other) noexcept
    {
        return std::ranges::equal(self.all(), other.all());
    }
    constexpr bool operator!=(this auto &&self, const Derived &other) noexcept
    {
        return !self.operator==(other);
    }

    constexpr size_t size(this auto &&self) noexcept { return self.rows * self.cols; }
    constexpr size_t rows(this auto &&self) noexcept { return self.rows; }
    constexpr size_t cols(this auto &&self) noexcept { return self.cols; }

    constexpr auto all(this auto &&self) noexcept
    {
        return std::span(self.data(), self.data() + self.rows * self.cols);
    }
    constexpr auto col(this auto &&self, size_t i) noexcept
    {
        DEBUG_ASSERT_MSG(i < self.cols, "{} is not a valid column", i);
        return StridedRange({}, self.data() + i, self.rows, self.cols);
    }
    constexpr auto row(this auto &&self, size_t i) noexcept
    {
        DEBUG_ASSERT_MSG(i < self.rows, "{} is not a valid row", i);
        return std::span(self.data() + i * self.cols, self.data() + (i + 1) * self.cols);
    }

    constexpr auto &&operator()(this auto &&self, size_t i, size_t j) noexcept
    {
        DEBUG_ASSERT_MSG(i < self.rows && j < self.cols,
                         "({}, {}) is not a valid matrix entry (x<{}, y<{})", j, i,
                         self.rows, self.cols);
        return self.data()[i * self.cols + j];
    }

    template <typename U>
    constexpr auto &&operator()(this auto &&self, Vec2<U> p) noexcept
    {
        DEBUG_ASSERT_MSG(self.in_bounds(p), "{} is not a valid matrix entry (x<{}, y<{})",
                         p, self.cols, self.rows);
        return self.data()[p.y * self.cols + p.x];
    }

    template <typename IndexTy = size_t>
    constexpr Ndindex2DRange<IndexTy> ndindex(this auto &&self) noexcept
    {
        return {self.rows, self.cols};
    }

    template <typename IndexTy>
    constexpr bool in_bounds(this auto &&self, Vec2<IndexTy> p) noexcept
    {
        using Unsigned = std::make_unsigned_t<IndexTy>;
        return static_cast<Unsigned>(p.x) < self.cols &&
               static_cast<Unsigned>(p.y) < self.rows;
    }
};

/// Non-owning view of a 2D matrix.
template <typename T>
struct MatrixView : MatrixBase<MatrixView<T>> {
    T *data_;
    size_t rows;
    size_t cols;

    using value_type = T;

    constexpr MatrixView() noexcept = default;

    constexpr MatrixView(T *data, size_t rows, size_t cols) noexcept
        : data_(data)
        , rows(rows)
        , cols(cols)
    {
    }

    T *data() const noexcept { return data_; }
};

/// Owning container for a 2D matrix.
template <typename T>
struct Matrix : MatrixBase<Matrix<T>> {
    std::unique_ptr<T[]> data_;
    size_t rows;
    size_t cols;

    using value_type = T;

    constexpr Matrix() = default;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
    constexpr Matrix(size_t rows_, size_t cols_, T value = T())
        : data_(std::make_unique_for_overwrite<T[]>(rows_ * cols_))
        , rows(rows_)
        , cols(cols_)
    {
        std::ranges::fill(this->all(), value);
    }

    constexpr Matrix(const Matrix &other)
        : data_(std::make_unique_for_overwrite<T[]>(other.rows * other.cols))
        , rows(other.rows)
        , cols(other.cols)
    {
        std::ranges::copy(other.all(), data());
    }
#pragma GCC diagnostic pop

    constexpr Matrix &operator=(const Matrix &other)
    {
        if (this == &other)
            return *this;

        if (rows == other.rows && cols == other.cols) {
            std::ranges::copy(other.all(), data());
            return *this;
        }

        Matrix m(other);
        std::swap(m, *this);
        return *this;
    }

    constexpr Matrix(Matrix &&other) noexcept
        : data_(std::exchange(other.data_, nullptr))
        , rows(std::exchange(other.rows, 0))
        , cols(std::exchange(other.cols, 0))
    {
    }

    constexpr Matrix &operator=(Matrix &&other) noexcept
    {
        data_ = std::exchange(other.data_, nullptr);
        rows = std::exchange(other.rows, 0);
        cols = std::exchange(other.cols, 0);
        return *this;
    }

    // Allow implicit conversion to MatrixView, akin to how std::vector<T> is
    // implicitly convertible to std::span<T>.
    operator MatrixView<T>() noexcept { return MatrixView<T>(data_.get(), rows, cols); }
    operator MatrixView<const T>() const noexcept
    {
        return MatrixView<const T>(data_.get(), rows, cols);
    }

    template <typename Proj = std::identity>
    static Matrix from_lines(std::span<const std::string_view> lines, Proj proj = {})
    {
        Matrix m(lines.size(), lines[0].size());
        T *p = m.data();
        for (size_t i = 0; i < lines.size(); i++) {
            for (size_t j = 0; j < lines[0].size(); j++)
                *p++ = proj(lines[i][j]);
        }
        return m;
    }

    constexpr Matrix<T>
    padded(const size_t pad_rows, const size_t pad_cols, const T &pad_value)
    {
        Matrix<T> result(rows + 2 * pad_rows, cols + 2 * pad_cols, pad_value);

        for (size_t i = 0; i < rows; ++i)
            std::ranges::copy(this->row(i), result.row(i + pad_cols).begin() + pad_rows);

        return result;
    }

    constexpr Matrix<T> padded(const size_t pad, const T &pad_value)
    {
        return padded(pad, pad, pad_value);
    }

    T *data() noexcept { return data_.get(); }
    const T *data() const noexcept { return data_.get(); }
};

/// Concept that matches any kind of matrix (either an owning Matrix or a
/// non-owning MatrixView), regardless of the element type.
template <typename M>
concept MatrixConcept = std::derived_from<M, MatrixBase<M>>;
static_assert(MatrixConcept<Matrix<int>>);
static_assert(MatrixConcept<MatrixView<int>>);

template <MatrixConcept M>
struct fmt::formatter<M> : fmt::formatter<std::remove_cv_t<typename M::value_type>> {
    auto format(const M &m, auto &ctx) const
    {
        for (size_t i = 0; i < m.rows; i++) {
            for (size_t j = 0; j < m.cols; j++)
                fmt::formatter<std::remove_cv_t<typename M::value_type>>::format(m(i, j),
                                                                                 ctx);
            fmt::format_to(ctx.out(), "\n");
        }

        return ctx.out();
    }
};

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

template <typename U>
inline inplace_vector<Vec2<U>, 4> neighbors4(const MatrixConcept auto &chart, Vec2<U> p)
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

template <typename U>
inline inplace_vector<Vec2<U>, 8> neighbors8(const MatrixConcept auto &grid, Vec2<U> p)
{
    inplace_vector<Vec2<U>, 8> result;
    for (auto n : neighbors8(p))
        if (grid.in_bounds(n))
            result.push_back(n);

    return result;
}

class CrcHasher {
private:
    template <std::integral T>
    static void update_crc(const std::byte *&p, uint64_t &crc) noexcept
    {
        T u;
        std::memcpy(&u, p, sizeof(T));
        crc = _mm_crc32_u64(crc, u);
        p += sizeof(T);
    }

    static size_t hash_bytes(const std::byte *p, size_t n) noexcept
    {
        uint64_t result = 0;

        for (; n >= 8; n -= 8)
            update_crc<uint64_t>(p, result);

        if (n) {
            alignas(8) std::array<std::byte, 8> tail{};
            for (size_t j = 0; n--; ++j)
                tail[j] = *p++;
            result = _mm_crc32_u64(result, std::bit_cast<uint64_t>(tail));
        }

        return result;
    }

    template <size_t N>
    static size_t hash_bytes_fixed(const std::byte *p) noexcept
    {
        uint64_t result = 0;
        for (size_t i = 0; i < N / 8; ++i)
            update_crc<uint64_t>(p, result);

        constexpr size_t rest = N % 8;
        if constexpr (rest & 4)
            update_crc<uint32_t>(p, result);
        if constexpr (rest & 2)
            update_crc<uint16_t>(p, result);
        if constexpr (rest & 1)
            update_crc<uint8_t>(p, result);

        return result;
    }

public:
    static size_t operator()(std::integral auto value) noexcept
    {
        static_assert(sizeof(value) <= 8);
        return _mm_crc32_u64(0, static_cast<uint64_t>(value));
    }

    static size_t operator()(const float value) noexcept
    {
        return _mm_crc32_u64(0, std::bit_cast<uint32_t>(value));
    }

    static size_t operator()(const double value) noexcept
    {
        return _mm_crc32_u64(0, std::bit_cast<uint64_t>(value));
    }

    template <typename T, size_t Extent>
    static size_t operator()(std::span<T, Extent> s) noexcept
    {
        static_assert(std::has_unique_object_representations_v<T>,
                      "Cannot hash type: it has non-unique object representations "
                      "(possibly padding?)");

        auto bytes = std::as_bytes(s);
        if constexpr (decltype(bytes)::extent == std::dynamic_extent) {
            return hash_bytes(bytes.data(), bytes.size());
        } else {
            return hash_bytes_fixed<decltype(bytes)::extent>(bytes.data());
        }
    }

    static size_t operator()(std::string_view s) noexcept
    {
        return hash_bytes(std::bit_cast<std::byte *>(s.data()), s.size());
    }

    static size_t operator()(const std::string &s) noexcept
    {
        return hash_bytes(std::bit_cast<std::byte *>(s.data()), s.size());
    }

    template <typename T>
    static size_t operator()(const T &value) noexcept
        requires(!std::integral<T>)
    {
        static_assert(std::has_unique_object_representations_v<T>,
                      "Cannot hash type: it has non-unique object representations "
                      "(possibly padding?)");

        return hash_bytes_fixed<sizeof(T)>(std::bit_cast<std::byte *>(&value));
    }
};
