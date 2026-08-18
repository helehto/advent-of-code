// Shared code between 2015/4, 2016/5 and 2016/14.

#pragma once

#include "bitmanip.h"
#include "common.h"
#include <hwy/highway.h>

namespace md5 {

using D = hn::ScalableTag<uint32_t>;
using VecT = hn::Vec<D>;
using Vec4T = hn::Vec4<D>;
constexpr D d;
constexpr size_t max_lanes = hn::MaxLanes(D());

/// The number of 32-bit SIMD lanes, i.e. the number of hashes that can be
/// computed at once.
///
/// Note that this cannot be constexpr since hn::Lanes() is not constexpr due
/// to SVE or RVV where the number of lanes can be set at runtime. We don't
/// support those yet, so in practice this is constant and folded away by the
/// compiler.
inline size_t lanes()
{
    return hn::Lanes(d);
}

// Fixed by the MD5 algorithm.
constexpr size_t words_per_block = 16;
constexpr size_t bytes_per_block = words_per_block * sizeof(uint32_t);

/// Initial state of the MD5 hash function.
inline Vec4T initial_state()
{
    return hn::Create4(d, hn::Set(d, 0x67452301), hn::Set(d, 0xefcdab89),
                       hn::Set(d, 0x98badcfe), hn::Set(d, 0x10325476));
}

/// 64-byte blocks laid out sequentially one after another; as many blocks as
/// we (potentially) have SIMD lanes.
struct SequentialBlocks {
    HWY_ALIGN_MAX char data[max_lanes][bytes_per_block];

    /// Return a set of blocks with each block containing string `s`.
    static SequentialBlocks splat(std::string_view s) noexcept
    {
        ASSERT(s.size() <= bytes_per_block);
        SequentialBlocks result{};
        for (size_t i = 0; i < lanes(); ++i)
            memcpy(result.data[i], s.data(), s.size());
        return result;
    }
};

/// Interleaved 4-byte words from 64-byte blocks, stored as little-endian. This
/// is the format fed into the core hash_block() function to compute multiple
/// hashes in parallel.
struct InterleavedBlocks {
    HWY_ALIGN_MAX uint32_t data[words_per_block][max_lanes];
};

// Interleave 4-byte words from 64-byte blocks laid out one after in memory.
inline InterleavedBlocks interleave(const SequentialBlocks &input)
{
    InterleavedBlocks result;
    const size_t lanes = ::md5::lanes();

    // GCC does a decent job of vectorizing this into a bunch of shuffles
    // (vpermi2d and vpermt2d); doing it by hand is unlikely to yield any
    // significant speedup.
    static_assert(std::endian::native == std::endian::little);
    auto *dst = reinterpret_cast<uint32_t *>(result.data);
    for (size_t i = 0; i < words_per_block; i++) {
        auto *src = reinterpret_cast<const char *>(input.data) + 4 * i;
        for (size_t j = 0; j < lanes; j++, src += bytes_per_block, dst++)
            memcpy(dst, src, sizeof(uint32_t));
    }

    return result;
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
inline void prepare_final_blocks(SequentialBlocks &HWY_RESTRICT messages,
                                 std::span<const uint32_t> length_bytes)
{
    ASSERT(length_bytes.size() >= lanes());

    for (size_t i = 0; i < lanes(); i++) {
        DEBUG_ASSERT(length_bytes[i] < bytes_per_block);

        // The buffer is assumed to be padded and the length of each message is
        // non-decreasing, so all we need to do is to insert the 1 bit (0x80)
        // and add the length in bits.
        messages.data[i][length_bytes[i]] = 0x80;

        // Assumes that message is never going to be more than 65536 bits, and
        // that the rest of the length field is already zeroed.
        messages.data[i][56] = (length_bytes[i] << 3) & 0xff;
        messages.data[i][57] = (length_bytes[i] >> 5) & 0xff;
    }
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
// The terminating 0x80 byte is inserted at the offset given by `x80_offset`,
// if set.
inline void prepare_final_blocks(SequentialBlocks &HWY_RESTRICT messages,
                                 std::optional<size_t> x80_offset,
                                 std::span<const uint32_t> length_bytes)
{
    ASSERT(length_bytes.size() >= lanes());
    ASSERT(!x80_offset || *x80_offset < bytes_per_block);

    for (size_t i = 0; i < lanes(); i++) {
        if (x80_offset)
            messages.data[i][*x80_offset] = 0x80;

        // Assumes that message is never going to be more than 65536 bits, and
        // that the rest of the length field is already zeroed.
        messages.data[i][56] = (length_bytes[i] << 3) & 0xff;
        messages.data[i][57] = (length_bytes[i] >> 5) & 0xff;
    }
}

enum class ResultType { full_result, only_a };

/// Hash multiple blocks simultaneously with SIMD.
///
/// `NonZeroBlockMask` is a 16-bit mask corresponding to each of the 16 4-byte
/// words in the message blocks. A set bit indicates that a word is potentially
/// non-zero and must be loaded from memory, while a cleared bit assumes that
/// the word is zero.
///
/// `ResultType` controls what is returned: either the full result (a, b, c, d)
/// as 4 vectors, or only the `a` value as a single vector.
template <uint16_t NonZeroBlockMask = 0xffff,
          ResultType ResultType = ResultType::full_result>
inline auto
hash_block(const InterleavedBlocks &HWY_RESTRICT M, VecT a0, VecT b0, VecT c0, VecT d0)
{
    VecT A(a0);
    VecT B(b0);
    VecT C(c0);
    VecT D(d0);

#define F(b, c, d) hn::BitwiseIfThenElse(b, c, d)
#define G(b, c, d) hn::BitwiseIfThenElse(d, b, c)
#define H(b, c, d) hn::Xor3(b, c, d)
#define I(b, c, d) (c ^ (b | hn::Not(d)))

#define QUARTER_ROUND(f, a, b, c, d, j, k, shift)                                        \
    do {                                                                                 \
        a += f(b, c, d);                                                                 \
        a += hn::Set(hn::DFromV<decltype(a)>(), K[k]);                                   \
        if constexpr (NonZeroBlockMask & (1 << (j)))                                     \
            a += hn::Load(hn::DFromV<decltype(a)>(), M.data[j]);                         \
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
    QUARTER_ROUND(F, A, B, C, D, 0, 0, 7);
    QUARTER_ROUND(F, D, A, B, C, 1, 1, 12);
    QUARTER_ROUND(F, C, D, A, B, 2, 2, 17);
    QUARTER_ROUND(F, B, C, D, A, 3, 3, 22);
    QUARTER_ROUND(F, A, B, C, D, 4, 4, 7);
    QUARTER_ROUND(F, D, A, B, C, 5, 5, 12);
    QUARTER_ROUND(F, C, D, A, B, 6, 6, 17);
    QUARTER_ROUND(F, B, C, D, A, 7, 7, 22);
    QUARTER_ROUND(F, A, B, C, D, 8, 8, 7);
    QUARTER_ROUND(F, D, A, B, C, 9, 9, 12);
    QUARTER_ROUND(F, C, D, A, B, 10, 10, 17);
    QUARTER_ROUND(F, B, C, D, A, 11, 11, 22);
    QUARTER_ROUND(F, A, B, C, D, 12, 12, 7);
    QUARTER_ROUND(F, D, A, B, C, 13, 13, 12);
    QUARTER_ROUND(F, C, D, A, B, 14, 14, 17);
    QUARTER_ROUND(F, B, C, D, A, 15, 15, 22);

    // Quarter-round 2 (G):
    QUARTER_ROUND(G, A, B, C, D, 1, 16, 5);
    QUARTER_ROUND(G, D, A, B, C, 6, 17, 9);
    QUARTER_ROUND(G, C, D, A, B, 11, 18, 14);
    QUARTER_ROUND(G, B, C, D, A, 0, 19, 20);
    QUARTER_ROUND(G, A, B, C, D, 5, 20, 5);
    QUARTER_ROUND(G, D, A, B, C, 10, 21, 9);
    QUARTER_ROUND(G, C, D, A, B, 15, 22, 14);
    QUARTER_ROUND(G, B, C, D, A, 4, 23, 20);
    QUARTER_ROUND(G, A, B, C, D, 9, 24, 5);
    QUARTER_ROUND(G, D, A, B, C, 14, 25, 9);
    QUARTER_ROUND(G, C, D, A, B, 3, 26, 14);
    QUARTER_ROUND(G, B, C, D, A, 8, 27, 20);
    QUARTER_ROUND(G, A, B, C, D, 13, 28, 5);
    QUARTER_ROUND(G, D, A, B, C, 2, 29, 9);
    QUARTER_ROUND(G, C, D, A, B, 7, 30, 14);
    QUARTER_ROUND(G, B, C, D, A, 12, 31, 20);

    // Quarter-round 3 (H):
    QUARTER_ROUND(H, A, B, C, D, 5, 32, 4);
    QUARTER_ROUND(H, D, A, B, C, 8, 33, 11);
    QUARTER_ROUND(H, C, D, A, B, 11, 34, 16);
    QUARTER_ROUND(H, B, C, D, A, 14, 35, 23);
    QUARTER_ROUND(H, A, B, C, D, 1, 36, 4);
    QUARTER_ROUND(H, D, A, B, C, 4, 37, 11);
    QUARTER_ROUND(H, C, D, A, B, 7, 38, 16);
    QUARTER_ROUND(H, B, C, D, A, 10, 39, 23);
    QUARTER_ROUND(H, A, B, C, D, 13, 40, 4);
    QUARTER_ROUND(H, D, A, B, C, 0, 41, 11);
    QUARTER_ROUND(H, C, D, A, B, 3, 42, 16);
    QUARTER_ROUND(H, B, C, D, A, 6, 43, 23);
    QUARTER_ROUND(H, A, B, C, D, 9, 44, 4);
    QUARTER_ROUND(H, D, A, B, C, 12, 45, 11);
    QUARTER_ROUND(H, C, D, A, B, 15, 46, 16);
    QUARTER_ROUND(H, B, C, D, A, 2, 47, 23);

    // Quarter-round 4 (I):
    QUARTER_ROUND(I, A, B, C, D, 0, 48, 6);
    QUARTER_ROUND(I, D, A, B, C, 7, 49, 10);
    QUARTER_ROUND(I, C, D, A, B, 14, 50, 15);
    QUARTER_ROUND(I, B, C, D, A, 5, 51, 21);
    QUARTER_ROUND(I, A, B, C, D, 12, 52, 6);
    QUARTER_ROUND(I, D, A, B, C, 3, 53, 10);
    QUARTER_ROUND(I, C, D, A, B, 10, 54, 15);
    QUARTER_ROUND(I, B, C, D, A, 1, 55, 21);
    QUARTER_ROUND(I, A, B, C, D, 8, 56, 6);
    QUARTER_ROUND(I, D, A, B, C, 15, 57, 10);
    QUARTER_ROUND(I, C, D, A, B, 6, 58, 15);
    QUARTER_ROUND(I, B, C, D, A, 13, 59, 21);
    QUARTER_ROUND(I, A, B, C, D, 4, 60, 6);
    QUARTER_ROUND(I, D, A, B, C, 11, 61, 10);
    QUARTER_ROUND(I, C, D, A, B, 2, 62, 15);
    QUARTER_ROUND(I, B, C, D, A, 9, 63, 21);

#undef F
#undef G
#undef H
#undef I
#undef QUARTER_ROUND

    if constexpr (ResultType == ResultType::only_a) {
        return A + a0;
    } else if constexpr (ResultType == ResultType::full_result) {
        return hn::Create4(d, A + a0, B + b0, C + c0, D + d0);
    } else {
        static_assert(false);
    }
}

template <uint16_t NonZeroBlockMask = 0xffff,
          ResultType ResultType = ResultType::full_result>
inline auto hash_block(const InterleavedBlocks &HWY_RESTRICT M, Vec4T state)
{
    const VecT a0 = hn::Get4<0>(state);
    const VecT b0 = hn::Get4<1>(state);
    const VecT c0 = hn::Get4<2>(state);
    const VecT d0 = hn::Get4<3>(state);
    return hash_block<NonZeroBlockMask, ResultType>(M, a0, b0, c0, d0);
}

template <uint16_t NonZeroBlockMask = 0xffff,
          ResultType ResultType = ResultType::full_result>
inline auto hash_block(const InterleavedBlocks &HWY_RESTRICT M)
{
    return hash_block<NonZeroBlockMask, ResultType>(M, initial_state());
}

template <uint16_t NonZeroBlockMask = 0xffff,
          ResultType ResultType = ResultType::full_result>
inline auto hash_block(
    const SequentialBlocks &HWY_RESTRICT chunks, VecT a0, VecT b0, VecT c0, VecT d0)
{
    // The input in `chunks` is 64-byte blocks laid out one after another. The
    // MD5 core loop expects memory to contain interleaved 4-byte words from
    // each block, so reshuffle the original input into that format.
    InterleavedBlocks M = interleave(chunks);
    return hash_block<NonZeroBlockMask, ResultType>(M, a0, b0, c0, d0);
}

template <uint16_t NonZeroBlockMask = 0xffff,
          ResultType ResultType = ResultType::full_result>
inline auto hash_block(const SequentialBlocks &HWY_RESTRICT chunks, Vec4T state)
{
    InterleavedBlocks M = interleave(chunks);
    return hash_block<NonZeroBlockMask, ResultType>(M, state);
}

template <uint16_t NonZeroBlockMask = 0xffff,
          ResultType ResultType = ResultType::full_result>
inline auto hash_block(const SequentialBlocks &HWY_RESTRICT chunks)
{
    InterleavedBlocks M = interleave(chunks);
    return hash_block<NonZeroBlockMask, ResultType>(M, initial_state());
}

inline char *to_chars(char *p, uint64_t n)
{
    // 00-99 packed into a single string.
    static constexpr const char packed_digits2[] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

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
    static_assert(N <= 16);
    uint64_t result = (UINT64_C(1) << (4 * (N & ~1))) - 1;
    if (N & 1)
        result |= UINT64_C(0xf) << (4 * N);
    return result;
}
static_assert(make_leading_zero_mask<5>() == 0xf0ffff);
static_assert(make_leading_zero_mask<6>() == 0xffffff);

}

/// Return a mask with a bit set for each 32-bit element in `hashes` that has
/// at least `N` leading zeroes when written as hexadecimal.
template <size_t N>
inline uint32_t leading_zero_mask(const hn::Vec<D> &hashes)
{
    static_assert(N <= 8);
    const hn::Vec<D> mask = hn::Set(d, detail::make_leading_zero_mask<N>());
    return hn::BitsFromMask(d, hn::Eq(mask & hashes, hn::Zero(d)));
}

struct State {
    SequentialBlocks messages{};
    std::string_view prefix;

    State(std::string_view pfx)
        : messages(SequentialBlocks::splat(pfx))
        , prefix(pfx)
    {
    }

    /// Compute MD5 hashes with [block, block+1, ..., block+lanes-1] appended
    /// to each block. The internal buffers are not cleared between calls, so
    /// the block number must never decrease between calls to this method.
    Vec4T run(const int block)
    {
        uint32_t lengths[max_lanes];

        for (size_t i = 0; i < lanes(); i++) {
            char *p = messages.data[i];
            char *q = to_chars(p + prefix.size(), block + i);
            lengths[i] = q - p;
        }

        prepare_final_blocks(messages, lengths);
        return hash_block(messages);
    }
};

/// 0000-9999 packed into a single string, plus a few extra entries wrapping
/// around to 0000 to avoid bounds checks in hash_4digit_chunks().
constexpr auto digits_4x = [] consteval {
    std::array<char, 4 * (10000 + max_lanes)> table;
    for (size_t i = 0; 4 * i < table.size(); ++i) {
        table[4 * i + 0] = '0' + i / 1000;
        table[4 * i + 1] = '0' + (i / 100) % 10;
        table[4 * i + 2] = '0' + (i / 10) % 10;
        table[4 * i + 3] = '0' + i % 10;
    }
    for (size_t i = 4 * 10000; i < table.size(); ++i)
        table[i] = table[i - 4 * 10000];
    return table;
}();

using HashBlockFunc = VecT(const SequentialBlocks &);
extern HashBlockFunc *const partial_hash_funcs[15];

/// Shared logic between 2015/4 and 2016/5.
///
/// Hashes 10,000 messages with a given prefix with the length `prefix_len`
/// concatenated with a incrementing numeric suffix starting at `chunk_start`,
/// which must be divisible by 10,000. `messages` is assumed to already be
/// filled with the prefix in each block, and is modified in place.
///
/// The `sink` function is repeatedly called with a vector containing the first
/// 32 bits of each hash, plus the numeric suffix of the first message in the
/// vector. It can return `false` to stop early, in which case this function
/// will also return `false`.
inline bool hash_4digit_chunks(md5::SequentialBlocks &messages,
                               const uint64_t chunk_start,
                               const size_t prefix_len,
                               std::invocable<VecT, uint64_t> auto &&sink)
{
    DEBUG_ASSERT(chunk_start % 10000 == 0);

    const size_t lanes = md5::lanes();
    uint64_t tail = 0;
    std::array<uint32_t, max_lanes> lengths;

    if (chunk_start == 0) [[unlikely]] {
        // The first 1,000 messages have a 1-3 digit suffix, so we cannot write
        // it out 4 digits at a time. Handle these the slow but simple way by
        // writing out the full suffix and length for each message.
        for (; tail < 1000; tail += lanes) {
            for (size_t i = 0; i < lanes; i++) {
                lengths[i] = prefix_len + digit_count_base10(tail + i);
                to_chars(&messages.data[i][prefix_len], tail + i);
            }
            prepare_final_blocks(messages, lengths);

            const auto hashes = hash_block<0x7fff, ResultType::only_a>(messages);
            if (!sink(hashes, tail))
                return false;
        }
    }

    const size_t suffix_len = std::max(4, digit_count_base10(chunk_start));
    DEBUG_ASSERT(prefix_len + suffix_len < bytes_per_block - 8 - 1);
    const size_t tail_offset = prefix_len + suffix_len - 4;

    // Prepare the messages beforehand, writing out everything needed to hash
    // them except for the suffix since the 10,000 messages in the same chunk
    // have the same length by construction.
    lengths.fill(prefix_len + suffix_len);
    prepare_final_blocks(messages, lengths);
    for (size_t i = 0; i < lanes; i++)
        to_chars(&messages.data[i][prefix_len], chunk_start + tail + i);

    const size_t non_empty_blocks = (prefix_len + suffix_len + 4) / 4;
    DEBUG_ASSERT(non_empty_blocks < std::size(partial_hash_funcs));

    const auto hash_fn = partial_hash_funcs[non_empty_blocks];

    for (; tail < 10000; tail += lanes) {
        // Since we can write four digits at a time using a single 4-byte
        // store, this inner loop updates only the last four digits of each
        // message. We tolerate `tail` being slightly larger than 9999 here
        // since the digits_4x table contains a few extra entries wrapping
        // around to 0000.
        for (size_t i = 0; i < lanes; i++)
            memcpy(&messages.data[i][tail_offset], &digits_4x[4 * (tail + i)], 4);

        // NOTE: This still has to interleave the message blocks for each batch
        // of messages to hash. This is likely the one remaining thing that
        // would yield a non-trivial speedup if eliminated; it accounts for
        // ~20% of the total runtime for 2015/4 on my machine.
        //
        // But in that case, all logic in this function would have to deal with
        // the suffix potentially being split into 4-byte words at variable
        // offsets depending on the prefix length, instead of being contiguous
        // in each message. Thinking about that gives me a headache, so let's
        // just not.
        const auto hashes = hash_fn(messages);
        if (!sink(hashes, chunk_start + tail))
            return false;
    }

    return true;
}

} // namespace md5
