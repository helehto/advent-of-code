#pragma once

#include <algorithm>
#include <aoc/bitmanip.h>
#include <aoc/inplace_vector.h>
#include <aoc/macros.h>
#include <array>
#include <bit>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fmt/base.h>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <string_view>
#include <sys/types.h>
#include <tuple>
#include <type_traits>
#include <utility>

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
            return crc32_u64(0, std::bit_cast<uint16_t>(p));
        } else if constexpr (sizeof(T) == 2) {
            return crc32_u64(0, std::bit_cast<uint32_t>(p));
        } else if constexpr (sizeof(T) == 4) {
            return crc32_u64(0, std::bit_cast<uint64_t>(p));
        } else if constexpr (sizeof(T) == 8) {
            return crc32_u64(crc32_u64(0, p.x), p.y);
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

/// Computes the number of digits in `n` when written in base 10.
constexpr int digit_count_base10(uint64_t n)
{
    const size_t lzcnt = std::countl_zero(n);

    // For integers with `i` leading zero bits where 0 ≤ i ≤ 64, index `i` in
    // this table gives floor(log10(2 ** i)).
    static constexpr std::array<uint8_t, 65> floor_log10_2exp = {
        19, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16, 16, 15, 15, 15,
        14, 14, 14, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10, 10,
        9,  9,  9,  8,  8,  8,  7,  7,  7,  7,  6,  6,  6,  5,  5,  5,  4,
        4,  4,  4,  3,  3,  3,  2,  2,  2,  1,  1,  1,  0,  0,
    };

    // For adjacent powers of two where there is a power of ten between them,
    // we may need to add an extra digit. For instance: the numbers 99 and 100
    // both have 57 leading zero bits when written as 64-bit integers in base
    // 2, as they both lie between 64 and and 128, but 100 contains one more
    // digit when written in base 10.
    //
    // This lookup table is used to determine whether this is the case.
    //
    // For any integer with `i` leading zero bits where 0 ≤ i ≤ 64, index `i`
    // in this table gives the smallest power of ten greater than it. The one
    // exception is index 64, corresponding to the integer 0, which still has
    // one digit when written in base 10.
    static constexpr auto need_extra_digit = [&] consteval {
        std::array<uint64_t, 65> tab;
        for (size_t i = 0; i < 64; i++)
            tab[i] = pow10i[floor_log10_2exp[i]];
        tab[64] = 0;
        return tab;
    }();

    const int extra = (n >= need_extra_digit[lzcnt]);
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
        std::span a = self.all();
        std::span b = other.all();
        return std::equal(a.begin(), a.end(), b.begin(), b.end());
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

    constexpr T *data() const noexcept { return data_; }
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
        std::span dest = this->all();
        std::fill(dest.begin(), dest.end(), value);
    }

    constexpr Matrix(const Matrix &other)
        : data_(std::make_unique_for_overwrite<T[]>(other.rows * other.cols))
        , rows(other.rows)
        , cols(other.cols)
    {
        std::span dest = other.all();
        std::copy(dest.begin(), dest.end(), this->data());
    }
#pragma GCC diagnostic pop

    constexpr Matrix &operator=(const Matrix &other)
    {
        if (this == &other)
            return *this;

        if (rows == other.rows && cols == other.cols) {
            std::span src = other.all();
            std::copy(src.begin(), src.end(), this->data());
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

    constexpr Matrix<T> padded(const size_t pad_u,
                               const size_t pad_d,
                               const size_t pad_l,
                               const size_t pad_r,
                               const T &pad_value)
    {
        Matrix<T> result(rows + pad_u + pad_d, cols + pad_l + pad_r, pad_value);

        for (size_t i = 0; i < rows; ++i) {
            std::span row = this->row(i);
            std::copy(row.begin(), row.end(), result.row(i + pad_u).begin() + pad_l);
        }

        return result;
    }

    constexpr Matrix<T>
    padded(const size_t pad_rows, const size_t pad_cols, const T &pad_value)
    {
        return padded(pad_rows, pad_rows, pad_cols, pad_cols, pad_value);
    }

    constexpr Matrix<T> padded(const size_t pad, const T &pad_value)
    {
        return padded(pad, pad, pad, pad, pad_value);
    }

    constexpr T *data() noexcept { return data_.get(); }
    constexpr const T *data() const noexcept { return data_.get(); }
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
            if (i)
                fmt::format_to(ctx.out(), "\n");
            for (size_t j = 0; j < m.cols; j++)
                fmt::formatter<std::remove_cv_t<typename M::value_type>>::format(m(i, j),
                                                                                 ctx);
        }

        return ctx.out();
    }
};

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
