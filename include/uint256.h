#pragma once

/// A 256-bit unsigned integer, with a subset of the usual operations.
struct uint256 {
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

    uint256(__m256i v) noexcept { _mm256_store_si256((__m256i *)limbs.data(), v); }

    uint256(uint64_t v) noexcept
    {
        const __m128i v128 = _mm_cvtsi64_si128(v);
        const __m256i v256 = _mm256_castsi128_si256(v128);
        _mm256_store_si256((__m256i *)limbs.data(), v256);
    }

    operator bool() const
    {
        const __m256i a = _mm256_load_si256((const __m256i *)limbs.data());
        return _mm256_testz_si256(a, a) == 0;
    }

    constexpr static uint256 ones(size_t n) noexcept
    {
        DEBUG_ASSERT(n <= 256);
        uint256 result;
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
        const __m256i a = _mm256_load_si256((const __m256i *)limbs.data());
        const __m256i b = _mm256_load_si256((const __m256i *)other.limbs.data());
        const __m256i result = _mm256_or_si256(a, b);
        _mm256_store_si256((__m256i *)limbs.data(), result);
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
        const __m256i a = _mm256_load_si256((const __m256i *)limbs.data());
        const __m256i b = _mm256_load_si256((const __m256i *)other.limbs.data());
        const __m256i result = _mm256_and_si256(a, b);
        _mm256_store_si256((__m256i *)limbs.data(), result);
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
        const __m256i a = _mm256_load_si256((const __m256i *)limbs.data());
        const __m256i b = _mm256_load_si256((const __m256i *)other.limbs.data());
        const __m256i result = _mm256_xor_si256(a, b);
        _mm256_store_si256((__m256i *)limbs.data(), result);
        return *this;
    }

    uint256 operator~() const noexcept
    {
        const __m256i a = _mm256_load_si256((const __m256i *)limbs.data());
        return uint256(_mm256_xor_si256(a, _mm256_set1_epi8(-1)));
    }

    uint256 shift_left1() const noexcept
    {
        const __m256i a = _mm256_load_si256((const __m256i *)limbs.data());
        const __m256i shifted = _mm256_slli_epi64(a, 1);

        // Carry bits between each 64-bit integer.
        // TODO: Is there a better way to do this?
        const __m256i hi = _mm256_srli_epi64(a, 63);
        const __m256i carries =
            _mm256_and_si256(_mm256_permute4x64_epi64(hi, 0b10'01'00'00),
                             _mm256_set_epi64x(-1, -1, -1, 0));
        const __m256i result = _mm256_or_si256(shifted, carries);

        return uint256(result);
    }

    /// Rotate left by 1 bit, wrapping around at the given bit width.
    uint256 rotate_left1(size_t n) const noexcept
    {
        DEBUG_ASSERT(n > 0 && n <= 256);
        const bool wrapped_bit = test_bit(n - 1);
        uint256 result = shift_left1();

        // TODO: This can be probably done entirely using AVX2 instead of using
        // a separate scalar path for the wraparound bit, using a lookup table
        // for the shuffle and mask.
        if (wrapped_bit) {
            result.set_bit(0);
            result.clear_bit(n);
        }

        return result;
    }

    uint256 shift_right1() const noexcept
    {
        const __m256i a = _mm256_load_si256((const __m256i *)limbs.data());
        const __m256i shifted = _mm256_srli_epi64(a, 1);

        // Carry bits between each 64-bit integer.
        // TODO: Is there a better way to do this?
        const __m256i lo = _mm256_slli_epi64(a, 63);
        const __m256i carries =
            _mm256_and_si256(_mm256_permute4x64_epi64(lo, 0b00'11'10'01),
                             _mm256_set_epi64x(0, -1, -1, -1));
        const __m256i result = _mm256_or_si256(shifted, carries);

        return uint256(result);
    }

    /// Rotate right by 1 bit, wrapping around at the given bit width.
    uint256 rotate_right1(size_t n) const noexcept
    {
        DEBUG_ASSERT(n > 0 && n <= 256);
        const bool wrapped_bit = test_bit(0);
        uint256 result = shift_right1();

        // TODO: This can be probably done entirely using AVX2 instead of using
        // a separate scalar path for the wraparound bit, using a lookup table
        // for the shuffle and mask.
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
