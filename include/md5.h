// Shared code between 2015/4, 2016/5 and 2016/14.

#pragma once

#include "common.h"
#include <hwy/highway.h>

namespace md5 {

using D = hn::FixedTag<uint32_t, 8>;
using VecT = hn::Vec<D>;
constexpr D d;

inline std::array<uint32_t, 8> mm256_u32x8(const VecT v)
{
    std::array<uint32_t, 8> result;
    hn::StoreU(v, d, result.data());
    return result;
}

/// Format a 32-bit integer as 8 hex digits.
inline void format_u32_hex(char *out, const uint32_t h)
{
    uint64_t v = h;

    // Swap the nibbles. (To see why, note that endianness affects the byte
    // order, not the order of nibbles within each byte; e.g. for 0x4d we want
    // to output '4' followed by 'd', i.e. the *second* nibble first.)
    v = ((v & 0x0f0f0f0f) << 4) | ((v & 0xf0f0f0f0) >> 4);

    // Expand each nibble to a full byte in the range 0x00-0x0f.
    v = ((v & 0xffff0000) << 16) | (v & 0x0000ffff);
    v = ((v & 0x0000ff00'0000ff00) << 8) | (v & 0x000000ff'000000ff);
    v = ((v & 0x00f000f0'00f000f0) << 4) | (v & 0x000f000f'000f000f);

    // Add 0x30 (ASCII '0') to each byte.
    const uint64_t ascii_digits0 = v + 0x30303030'30303030;

    // Nibbles between 0-9 are correct, but a-f need to be fixed up. Identify
    // these by adding 6, which causes them to carry into the next nibble.
    const uint64_t carries4 = v + 0x06060606'06060606;
    const uint64_t carries0 = (carries4 & 0x10101010'10101010) >> 4;

    // Add 0x27 (ASCII 'a' - 10 - 0x30) to the nibbles that need to be
    // fixed up, i.e. the ones that carried.
    const uint64_t ascii_hex_digits = ascii_digits0 + carries0 * 0x27;

    memcpy(out, &ascii_hex_digits, sizeof(ascii_hex_digits));
}

struct alignas(32) Result {
    VecT a;
    VecT b;
    VecT c;
    VecT d;

    std::array<std::array<uint32_t, 8>, 4> to_arrays() const
    {
        return {mm256_u32x8(a), mm256_u32x8(b), mm256_u32x8(c), mm256_u32x8(d)};
    }

    std::array<std::array<char, 32>, 8> to_hex() const
    {
        const std::array<uint32_t, 8> aa = mm256_u32x8(a);
        const std::array<uint32_t, 8> bb = mm256_u32x8(b);
        const std::array<uint32_t, 8> cc = mm256_u32x8(c);
        const std::array<uint32_t, 8> dd = mm256_u32x8(d);

        std::array<std::array<char, 32>, 8> result;
        for (size_t i = 0; i < 8; i++) {
            char *p = result[i].data();
            format_u32_hex(p, aa[i]);
            format_u32_hex(p + 8, bb[i]);
            format_u32_hex(p + 16, cc[i]);
            format_u32_hex(p + 24, dd[i]);
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
inline Result
hash_block(const Block32x16x8 &__restrict M, VecT a0, VecT b0, VecT c0, VecT d0)
{
    VecT A(a0);
    VecT B(b0);
    VecT C(c0);
    VecT D(d0);

#define F(b, c, d) ((b & c) | hn::AndNot(b, d))
#define G(b, c, d) ((d & b) | hn::AndNot(d, c))
#define H(b, c, d) (b ^ c ^ d)
#define I(b, c, d) (c ^ (b | hn::Not(d)))

#define QUARTER_ROUND(f, a, b, c, d, j, k, shift)                                        \
    do {                                                                                 \
        a += f(b, c, d);                                                                 \
        a += hn::Set(hn::DFromV<decltype(a)>(), K[k]);                                   \
        a += hn::Load(hn::DFromV<decltype(a)>(), &M.data[8 * (j)]);                      \
        a = hn::RotateLeft<shift>(a);                                                    \
        a += b;                                                                          \
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

    return Result{a0 + A, b0 + B, c0 + C, d0 + D};
}

inline Result hash_block(const Block32x16x8 &__restrict M)
{
    const VecT a0 = hn::Set(d, 0x67452301);
    const VecT b0 = hn::Set(d, 0xefcdab89);
    const VecT c0 = hn::Set(d, 0x98badcfe);
    const VecT d0 = hn::Set(d, 0x10325476);
    return hash_block(M, a0, b0, c0, d0);
}

inline Result
hash_block(const Block8x8x64 &__restrict chunks, VecT a0, VecT b0, VecT c0, VecT d0)
{
    // The input in `chunks` is eight 64-byte blocks laid out one after
    // another. The MD5 core loop expects memory to contain interleaved 4-byte
    // words from each block, so reshuffle the original input into that format.
    Block32x16x8 M = permute_input(chunks);

    return hash_block(M, a0, b0, c0, d0);
}

inline Result hash_block(const Block8x8x64 &__restrict chunks)
{
    const VecT a0 = hn::Set(d, 0x67452301);
    const VecT b0 = hn::Set(d, 0xefcdab89);
    const VecT c0 = hn::Set(d, 0x98badcfe);
    const VecT d0 = hn::Set(d, 0x10325476);
    return hash_block(chunks, a0, b0, c0, d0);
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
inline uint32_t leading_zero_mask(const hn::Vec<D> &hashes)
{
    const hn::Vec<D> mask = hn::Set(d, detail::make_leading_zero_mask<N>());
    return hn::BitsFromMask(d, hn::Eq(mask & hashes, hn::Zero(d)));
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
        return hash_block(messages);
    }
};

}
