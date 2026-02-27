// Shared code between 2015/4, 2016/5 and 2016/14.

#pragma once

#include "common.h"
#include <hwy/highway.h>

namespace md5 {

using D = hn::ScalableTag<uint32_t>;
using VecT = hn::Vec<D>;
constexpr D d;
constexpr size_t max_lanes = hn::MaxLanes(D());

inline size_t lanes()
{
    return hn::Lanes(d);
}

// Fixed by the MD5 algorithm.
constexpr size_t words_per_block = 16;
constexpr size_t bytes_per_block = words_per_block * sizeof(uint32_t);

struct HWY_ALIGN_MAX Result {
    // Vectors of a, b, c, d stored one after another.
    HWY_ALIGN_MAX uint32_t data[4 * max_lanes];

    VecT a() const { return hn::Load(D(), data + 0 * lanes()); }
    VecT b() const { return hn::Load(D(), data + 1 * lanes()); }
    VecT c() const { return hn::Load(D(), data + 2 * lanes()); }
    VecT d() const { return hn::Load(D(), data + 3 * lanes()); }

    void set_a(VecT v) { hn::Store(v, D(), data + 0 * lanes()); }
    void set_b(VecT v) { hn::Store(v, D(), data + 1 * lanes()); }
    void set_c(VecT v) { hn::Store(v, D(), data + 2 * lanes()); }
    void set_d(VecT v) { hn::Store(v, D(), data + 3 * lanes()); }

    static inline std::array<uint32_t, max_lanes> to_array(const VecT v)
    {
        std::array<uint32_t, max_lanes> result;
        hn::StoreU(v, D(), result.data());
        return result;
    }

    std::array<std::array<uint32_t, max_lanes>, 4> to_arrays() const
    {
        return {to_array(a()), to_array(b()), to_array(c()), to_array(d())};
    }
};

/// 64-byte blocks laid out sequentially one after another; as many blocks as
/// we (potentially) have SIMD lanes.
struct alignas(32) SequentialBlocks {
    char data[bytes_per_block * max_lanes];
};

/// Interleaved 4-byte words from 64-byte blocks. This is the format fed into
/// the core hash_block() function to compute multiple hashes in parallel.
struct alignas(32) InterleavedBlocks {
    uint32_t data[words_per_block * max_lanes];
};

// Interleave 4-byte words from 64-byte blocks laid out one after in memory.
inline InterleavedBlocks interleave(const SequentialBlocks &input)
{
    InterleavedBlocks result;
    const size_t lanes = ::md5::lanes();

    // GCC does a decent job of vectorizing this into a bunch of shuffles
    // (vpermi2d and vpermt2d); doing it by hand is unlikely to yield any
    // significant speedup.
    auto *input32 = reinterpret_cast<const uint32_t *>(&input.data);
    for (size_t i = 0; i < words_per_block; i++)
        for (size_t j = 0; j < lanes; j++)
            result.data[lanes * i + j] = input32[words_per_block * j + i];

    return result;
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
inline void prepare_final_blocks(SequentialBlocks &HWY_RESTRICT messages,
                                 const uint32_t *HWY_RESTRICT length_bytes)
{
    for (size_t i = 0; i < lanes(); i++) {
        // The buffer is assumed to be padded and the length of each message is
        // non-decreasing, so all we need to do is to insert the 1 bit (0x80)
        // and add the length in bits.
        messages.data[bytes_per_block * i + length_bytes[i]] = 0x80;

        // The message is never going to be more than 65536 bits.
        messages.data[bytes_per_block * i + 56] = (length_bytes[i] << 3) & 0xff;
        messages.data[bytes_per_block * i + 57] = (length_bytes[i] >> 5) & 0xff;
    }
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
inline void prepare_final_blocks(SequentialBlocks &HWY_RESTRICT messages,
                                 std::optional<size_t> x80_offset,
                                 const uint32_t *HWY_RESTRICT length_bytes)
{
    for (size_t i = 0; i < lanes(); i++) {
        if (x80_offset)
            messages.data[bytes_per_block * i + *x80_offset] = 0x80;

        // The message is never going to be more than 65536 bits.
        messages.data[bytes_per_block * i + 56] = (length_bytes[i] << 3) & 0xff;
        messages.data[bytes_per_block * i + 57] = (length_bytes[i] >> 5) & 0xff;
    }
}

// Hash eight blocks simultaneously. The return values is an array where index
// `k` contains the `k`:th 32-bit word of the eight resulting hashes.
inline Result
hash_block(const InterleavedBlocks &HWY_RESTRICT M, VecT a0, VecT b0, VecT c0, VecT d0)
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
        a += hn::LoadU(hn::DFromV<decltype(a)>(), &M.data[lanes() * (j)]);               \
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

    Result r;
    r.set_a(A + a0);
    r.set_b(B + b0);
    r.set_c(C + c0);
    r.set_d(D + d0);
    return r;
}

inline Result hash_block(const InterleavedBlocks &HWY_RESTRICT M)
{
    const VecT a0 = hn::Set(d, 0x67452301);
    const VecT b0 = hn::Set(d, 0xefcdab89);
    const VecT c0 = hn::Set(d, 0x98badcfe);
    const VecT d0 = hn::Set(d, 0x10325476);
    return hash_block(M, a0, b0, c0, d0);
}

inline Result hash_block(
    const SequentialBlocks &HWY_RESTRICT chunks, VecT a0, VecT b0, VecT c0, VecT d0)
{
    // The input in `chunks` is eight 64-byte blocks laid out one after
    // another. The MD5 core loop expects memory to contain interleaved 4-byte
    // words from each block, so reshuffle the original input into that format.
    InterleavedBlocks M = interleave(chunks);

    return hash_block(M, a0, b0, c0, d0);
}

inline Result hash_block(const SequentialBlocks &HWY_RESTRICT chunks)
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
consteval uint64_t make_leading_zero_mask()
{
    uint64_t result = (UINT64_C(1) << (4 * (N & ~1))) - 1;
    if (N & 1)
        result |= UINT64_C(0xf) << (4 * N);
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
    SequentialBlocks messages{};
    std::string_view prefix;

    State(std::string_view pfx)
        : prefix(pfx)
    {
        for (size_t i = 0; i < lanes(); ++i)
            memcpy(&messages.data[i * bytes_per_block], prefix.data(), prefix.size());
    }

    /// Compute eight MD5 hashes with [block, block+1, ..., block+7] appended
    /// to each block. The internal buffers are not cleared between calls, so
    /// the block number must never decrease between calls to this method.
    Result run(const int block)
    {
        uint32_t lengths[max_lanes];

        for (size_t i = 0; i < lanes(); i++) {
            char *p = messages.data + 64 * i;
            char *q = to_chars(p + prefix.size(), block + i);
            lengths[i] = q - p;
        }

        prepare_final_blocks(messages, lengths);
        return hash_block(messages);
    }
};

}
