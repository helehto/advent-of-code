#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if defined(__BMI2__)
#include <immintrin.h> // bextr/bzhi/pdep/pext
#define DISPATCH_BMI2(intrinsic, fallback, ...)                                          \
    if !consteval {                                                                      \
        return intrinsic(__VA_ARGS__);                                                   \
    } else {                                                                             \
        return fallback(__VA_ARGS__);                                                    \
    }
#else
#define DISPATCH_BMI2(intrinsic, fallback, ...) return fallback(__VA_ARGS__)
#endif

#if defined(__SSE4_2__)
#include <immintrin.h> // crc32
#define DISPATCH_SSE42(intrinsic, fallback, ...)                                         \
    if !consteval {                                                                      \
        return intrinsic(__VA_ARGS__);                                                   \
    } else {                                                                             \
        return fallback(__VA_ARGS__);                                                    \
    }
#else
#define DISPATCH_SSE42(intrinsic, fallback, ...) return fallback(__VA_ARGS__)
#endif

/// constexpr-friendly fallback implementation of bzhi from BMI2.
template <typename T>
constexpr T bzhi_fallback(T value, unsigned int index)
{
    const T mask = index < 8 * sizeof(T) ? ((T(1) << index) - 1) : T(-1);
    return value & mask;
}
constexpr uint32_t bzhi_u32(uint32_t value, unsigned int index)
{
    DISPATCH_BMI2(_bzhi_u32, bzhi_fallback<uint32_t>, value, index);
}
constexpr uint64_t bzhi_u64(uint64_t value, unsigned int index)
{
    DISPATCH_BMI2(_bzhi_u64, bzhi_fallback<uint64_t>, value, index);
}

/// constexpr-friendly fallback implementation of bextr from BMI.
template <typename T>
constexpr T bextr_fallback(T value, unsigned int index, unsigned int count)
{
    return index < 8 * sizeof(T) ? bzhi_fallback<T>(value >> index, count) : 0;
}
constexpr uint32_t bextr_u32(uint32_t value, unsigned int index, unsigned int count)
{
    DISPATCH_BMI2(_bextr_u32, bextr_fallback<uint32_t>, value, index, count);
}
constexpr uint64_t bextr_u64(uint64_t value, unsigned int index, unsigned int count)
{
    DISPATCH_BMI2(_bextr_u64, bextr_fallback<uint64_t>, value, index, count);
}

/// constexpr-friendly fallback implementation of pdep from BMI2.
template <typename T>
constexpr T pdep_fallback(T value, T mask)
{
    T result = 0;

    for (size_t i = 0, k = 0; i < 8 * sizeof(T); ++i) {
        const auto m = T(1) << i;
        const auto v = T(1) << k;
        if (mask & m) {
            result |= (value & v) ? m : 0;
            k++;
        }
    }

    return result;
}
constexpr uint32_t pdep_u32(uint32_t value, uint32_t mask)
{
    DISPATCH_BMI2(_pdep_u32, pdep_fallback<uint32_t>, value, mask);
}
constexpr uint64_t pdep_u64(uint64_t value, uint64_t mask)
{
    DISPATCH_BMI2(_pdep_u64, pdep_fallback<uint64_t>, value, mask);
}

/// constexpr-friendly fallback implementation of pext from BMI2.
template <typename T>
constexpr T pext_fallback(T value, T mask)
{
    T result = 0;

    for (size_t i = 0, k = 0; i < 8 * sizeof(T); ++i) {
        const auto m = T(1) << i;
        if (mask & m) {
            result |= (value & m) ? (T(1) << k) : 0;
            k++;
        }
    }

    return result;
}
constexpr uint32_t pext_u32(uint32_t value, uint32_t mask)
{
    DISPATCH_BMI2(_pext_u32, pext_fallback<uint32_t>, value, mask);
}
constexpr uint64_t pext_u64(uint64_t value, uint64_t mask)
{
    DISPATCH_BMI2(_pext_u64, pext_fallback<uint64_t>, value, mask);
}

/// Compute the next lexicographic permutation of the bits in `v`.
///
/// Taken from <https://graphics.stanford.edu/~seander/bithacks.html>.
constexpr size_t next_bit_permutation(size_t v)
{
    const auto t = v | (v - 1);
    return (t + 1) | (((~t & -~t) - 1) >> (std::countr_zero(v) + 1));
}

/// Reflect the bits in `v` across the middle of the value.
constexpr uint8_t bit_reflect(uint8_t v)
{
    v = ((v & 0xf0) >> 4) | ((v & 0x0f) << 4);
    v = ((v & 0xcc) >> 2) | ((v & 0x33) << 2);
    v = ((v & 0xaa) >> 1) | ((v & 0x55) << 1);
    return v;
}
constexpr uint16_t bit_reflect(uint16_t v)
{
    v = std::byteswap(v);
    v = ((v & 0xf0f0) >> 4) | ((v & 0x0f0f) << 4);
    v = ((v & 0xcccc) >> 2) | ((v & 0x3333) << 2);
    v = ((v & 0xaaaa) >> 1) | ((v & 0x5555) << 1);
    return v;
}
constexpr uint32_t bit_reflect(uint32_t v)
{
    v = std::byteswap(v);
    v = ((v & 0xf0f0f0f0) >> 4) | ((v & 0x0f0f0f0f) << 4);
    v = ((v & 0xcccccccc) >> 2) | ((v & 0x33333333) << 2);
    v = ((v & 0xaaaaaaaa) >> 1) | ((v & 0x55555555) << 1);
    return v;
}
constexpr uint64_t bit_reflect(uint64_t v)
{
    v = std::byteswap(v);
    v = ((v & 0xf0f0f0f0f0f0f0f0) >> 4) | ((v & 0x0f0f0f0f0f0f0f0f) << 4);
    v = ((v & 0xcccccccccccccccc) >> 2) | ((v & 0x3333333333333333) << 2);
    v = ((v & 0xaaaaaaaaaaaaaaaa) >> 1) | ((v & 0x5555555555555555) << 1);
    return v;
}

/// constexpr-friendly fallback implementation of crc32 from SSE4.2.
template <typename T>
constexpr uint32_t crc32_fallback(uint32_t crc, T v)
{
    using U = std::conditional_t<sizeof(T) <= 4, uint64_t, __uint128_t>;
    constexpr size_t bits = 8 * sizeof(T);
    U result = (static_cast<U>(bit_reflect(v)) << 32) ^
               (static_cast<U>(bit_reflect(crc)) << bits);
    for (int i = bits - 1; i >= 0; --i)
        result ^= result & (U(1) << (i + 32)) ? U(0x1edc6f41) << i : 0;
    return bit_reflect(static_cast<uint32_t>(result));
}
constexpr uint32_t crc32_u8(uint32_t crc, uint8_t v)
{
    DISPATCH_SSE42(_mm_crc32_u8, crc32_fallback<uint8_t>, crc, v);
}
constexpr uint32_t crc32_u16(uint32_t crc, uint16_t v)
{
    DISPATCH_SSE42(_mm_crc32_u16, crc32_fallback<uint16_t>, crc, v);
}
constexpr uint32_t crc32_u32(uint32_t crc, uint32_t v)
{
    DISPATCH_SSE42(_mm_crc32_u32, crc32_fallback<uint32_t>, crc, v);
}
constexpr uint32_t crc32_u64(uint32_t crc, uint64_t v)
{
    DISPATCH_SSE42(_mm_crc32_u64, crc32_fallback<uint64_t>, crc, v);
}

#undef DISPATCH_BMI2
#undef DISPATCH_SSE42
