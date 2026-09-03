#pragma once

#include <aoc/macros.h>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <hwy/highway.h>

namespace detail {

namespace hn = hwy::HWY_NAMESPACE;

/// A 256-bit unsigned integer, with a subset of the usual operations.
struct uint256 {
    using D = hn::CappedTag<uint64_t, 4>;
    static constexpr D d{};
    static_assert(hn::MaxLanes(d) == 2 || hn::MaxLanes(d) == 4);

    // This array holds the 64-bit limbs, stored as little-endian (i.e.
    // limbs[0] is least significant).
    //
    // Some operations, e.g. bitwise ops and shifts, load and operate on the
    // limbs as a single 256-bit vector when possible. This helps the compiler
    // to fuse operations, e.g. ~a&b into a vpandn instruction, or any
    // 3-operand bitwise function into a single vpternlog instruction if
    // AVX-512 is available.
    //
    // For targets without 256-bit vector support, the limbs are operated on as
    // two 128-bit vectors.
    alignas(32) std::array<uint64_t, 4> limbs;

    uint256() noexcept = default;

    uint256(uint64_t v) noexcept
    {
        const hn::Vec<D> zero = hn::Zero(d);
        hn::Vec<D> limbs = hn::InsertLane(zero, 0, v);
        hn::Store(limbs, d, &this->limbs[0]);
        if constexpr (hn::MaxLanes(d) < 4)
            hn::Store(zero, d, &this->limbs[2]);
    }

    operator bool() const
    {
        if constexpr (hn::MaxLanes(d) < 4) {
            return !hn::AllBits0(d, hn::Load(d, &limbs[0])) ||
                   !hn::AllBits0(d, hn::Load(d, &limbs[2]));
        } else {
            return !hn::AllBits0(d, hn::Load(d, &limbs[0]));
        }
    }

    constexpr static uint256 ones(size_t n) noexcept
    {
        DEBUG_ASSERT(n <= 256);
        uint256 result(0);
        for (size_t i = 0; i < n; ++i)
            result.set_bit(i);
        return result;
    }

    constexpr void set_bit(size_t n) noexcept
    {
        DEBUG_ASSERT(n < 256);
        limbs[n / 64] |= UINT64_C(1) << (n % 64);
    }

    constexpr void clear_bit(size_t n) noexcept
    {
        DEBUG_ASSERT(n < 256);
        limbs[n / 64] &= ~(UINT64_C(1) << (n % 64));
    }

    constexpr bool test_bit(size_t n) const noexcept
    {
        DEBUG_ASSERT(n < 256);
        return (limbs[n / 64] & (UINT64_C(1) << (n % 64))) != 0;
    }

    uint256 operator|(const uint256 &other) const noexcept
    {
        uint256 result = *this;
        result |= other;
        return result;
    }

    uint256 &operator|=(const uint256 &other) noexcept
    {
        hn::Vec<D> a = hn::Load(d, &limbs[0]);
        hn::Vec<D> b = hn::Load(d, &other.limbs[0]);
        hn::Store(a | b, d, &limbs[0]);
        if constexpr (hn::MaxLanes(d) < 4) {
            a = hn::Load(d, &limbs[2]);
            b = hn::Load(d, &other.limbs[2]);
            hn::Store(a | b, d, &limbs[2]);
        }
        return *this;
    }

    uint256 operator&(const uint256 &other) const noexcept
    {
        uint256 result = *this;
        result &= other;
        return result;
    }

    uint256 &operator&=(const uint256 &other) noexcept
    {
        hn::Vec<D> a = hn::Load(d, &limbs[0]);
        hn::Vec<D> b = hn::Load(d, &other.limbs[0]);
        hn::Store(a & b, d, &limbs[0]);
        if constexpr (hn::MaxLanes(d) < 4) {
            a = hn::Load(d, &limbs[2]);
            b = hn::Load(d, &other.limbs[2]);
            hn::Store(a & b, d, &limbs[2]);
        }
        return *this;
    }

    uint256 operator^(const uint256 &other) const noexcept
    {
        uint256 result = *this;
        result ^= other;
        return result;
    }

    uint256 &operator^=(const uint256 &other) noexcept
    {
        hn::Vec<D> a = hn::Load(d, &limbs[0]);
        hn::Vec<D> b = hn::Load(d, &other.limbs[0]);
        hn::Store(a ^ b, d, &limbs[0]);
        if constexpr (hn::MaxLanes(d) < 4) {
            a = hn::Load(d, &limbs[2]);
            b = hn::Load(d, &other.limbs[2]);
            hn::Store(a ^ b, d, &limbs[2]);
        }
        return *this;
    }

    uint256 operator~() const noexcept
    {
        uint256 result;
        hn::Vec<D> v = hn::Load(d, &limbs[0]);
        hn::Store(hn::Not(v), d, &result.limbs[0]);
        if constexpr (hn::MaxLanes(d) < 4) {
            v = hn::Load(d, &limbs[2]);
            hn::Store(hn::Not(v), d, &result.limbs[2]);
        }
        return result;
    }

    static hn::Vec<D> shift_left1_vec(const hn::Vec<D> v) noexcept
    {
        const hn::Vec<D> shifted = hn::ShiftLeft<1>(v);
        const hn::Vec<D> carries = hn::Slide1Up(d, hn::ShiftRight<63>(v));
        return shifted | carries;
    }

    uint256 shift_left1() const noexcept
    {
        uint256 result;

        const uint64_t carry = limbs[1] >> 63; // only used for 2x128-bit case
        hn::Vec<D> a = hn::Load(d, &limbs[0]);
        hn::Store(shift_left1_vec(a), d, &result.limbs[0]);

        if constexpr (hn::MaxLanes(d) < 4) {
            a = hn::Load(d, &limbs[2]);
            hn::Store(shift_left1_vec(a), d, &result.limbs[2]);
            result.limbs[2] = (result.limbs[2] & ~UINT64_C(1)) | carry;
        }

        return result;
    }

    /// Rotate left by 1 bit, wrapping around at the given bit width.
    uint256 rotate_left1(size_t n) const noexcept
    {
        DEBUG_ASSERT(n > 0 && n <= 256);
        const bool wrapped_bit = test_bit(n - 1);
        uint256 result = shift_left1();

        if (wrapped_bit) {
            result.set_bit(0);
            result.clear_bit(n);
        }

        return result;
    }

    static hn::Vec<D> shift_right1_vec(const hn::Vec<D> v) noexcept
    {
        const hn::Vec<D> shifted = hn::ShiftRight<1>(v);
        const hn::Vec<D> carries = hn::Slide1Down(d, hn::ShiftLeft<63>(v));
        return shifted | carries;
    }

    uint256 shift_right1() const noexcept
    {
        uint256 result;

        const uint64_t carry = limbs[2] & 1; // only used for 2x128-bit case
        hn::Vec<D> a = hn::Load(d, &limbs[0]);
        hn::Store(shift_right1_vec(a), d, &result.limbs[0]);

        if constexpr (hn::MaxLanes(d) < 4) {
            a = hn::Load(d, &limbs[2]);
            hn::Store(shift_right1_vec(a), d, &result.limbs[2]);
            result.limbs[1] = (result.limbs[1] & ~(UINT64_C(1) << 63)) | (carry << 63);
        }

        return result;
    }

    /// Rotate right by 1 bit, wrapping around at the given bit width.
    uint256 rotate_right1(size_t n) const noexcept
    {
        DEBUG_ASSERT(n > 0 && n <= 256);
        const bool wrapped_bit = test_bit(0);
        uint256 result = shift_right1();

        if (wrapped_bit)
            result.set_bit(n - 1);

        return result;
    }
};

constexpr int countl_zero(const uint256 &v) noexcept
{
    if (v.limbs[3])
        return 0 * 64 + std::countl_zero(v.limbs[3]);
    else if (v.limbs[2])
        return 1 * 64 + std::countl_zero(v.limbs[2]);
    else if (v.limbs[1])
        return 2 * 64 + std::countl_zero(v.limbs[1]);
    else
        return 3 * 64 + std::countl_zero(v.limbs[0]);
}

constexpr int countr_zero(const uint256 &v) noexcept
{
    if (v.limbs[0])
        return 0 * 64 + std::countr_zero(v.limbs[0]);
    else if (v.limbs[1])
        return 1 * 64 + std::countr_zero(v.limbs[1]);
    else if (v.limbs[2])
        return 2 * 64 + std::countr_zero(v.limbs[2]);
    else
        return 3 * 64 + std::countr_zero(v.limbs[3]);
}

constexpr int popcount(const uint256 &v) noexcept
{
    return std::popcount(v.limbs[0]) + std::popcount(v.limbs[1]) +
           std::popcount(v.limbs[2]) + std::popcount(v.limbs[3]);
}

}

using detail::uint256;
