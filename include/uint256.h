#pragma once

#include "macros.h"
#include <array>
#include <bit>
#include <hwy/highway.h>

namespace detail {

namespace hn = hwy::HWY_NAMESPACE;

/// A 256-bit unsigned integer, with a subset of the usual operations.
struct uint256 {
    using D = hn::FixedTag<uint64_t, 4>;

    // This array holds the 64-bit limbs, stored as little-endian (i.e.
    // limbs[0] is least significant).
    //
    // Some operations, e.g. bitwise ops and shifts, load and operate on the
    // limbs as a single 256-bit vector. This helps the compiler to fuse
    // operations, e.g. ~a&b into a vpandn instruction, or any 3-operand
    // bitwise function into a single vpternlog instruction if AVX-512 is
    // available.
    alignas(32) std::array<uint64_t, 4> limbs;

    uint256() noexcept = default;

    uint256(hn::Vec<D> v) noexcept { hn::Store(v, D(), limbs.data()); }

    uint256(uint64_t v) noexcept
    {
        hn::Vec<D> limbs = hn::InsertLane(hn::Zero(D()), 0, v);
        hn::Store(limbs, D(), this->limbs.data());
    }

    operator bool() const { return !hn::AllBits0(D(), hn::Load(D(), limbs.data())); }

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
        const hn::Vec<D> a = hn::Load(D(), limbs.data());
        const hn::Vec<D> b = hn::Load(D(), other.limbs.data());
        hn::Store(a | b, D(), limbs.data());
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
        const hn::Vec<D> a = hn::Load(D(), limbs.data());
        const hn::Vec<D> b = hn::Load(D(), other.limbs.data());
        hn::Store(a & b, D(), limbs.data());
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
        const hn::Vec<D> a = hn::Load(D(), limbs.data());
        const hn::Vec<D> b = hn::Load(D(), other.limbs.data());
        hn::Store(a ^ b, D(), limbs.data());
        return *this;
    }

    uint256 operator~() const noexcept
    {
        return uint256(hn::Not(hn::Load(D(), limbs.data())));
    }

    uint256 shift_left1() const noexcept
    {
        const hn::Vec<D> a = hn::Load(D(), limbs.data());
        const hn::Vec<D> shifted = hn::ShiftLeft<1>(a);

        // Carry bits between each 64-bit integer.
        // TODO: Is there a better way to do this?
        const hn::Vec<D> hi = hn::ShiftRight<63>(a);
        alignas(32) uint64_t masks[] = {0, uint64_t(-1), uint64_t(-1), uint64_t(-1)};
        const hn::Vec<D> carries = hn::Slide1Up(D(), hi) & hn::Load(D(), masks);

        return uint256(shifted | carries);
    }

    /// Rotate left by 1 bit, wrapping around at the given bit width.
    uint256 rotate_left1(size_t n) const noexcept
    {
        DEBUG_ASSERT(n > 0 && n <= 256);
        const bool wrapped_bit = test_bit(n - 1);
        uint256 result = shift_left1();

        // TODO: Is there a better way to do this?
        if (wrapped_bit) {
            result.set_bit(0);
            result.clear_bit(n);
        }

        return result;
    }

    uint256 shift_right1() const noexcept
    {
        const hn::Vec<D> a = hn::Load(D(), limbs.data());
        const hn::Vec<D> shifted = hn::ShiftRight<1>(a);

        // Carry bits between each 64-bit integer.
        // TODO: Is there a better way to do this?
        const hn::Vec<D> lo = hn::ShiftLeft<63>(a);
        alignas(32) uint64_t masks[] = {uint64_t(-1), uint64_t(-1), uint64_t(-1), 0};
        const hn::Vec<D> carries = hn::Slide1Down(D(), lo) & hn::Load(D(), masks);

        return uint256(shifted | carries);
    }

    /// Rotate right by 1 bit, wrapping around at the given bit width.
    uint256 rotate_right1(size_t n) const noexcept
    {
        DEBUG_ASSERT(n > 0 && n <= 256);
        const bool wrapped_bit = test_bit(0);
        uint256 result = shift_right1();

        // TODO: Is there a better way to do this?
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
