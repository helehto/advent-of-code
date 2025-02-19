// Shared code between 2015/4 and 2016/5.

#pragma once

#include "common.h"

inline __m256i mm256_rol(const __m256i a, const int n)
{
    return _mm256_or_si256(_mm256_slli_epi32(a, n), _mm256_srli_epi32(a, 32 - n));
}

inline __m256i mm256_not(const __m256i v)
{
    return _mm256_xor_si256(v, _mm256_set1_epi32(-1));
}

// Hash eight single-block messages simultaneously. The lengths are given in
// `lengths`, and the return values is a vector containing the first 32-bit
// words of the eight resulting hashes.
inline __m256i md5_block_avx2(uint8_t *const __restrict messages,
                              const uint32_t *const __restrict lengths)
{
    // Padding and appending message length. (There is no scatter instruction
    // in AVX2, so this has to be a scalar loop.)
    for (int i = 0; i < 8; i++) {
        // The buffer is zero-initialized and the length of each message is
        // non-decreasing, so this is all that is needed to properly pad it for
        // each iteration.
        messages[64 * i + lengths[i]] = 0x80;

        // The message is never going to be more than 65536 bits.
        messages[64 * i + 56] = (lengths[i] << 3) & 0xff;
        messages[64 * i + 57] = (lengths[i] >> 5) & 0xff;
    }

    // The original input is eight 64-byte blocks laid out one after another.
    // The actual MD5 code below expects memory to contain interleaved 4-byte
    // words from each block, so reshuffle the original input into that
    // format.
    alignas(32) uint32_t M[16 * 8];
    {
        auto *W = reinterpret_cast<const uint32_t *>(messages);
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 8; j++) {
                M[8 * i + j] = W[16 * j + i];
            }
        }
    }

    __m256i A = _mm256_set1_epi32(0x67452301);
    __m256i B = _mm256_set1_epi32(0xefcdab89);
    __m256i C = _mm256_set1_epi32(0x98badcfe);
    __m256i D = _mm256_set1_epi32(0x10325476);

#define F(b, c, d) _mm256_or_si256(_mm256_and_si256(b, c), _mm256_andnot_si256(b, d))
#define G(b, c, d) _mm256_or_si256(_mm256_and_si256(d, b), _mm256_andnot_si256(d, c))
#define H(b, c, d) _mm256_xor_si256(b, _mm256_xor_si256(c, d))
#define I(b, c, d) _mm256_xor_si256(c, _mm256_or_si256(b, mm256_not(d)))

#define QUARTER_ROUND(f, a, b, c, d, j, k, shift)                                        \
    do {                                                                                 \
        a = _mm256_add_epi32(a, f(b, c, d));                                             \
        a = _mm256_add_epi32(a, _mm256_set1_epi32(K[k]));                                \
        a = _mm256_add_epi32(a, _mm256_load_si256((const __m256i *)(&M[8 * (j)])));      \
        a = mm256_rol(a, shift);                                                         \
        a = _mm256_add_epi32(a, b);                                                      \
    } while (0)

    static constexpr uint32_t K[] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
        0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
        0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
        0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
        0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
        0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
    };

    // Quarter-round 1 (F):
    for (int i = 0; i < 16; i += 4) {
        QUARTER_ROUND(F, A, B, C, D, i + 0, i + 0, 7);
        QUARTER_ROUND(F, D, A, B, C, i + 1, i + 1, 12);
        QUARTER_ROUND(F, C, D, A, B, i + 2, i + 2, 17);
        QUARTER_ROUND(F, B, C, D, A, i + 3, i + 3, 22);
    }

    // Quarter-round 2 (G):
    for (int i = 16; i < 32; i += 4) {
        QUARTER_ROUND(G, A, B, C, D, (5 * (i + 0) + 1) & 15, i + 0, 5);
        QUARTER_ROUND(G, D, A, B, C, (5 * (i + 1) + 1) & 15, i + 1, 9);
        QUARTER_ROUND(G, C, D, A, B, (5 * (i + 2) + 1) & 15, i + 2, 14);
        QUARTER_ROUND(G, B, C, D, A, (5 * (i + 3) + 1) & 15, i + 3, 20);
    }

    // Quarter-round 3 (H):
    for (int i = 32; i < 48; i += 4) {
        QUARTER_ROUND(H, A, B, C, D, (3 * (i + 0) + 5) & 15, i + 0, 4);
        QUARTER_ROUND(H, D, A, B, C, (3 * (i + 1) + 5) & 15, i + 1, 11);
        QUARTER_ROUND(H, C, D, A, B, (3 * (i + 2) + 5) & 15, i + 2, 16);
        QUARTER_ROUND(H, B, C, D, A, (3 * (i + 3) + 5) & 15, i + 3, 23);
    }

    // Quarter-round 4 (I):
    for (int i = 48; i < 64; i += 4) {
        QUARTER_ROUND(I, A, B, C, D, (7 * (i + 0)) & 15, i + 0, 6);
        QUARTER_ROUND(I, D, A, B, C, (7 * (i + 1)) & 15, i + 1, 10);
        QUARTER_ROUND(I, C, D, A, B, (7 * (i + 2)) & 15, i + 2, 15);
        QUARTER_ROUND(I, B, C, D, A, (7 * (i + 3)) & 15, i + 3, 21);
    }

#undef F
#undef G
#undef H
#undef I
#undef QUARTER_ROUND

    return _mm256_add_epi32(A, _mm256_set1_epi32(0x67452301));
}

constexpr uint64_t ctpow(uint64_t base, int exp)
{
    uint64_t result = 1;
    for (int i = 0; i < exp; i++)
        result *= base;
    return result;
}

[[gnu::noinline]] inline char *to_chars(char *p, int n)
{
    // 00-99 packed into a single string.
    static constexpr const char packed_digits2[] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

    if (n < 10) [[unlikely]] {
        *p = '0' + n % 10;
        return p + 1;
    }

    // Determine the number of base 10 digits to be written. This way, we can
    // write the digits into the right place immediately and not have to
    // reverse or move them afterwards.
    const int ndigits = digit_count_base10(n);

    char *q = p + ndigits;

    for (; n >= 100; n /= 100, q -= 2) {
        const int r = n % 100;
        q[-2] = packed_digits2[2 * r];
        q[-1] = packed_digits2[2 * r + 1];
    }

    // At this point, `n` is at most 99. Instead of dividing by 10 to find the
    // final digits, split this into two cases: 0 < n < 10 and 10 ≤ n < 100.
    if (n >= 10) {
        q[-2] = packed_digits2[2 * n + 0];
        q[-1] = packed_digits2[2 * n + 1];
    } else {
        q[-1] = n + '0';
    }

    return {p + ndigits};
}

template <size_t N>
constexpr static uint32_t make_leading_zero_mask()
{
    uint32_t result = (1 << (4 * (N & ~1))) - 1;
    if (N & 1)
        result |= 0xf << (4 * N);
    return result;
}
static_assert(make_leading_zero_mask<5>() == 0xf0ffff);
static_assert(make_leading_zero_mask<6>() == 0xffffff);

struct md5_avx_state {
    std::array<char, 512> buffer;
    std::string_view prefix;

    md5_avx_state(std::string_view pfx)
        : prefix(pfx)
    {
        buffer.fill(0);
        for (int i = 0; i < 512; i += 64)
            memcpy(&buffer[i], prefix.data(), prefix.size());
    }

    /// Compute eight MD5 hashes with [block, block+1, ..., block+7] appended
    /// to each block. The internal buffers are not cleared between calls, so
    /// the block number must never decrease between calls to this method.
    __m256i run(const int block)
    {
        std::array<uint32_t, 8> lengths;

        for (int i = 0; i < 8; i++) {
            char *p = buffer.data() + 64 * i;
            char *q = to_chars(p + prefix.size(), block + i);
            lengths[i] = q - p;
        }

        return md5_block_avx2((uint8_t *)buffer.data(), lengths.data());
    }

    /// Return a 8-bit mask with a bit set for each element in `hashes` that
    /// has at least `N` leading zeroes when written as hexadecimal.
    template <size_t N>
    static uint32_t leading_zero_mask(const __m256i hashes)
    {
        const auto mask = _mm256_set1_epi32(make_leading_zero_mask<N>());
        const auto zero = _mm256_setzero_si256();
        const __m256i eq = _mm256_cmpeq_epi32(_mm256_and_si256(mask, hashes), zero);
        return _mm256_movemask_ps(_mm256_castsi256_ps(eq));
    }
};
