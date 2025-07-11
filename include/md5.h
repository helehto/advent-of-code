// Shared code between 2015/4, 2016/5 and 2016/14.

#pragma once

#include "common.h"

namespace md5 {

inline __m256i mm256_rol(const __m256i a, const int n)
{
    return _mm256_or_si256(_mm256_slli_epi32(a, n), _mm256_srli_epi32(a, 32 - n));
}

inline __m256i mm256_not(const __m256i v)
{
    return _mm256_xor_si256(v, _mm256_set1_epi32(-1));
}

inline __m256i mm256_swapnibbles(const __m256i v)
{
    const __m256i mask_hi = _mm256_set1_epi32(0xf0f0f0f0);
    const __m256i mask_lo = _mm256_set1_epi32(0x0f0f0f0f);
    const __m256i result_lo = _mm256_srli_epi32(_mm256_and_si256(v, mask_hi), 4);
    const __m256i result_hi = _mm256_slli_epi32(_mm256_and_si256(v, mask_lo), 4);
    return _mm256_or_si256(result_lo, result_hi);
}

inline std::array<uint32_t, 8> mm256_u32x8(const __m256i v)
{
    std::array<uint32_t, 8> result;
    _mm256_storeu_si256((__m256i *)&result, v);
    return result;
}

struct alignas(32) Result {
    __m256i a;
    __m256i b;
    __m256i c;
    __m256i d;

    std::array<std::array<uint32_t, 8>, 4> to_arrays() const
    {
        return {mm256_u32x8(a), mm256_u32x8(b), mm256_u32x8(c), mm256_u32x8(d)};
    }

    std::array<std::array<char, 32>, 8> to_hex() const
    {
        const __m128i hex_chars = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7',
                                                '8', '9', 'a', 'b', 'c', 'd', 'e', 'f');

        auto output_u32 = [&](char *p, const uint32_t h) {
            // Expand each nibble to a full byte in the range 0x00-0x0f so that
            // it can be used as a lookup index in _mm_shuffle_epi8() below.
            const uint64_t bytes = _pdep_u64(h, 0x0f0f0f0f0f0f0f0f);
            const __m128i vbytes = _mm_cvtsi64x_si128(bytes);

            // Map each byte to its corresponding hex character.
            const __m128i output = _mm_shuffle_epi8(hex_chars, vbytes);

            uint64_t out64 = _mm_cvtsi128_si64(output);
            memcpy(p, &out64, sizeof(out64));
        };

        // First, swap the nibbles in all bytes. (To see why, note that
        // endianness affects the byte order, *not* the order of nibbles within
        // each byte; e.g. for 0x4d we want to output '4' followed by 'd', i.e.
        // the *second* nibble first.)
        const std::array<uint32_t, 8> aa = mm256_u32x8(mm256_swapnibbles(a));
        const std::array<uint32_t, 8> bb = mm256_u32x8(mm256_swapnibbles(b));
        const std::array<uint32_t, 8> cc = mm256_u32x8(mm256_swapnibbles(c));
        const std::array<uint32_t, 8> dd = mm256_u32x8(mm256_swapnibbles(d));

        std::array<std::array<char, 32>, 8> result;
        for (size_t i = 0; i < 8; i++) {
            char *p = result[i].data();
            output_u32(p, aa[i]);
            output_u32(p + 8, bb[i]);
            output_u32(p + 16, cc[i]);
            output_u32(p + 24, dd[i]);
        }

        return result;
    }
};

struct alignas(32) Block8x8x64 {
    char data[512];
};

struct alignas(32) Block32x16x8 {
    uint32_t data[16 * 8];

    // Returns a reference to the `i`:th character of the `m`:th message in
    // this block.
    char &at(size_t m, size_t i)
    {
        DEBUG_ASSERT(m < 8);
        DEBUG_ASSERT(i < 64);
        return reinterpret_cast<char *>(data)[32 * (i / 4) + 4 * m + i % 4];
    }
};

// Reshuffles `input` which contains eight 64-byte blocks laid out one after
// another into interleaved 4-byte words from each block.
inline Block32x16x8 permute_input(const Block8x8x64 &input)
{
    Block32x16x8 result;

    // GCC does a decent job of vectorizing this into a bunch of shuffles
    // (vpermi2d and vpermt2d); doing it by hand is unlikely to yield any
    // significant speedup.
    auto *input32 = reinterpret_cast<const uint32_t *>(&input.data);
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 8; j++)
            result.data[8 * i + j] = input32[16 * j + i];

    return result;
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
inline void prepare_final_blocks(Block8x8x64 &__restrict messages,
                                 const uint32_t (&__restrict length_bytes)[8])
{
    for (int i = 0; i < 8; i++) {
        // The buffer is assumed to be padded and the length of each message is
        // non-decreasing, so all we need to do is to insert the 1 bit (0x80)
        // and add the length in bits.
        messages.data[64 * i + length_bytes[i]] = 0x80;

        // The message is never going to be more than 65536 bits.
        messages.data[64 * i + 56] = (length_bytes[i] << 3) & 0xff;
        messages.data[64 * i + 57] = (length_bytes[i] >> 5) & 0xff;
    }
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
inline void prepare_final_blocks(Block32x16x8 &__restrict messages,
                                 const uint32_t (&__restrict length_bytes)[8])
{
    for (int m = 0; m < 8; m++) {
        // The buffer is assumed to be padded and the length of each message is
        // non-decreasing, so all we need to do is to insert the 1 bit (0x80)
        // and add the length in bits.
        messages.at(m, length_bytes[m]) = 0x80;

        // The message is never going to be more than 65536 bits.
        messages.at(m, 56) = (length_bytes[m] << 3) & 0xff;
        messages.at(m, 57) = (length_bytes[m] >> 5) & 0xff;
    }
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
inline void prepare_final_blocks(Block8x8x64 &__restrict messages,
                                 std::optional<size_t> x80_offset,
                                 const uint32_t (&__restrict length_bytes)[8])
{
    for (int i = 0; i < 8; i++) {
        if (x80_offset)
            messages.data[64 * i + *x80_offset] = 0x80;

        // The message is never going to be more than 65536 bits.
        messages.data[64 * i + 56] = (length_bytes[i] << 3) & 0xff;
        messages.data[64 * i + 57] = (length_bytes[i] >> 5) & 0xff;
    }
}

// Hash eight blocks simultaneously. The return values is an array where index
// `k` contains the `k`:th 32-bit word of the eight resulting hashes.
inline Result do_block_avx2(
    const Block32x16x8 &__restrict M, __m256i a0, __m256i b0, __m256i c0, __m256i d0)
{
    __m256i A = a0;
    __m256i B = b0;
    __m256i C = c0;
    __m256i D = d0;

#define F(b, c, d) _mm256_or_si256(_mm256_and_si256(b, c), _mm256_andnot_si256(b, d))
#define G(b, c, d) _mm256_or_si256(_mm256_and_si256(d, b), _mm256_andnot_si256(d, c))
#define H(b, c, d) _mm256_xor_si256(b, _mm256_xor_si256(c, d))
#define I(b, c, d) _mm256_xor_si256(c, _mm256_or_si256(b, mm256_not(d)))

#define QUARTER_ROUND(f, a, b, c, d, j, k, shift)                                        \
    do {                                                                                 \
        a = _mm256_add_epi32(a, f(b, c, d));                                             \
        a = _mm256_add_epi32(a, _mm256_set1_epi32(K[k]));                                \
        a = _mm256_add_epi32(a, _mm256_load_si256((const __m256i *)(&M.data[8 * (j)]))); \
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

    return Result{
        .a = _mm256_add_epi32(a0, A),
        .b = _mm256_add_epi32(b0, B),
        .c = _mm256_add_epi32(c0, C),
        .d = _mm256_add_epi32(d0, D),
    };
}

inline Result do_block_avx2(const Block32x16x8 &__restrict M)
{
    __m256i a0 = _mm256_set1_epi32(0x67452301);
    __m256i b0 = _mm256_set1_epi32(0xefcdab89);
    __m256i c0 = _mm256_set1_epi32(0x98badcfe);
    __m256i d0 = _mm256_set1_epi32(0x10325476);
    return do_block_avx2(M, a0, b0, c0, d0);
}

inline Result do_block_avx2(
    const Block8x8x64 &__restrict chunks, __m256i a0, __m256i b0, __m256i c0, __m256i d0)
{
    // The input in `chunks` is eight 64-byte blocks laid out one after
    // another. The MD5 core loop expects memory to contain interleaved 4-byte
    // words from each block, so reshuffle the original input into that format.
    Block32x16x8 M = permute_input(chunks);

    return do_block_avx2(M, a0, b0, c0, d0);
}

inline Result do_block_avx2(const Block8x8x64 &__restrict chunks)
{
    __m256i a0 = _mm256_set1_epi32(0x67452301);
    __m256i b0 = _mm256_set1_epi32(0xefcdab89);
    __m256i c0 = _mm256_set1_epi32(0x98badcfe);
    __m256i d0 = _mm256_set1_epi32(0x10325476);
    return do_block_avx2(chunks, a0, b0, c0, d0);
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

    return p + ndigits;
}

namespace detail {

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

}

/// Return a 8-bit mask with a bit set for each element in `hashes` that
/// has at least `N` leading zeroes when written as hexadecimal.
template <size_t N>
inline uint32_t leading_zero_mask(const __m256i hashes)
{
    const auto mask = _mm256_set1_epi32(detail::make_leading_zero_mask<N>());
    const auto zero = _mm256_setzero_si256();
    const __m256i eq = _mm256_cmpeq_epi32(_mm256_and_si256(mask, hashes), zero);
    return _mm256_movemask_ps(_mm256_castsi256_ps(eq));
}

struct State {
    Block8x8x64 messages{};
    std::string_view prefix;

    State(std::string_view pfx)
        : prefix(pfx)
    {
        for (int i = 0; i < 512; i += 64)
            memcpy(&messages.data[i], prefix.data(), prefix.size());
    }

    /// Compute eight MD5 hashes with [block, block+1, ..., block+7] appended
    /// to each block. The internal buffers are not cleared between calls, so
    /// the block number must never decrease between calls to this method.
    Result run(const int block)
    {
        uint32_t lengths[8];

        for (int i = 0; i < 8; i++) {
            char *p = messages.data + 64 * i;
            char *q = to_chars(p + prefix.size(), block + i);
            lengths[i] = q - p;
        }

        prepare_final_blocks(messages, lengths);
        return do_block_avx2(messages);
    }
};

}
