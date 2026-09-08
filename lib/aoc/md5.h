// Shared code between 2015/4, 2016/5, 2016/14 and 2016/17.

#pragma once

#include <algorithm>
#include <aoc/macros.h>
#include <aoc/math.h>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <hwy/highway.h>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace md5 {

namespace hn = hwy::HWY_NAMESPACE;

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

/// 64-byte blocks laid out sequentially one after another.
template <size_t N>
struct SequentialBlocksN {
    static_assert(N >= 1);
    HWY_ALIGN_MAX char data[N * max_lanes][bytes_per_block];

    /// Return a set of blocks with each block containing string `s`.
    static SequentialBlocksN splat(std::string_view s) noexcept
    {
        ASSERT(s.size() <= bytes_per_block);
        SequentialBlocksN result{};
        for (size_t i = 0; i < N * max_lanes; ++i)
            memcpy(result.data[i], s.data(), s.size());
        return result;
    }
};

/// Interleaved 4-byte words from 64-byte blocks, stored as little-endian. This
/// is the format fed into the core hash_block() function to compute multiple
/// hashes in parallel.
template <size_t N>
struct InterleavedBlocksN {
    static_assert(N >= 1);
    HWY_ALIGN_MAX uint32_t data[N][words_per_block][max_lanes];
};

// Default versions of SequentialBlocksN and InterleavedBlocksN to use. This
// choice depends on architecture, e.g. the number of available vector
// registers (we want as many as possible without causing excessive spills due
// to register pressure in `hash_block()`), and microarchitecture, e.g. how
// many parallel dependency chains can actually be sustained.
//
// The specific choices below are half-empirical and half-hand-wavey based on
// what seems to be fastest on the machines I've measured on.
//
// Each additional unroll factor adds 4 more registers for hash state, plus a
// constant 4 registers for the initial state which has to be kept alive until
// the final addition.
#if defined(__AVX512F__)
// AVX-512: 32 registers; unroll by 6 (state consumes 24+4 registers).
using InterleavedBlocks = InterleavedBlocksN<6>;
using SequentialBlocks = SequentialBlocksN<6>;
#elif defined(__aarch64__)
// NEON: 32 registers; unroll by 4 (state consumes 16+4 registers).
using InterleavedBlocks = InterleavedBlocksN<4>;
using SequentialBlocks = SequentialBlocksN<4>;
#elif defined(__AVX__) || defined(__SSE2__)
// SSE/AVX: 16 registers; unroll by 2 (state consumes 8+4 registers).
using InterleavedBlocks = InterleavedBlocksN<2>;
using SequentialBlocks = SequentialBlocksN<2>;
#else
// No clue, don't unroll.
using InterleavedBlocks = InterleavedBlocksN<1>;
using SequentialBlocks = SequentialBlocksN<1>;
#endif

// Interleave 4-byte words from 64-byte blocks laid out one after another.
template <size_t N>
inline InterleavedBlocksN<N> interleave(const SequentialBlocksN<N> &input)
{
    static_assert(std::endian::native == std::endian::little);

    // GCC does a decent job of vectorizing this into a bunch of shuffles
    // (vpermi2d and vpermt2d); doing it by hand is unlikely to yield any
    // significant speedup.
    InterleavedBlocksN<N> result;
    for (size_t i = 0; i < N; ++i) {
        auto *HWY_RESTRICT dst = reinterpret_cast<uint32_t *>(&result.data[i]);
        const char *HWY_RESTRICT src =
            reinterpret_cast<const char *>(input.data) + i * max_lanes * bytes_per_block;
        for (size_t w = 0; w < words_per_block; w++) {
            auto *HWY_RESTRICT p = &src[4 * w];
            for (size_t j = 0; j < max_lanes; j++, p += bytes_per_block, dst++)
                memcpy(dst, p, sizeof(uint32_t));
        }
    }
    return result;
}

// Prepare the final messages blocks by inserting the block lengths into the
// `messages`, assuming that the messages are already padded with zero bits.
template <size_t N>
inline void prepare_final_blocks(SequentialBlocksN<N> &messages,
                                 std::span<const uint32_t> length_bytes)
{
    ASSERT(length_bytes.size() >= N * lanes());

    for (size_t i = 0; i < N * lanes(); i++) {
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
template <size_t N>
inline void prepare_final_blocks(SequentialBlocksN<N> &messages,
                                 std::optional<size_t> x80_offset,
                                 std::span<const uint32_t> length_bytes)
{
    ASSERT(length_bytes.size() >= N * lanes());
    ASSERT(!x80_offset || *x80_offset < bytes_per_block);

    for (size_t i = 0; i < N * lanes(); i++) {
        if (x80_offset)
            messages.data[i][*x80_offset] = 0x80;

        // Assumes that message is never going to be more than 65536 bits, and
        // that the rest of the length field is already zeroed.
        messages.data[i][56] = (length_bytes[i] << 3) & 0xff;
        messages.data[i][57] = (length_bytes[i] >> 5) & 0xff;
    }
}

// The actual MD5 core compression functions are defined below. This is a mess
// of macros because of a few reasons:
//
// 1. We want to provide multiple versions with different unroll factors.
//
// 2. To be compatible with SVE/RVV, vectors can't be stored in structs or
//    arrays. This means that we can't have a single template that stores hash
//    state via a struct or array with a templated size.
//
// Hnece, all hash state vectors must be local variables or be explicitly
// written to and read from memory. This _could_ likely be worked around by
// keeping all state as template parameter packs, operating on them via pack
// expansion, and HWY_FLATTEN-ing the functions, but I don't trust myself to
// write it correctly, much less read a week later. For all of its ugliness,
// this approach at least expands to something that is very straight-forward to
// understand (even if extremely verbose).
//
// 3. Returning the hash states runs into the same limitation: we can't return
//    a user-defined struct or an array.
//
// Instead, rather than writing the output to memory, hash_block() invokes a
// sink, à la continuation-passing style, with the full hash state bundled as
// 4-tuples of vectors.
//
// Writing the output to memory would also work, but passing the live vectors
// to a sink has the benefit of making it more likely that they are kept in
// registers after inlining; it also lets us select only parts of the result.
// This is of interest for some solvers that only care about A (the first 32
// bits of the hash), e.g. 2015/4 and 2016/5, in which case the compiler can
// DCE the steps of the last quarter-round that only affect B, C, and D.
//
// A huge downside with this is that it introduces a potential performance trap
// for large unroll counts. In that case there are going to be many vector
// registers live across the first sink call; if it invokes a function which
// the compiler hasn't inlined (or can't inline), *all* of those registers are
// going to be immediately spilled and restored. A similar situation arises if
// the sink itself is large and adds register pressure once inlined, which
// again leads to a lot of spills onto the stack.

// MD5 quarter-round functions.
#define F(b, c, d) hn::BitwiseIfThenElse(b, c, d)
#define G(b, c, d) hn::BitwiseIfThenElse(d, b, c)
#define H(b, c, d) hn::Xor3(b, c, d)
#define I(b, c, d) (c ^ (b | hn::Not(d)))

#define QUARTER_ROUND(m, func, i, aa, bb, cc, dd, k, shift)                              \
    do {                                                                                 \
        GLUE(aa, i) += func(GLUE(bb, i), GLUE(cc, i), GLUE(dd, i));                      \
        GLUE(aa, i) += hn::Set(d, k);                                                    \
        GLUE(aa, i) += hn::Load(d, m);                                                   \
        GLUE(aa, i) = hn::RotateLeft<shift>(GLUE(aa, i));                                \
        GLUE(aa, i) += GLUE(bb, i);                                                      \
    } while (0)

#define ALL_MD5_ROUNDS(Q)                                                                \
    do {                                                                                 \
        /* Quarter-round 1 (F): */                                                       \
        Q(F, A, B, C, D, 0, 0xd76aa478, 7);                                              \
        Q(F, D, A, B, C, 1, 0xe8c7b756, 12);                                             \
        Q(F, C, D, A, B, 2, 0x242070db, 17);                                             \
        Q(F, B, C, D, A, 3, 0xc1bdceee, 22);                                             \
        Q(F, A, B, C, D, 4, 0xf57c0faf, 7);                                              \
        Q(F, D, A, B, C, 5, 0x4787c62a, 12);                                             \
        Q(F, C, D, A, B, 6, 0xa8304613, 17);                                             \
        Q(F, B, C, D, A, 7, 0xfd469501, 22);                                             \
        Q(F, A, B, C, D, 8, 0x698098d8, 7);                                              \
        Q(F, D, A, B, C, 9, 0x8b44f7af, 12);                                             \
        Q(F, C, D, A, B, 10, 0xffff5bb1, 17);                                            \
        Q(F, B, C, D, A, 11, 0x895cd7be, 22);                                            \
        Q(F, A, B, C, D, 12, 0x6b901122, 7);                                             \
        Q(F, D, A, B, C, 13, 0xfd987193, 12);                                            \
        Q(F, C, D, A, B, 14, 0xa679438e, 17);                                            \
        Q(F, B, C, D, A, 15, 0x49b40821, 22);                                            \
        /* Quarter-round 2 (G): */                                                       \
        Q(G, A, B, C, D, 1, 0xf61e2562, 5);                                              \
        Q(G, D, A, B, C, 6, 0xc040b340, 9);                                              \
        Q(G, C, D, A, B, 11, 0x265e5a51, 14);                                            \
        Q(G, B, C, D, A, 0, 0xe9b6c7aa, 20);                                             \
        Q(G, A, B, C, D, 5, 0xd62f105d, 5);                                              \
        Q(G, D, A, B, C, 10, 0x02441453, 9);                                             \
        Q(G, C, D, A, B, 15, 0xd8a1e681, 14);                                            \
        Q(G, B, C, D, A, 4, 0xe7d3fbc8, 20);                                             \
        Q(G, A, B, C, D, 9, 0x21e1cde6, 5);                                              \
        Q(G, D, A, B, C, 14, 0xc33707d6, 9);                                             \
        Q(G, C, D, A, B, 3, 0xf4d50d87, 14);                                             \
        Q(G, B, C, D, A, 8, 0x455a14ed, 20);                                             \
        Q(G, A, B, C, D, 13, 0xa9e3e905, 5);                                             \
        Q(G, D, A, B, C, 2, 0xfcefa3f8, 9);                                              \
        Q(G, C, D, A, B, 7, 0x676f02d9, 14);                                             \
        Q(G, B, C, D, A, 12, 0x8d2a4c8a, 20);                                            \
        /* Quarter-round 3 (H): */                                                       \
        Q(H, A, B, C, D, 5, 0xfffa3942, 4);                                              \
        Q(H, D, A, B, C, 8, 0x8771f681, 11);                                             \
        Q(H, C, D, A, B, 11, 0x6d9d6122, 16);                                            \
        Q(H, B, C, D, A, 14, 0xfde5380c, 23);                                            \
        Q(H, A, B, C, D, 1, 0xa4beea44, 4);                                              \
        Q(H, D, A, B, C, 4, 0x4bdecfa9, 11);                                             \
        Q(H, C, D, A, B, 7, 0xf6bb4b60, 16);                                             \
        Q(H, B, C, D, A, 10, 0xbebfbc70, 23);                                            \
        Q(H, A, B, C, D, 13, 0x289b7ec6, 4);                                             \
        Q(H, D, A, B, C, 0, 0xeaa127fa, 11);                                             \
        Q(H, C, D, A, B, 3, 0xd4ef3085, 16);                                             \
        Q(H, B, C, D, A, 6, 0x04881d05, 23);                                             \
        Q(H, A, B, C, D, 9, 0xd9d4d039, 4);                                              \
        Q(H, D, A, B, C, 12, 0xe6db99e5, 11);                                            \
        Q(H, C, D, A, B, 15, 0x1fa27cf8, 16);                                            \
        Q(H, B, C, D, A, 2, 0xc4ac5665, 23);                                             \
        /* Quarter-round 4 (I): */                                                       \
        Q(I, A, B, C, D, 0, 0xf4292244, 6);                                              \
        Q(I, D, A, B, C, 7, 0x432aff97, 10);                                             \
        Q(I, C, D, A, B, 14, 0xab9423a7, 15);                                            \
        Q(I, B, C, D, A, 5, 0xfc93a039, 21);                                             \
        Q(I, A, B, C, D, 12, 0x655b59c3, 6);                                             \
        Q(I, D, A, B, C, 3, 0x8f0ccc92, 10);                                             \
        Q(I, C, D, A, B, 10, 0xffeff47d, 15);                                            \
        Q(I, B, C, D, A, 1, 0x85845dd1, 21);                                             \
        Q(I, A, B, C, D, 8, 0x6fa87e4f, 6);                                              \
        Q(I, D, A, B, C, 15, 0xfe2ce6e0, 10);                                            \
        Q(I, C, D, A, B, 6, 0xa3014314, 15);                                             \
        Q(I, B, C, D, A, 13, 0x4e0811a1, 21);                                            \
        Q(I, A, B, C, D, 4, 0xf7537e82, 6);                                              \
        Q(I, D, A, B, C, 11, 0xbd3af235, 10);                                            \
        Q(I, C, D, A, B, 2, 0x2ad7d2bb, 15);                                             \
        Q(I, B, C, D, A, 9, 0xeb86d391, 21);                                             \
    } while (0)

inline auto hash_block(const InterleavedBlocksN<1> &M, Vec4T state, auto &&sink)
{
    const VecT a_init = hn::Get4<0>(state);
    const VecT b_init = hn::Get4<1>(state);
    const VecT c_init = hn::Get4<2>(state);
    const VecT d_init = hn::Get4<3>(state);
    VecT A0 = a_init, B0 = b_init, C0 = c_init, D0 = d_init;

#define QUARTER_ROUNDS_UNROLLED(f, aa, bb, cc, dd, j, k, shift)                          \
    QUARTER_ROUND(M.data[0][j], f, 0, aa, bb, cc, dd, k, shift)

    ALL_MD5_ROUNDS(QUARTER_ROUNDS_UNROLLED);

#undef QUARTER_ROUNDS_UNROLLED

    return sink(hn::Create4(d, A0 + a_init, B0 + b_init, C0 + c_init, D0 + d_init));
}

inline auto hash_block(const InterleavedBlocksN<2> &M, Vec4T state, auto &&sink)
{
    const VecT a_init = hn::Get4<0>(state);
    const VecT b_init = hn::Get4<1>(state);
    const VecT c_init = hn::Get4<2>(state);
    const VecT d_init = hn::Get4<3>(state);
    VecT A0 = a_init, B0 = b_init, C0 = c_init, D0 = d_init;
    VecT A1 = a_init, B1 = b_init, C1 = c_init, D1 = d_init;

#define QUARTER_ROUNDS_UNROLLED(f, aa, bb, cc, dd, j, k, shift)                          \
    QUARTER_ROUND(M.data[0][j], f, 0, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[1][j], f, 1, aa, bb, cc, dd, k, shift)

    ALL_MD5_ROUNDS(QUARTER_ROUNDS_UNROLLED);

#undef QUARTER_ROUNDS_UNROLLED

    return sink(hn::Create4(d, A0 + a_init, B0 + b_init, C0 + c_init, D0 + d_init),
                hn::Create4(d, A1 + a_init, B1 + b_init, C1 + c_init, D1 + d_init));
}

inline auto hash_block(const InterleavedBlocksN<4> &M, Vec4T state, auto &&sink)
{
    const VecT a_init = hn::Get4<0>(state);
    const VecT b_init = hn::Get4<1>(state);
    const VecT c_init = hn::Get4<2>(state);
    const VecT d_init = hn::Get4<3>(state);
    VecT A0 = a_init, B0 = b_init, C0 = c_init, D0 = d_init;
    VecT A1 = a_init, B1 = b_init, C1 = c_init, D1 = d_init;
    VecT A2 = a_init, B2 = b_init, C2 = c_init, D2 = d_init;
    VecT A3 = a_init, B3 = b_init, C3 = c_init, D3 = d_init;

#define QUARTER_ROUNDS_UNROLLED(f, aa, bb, cc, dd, j, k, shift)                          \
    QUARTER_ROUND(M.data[0][j], f, 0, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[1][j], f, 1, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[2][j], f, 2, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[3][j], f, 3, aa, bb, cc, dd, k, shift)

    ALL_MD5_ROUNDS(QUARTER_ROUNDS_UNROLLED);

#undef QUARTER_ROUNDS_UNROLLED

    return sink(hn::Create4(d, A0 + a_init, B0 + b_init, C0 + c_init, D0 + d_init),
                hn::Create4(d, A1 + a_init, B1 + b_init, C1 + c_init, D1 + d_init),
                hn::Create4(d, A2 + a_init, B2 + b_init, C2 + c_init, D2 + d_init),
                hn::Create4(d, A3 + a_init, B3 + b_init, C3 + c_init, D3 + d_init));
}

inline auto hash_block(const InterleavedBlocksN<6> &M, Vec4T state, auto &&sink)
{
    const VecT a_init = hn::Get4<0>(state);
    const VecT b_init = hn::Get4<1>(state);
    const VecT c_init = hn::Get4<2>(state);
    const VecT d_init = hn::Get4<3>(state);
    VecT A0 = a_init, B0 = b_init, C0 = c_init, D0 = d_init;
    VecT A1 = a_init, B1 = b_init, C1 = c_init, D1 = d_init;
    VecT A2 = a_init, B2 = b_init, C2 = c_init, D2 = d_init;
    VecT A3 = a_init, B3 = b_init, C3 = c_init, D3 = d_init;
    VecT A4 = a_init, B4 = b_init, C4 = c_init, D4 = d_init;
    VecT A5 = a_init, B5 = b_init, C5 = c_init, D5 = d_init;

#define QUARTER_ROUNDS_UNROLLED(f, aa, bb, cc, dd, j, k, shift)                          \
    QUARTER_ROUND(M.data[0][j], f, 0, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[1][j], f, 1, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[2][j], f, 2, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[3][j], f, 3, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[4][j], f, 4, aa, bb, cc, dd, k, shift);                         \
    QUARTER_ROUND(M.data[5][j], f, 5, aa, bb, cc, dd, k, shift)

    ALL_MD5_ROUNDS(QUARTER_ROUNDS_UNROLLED);

#undef QUARTER_ROUNDS_UNROLLED

    return sink(hn::Create4(d, A0 + a_init, B0 + b_init, C0 + c_init, D0 + d_init),
                hn::Create4(d, A1 + a_init, B1 + b_init, C1 + c_init, D1 + d_init),
                hn::Create4(d, A2 + a_init, B2 + b_init, C2 + c_init, D2 + d_init),
                hn::Create4(d, A3 + a_init, B3 + b_init, C3 + c_init, D3 + d_init),
                hn::Create4(d, A4 + a_init, B4 + b_init, C4 + c_init, D4 + d_init),
                hn::Create4(d, A5 + a_init, B5 + b_init, C5 + c_init, D5 + d_init));
}

#undef F
#undef G
#undef H
#undef I
#undef QUARTER_ROUND
#undef ALL_MD5_ROUNDS

inline auto hash_block(const InterleavedBlocksN<1> &M, Vec4T state = initial_state())
{
    auto sink = [](Vec4T hashes) { return hashes; };
    return hash_block(M, state, sink);
}

inline auto hash_block(const SequentialBlocksN<1> &chunks, Vec4T state = initial_state())
{
    return hash_block(interleave(chunks), state);
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
    SequentialBlocksN<1> messages{};
    std::string_view prefix;

    State(std::string_view pfx)
        : messages(SequentialBlocksN<1>::splat(pfx))
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

/// 0000-9999 packed into a single string, plus extra entries wrapping around
/// to 0000 to avoid bounds checks in hash_4digit_chunks().
constexpr auto digits_4x = [] consteval {
    std::array<char, 4 * (10000 + 6 * max_lanes)> table;
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

/// Shared logic between 2015/4 and 2016/5.
///
/// Hashes 10,000 messages with a given prefix with the length `prefix_len`
/// concatenated with a incrementing numeric suffix starting at `chunk_start`,
/// which must be divisible by 10,000. `messages` is assumed to already be
/// filled with the prefix in each block, and is modified in place.
///
/// `sink_fn` is repeatedly called with a vector containing the first 32 bits
/// of each hash, plus the numeric suffix of the first message in the vector.
/// It can return `false` to stop early, in which case this function will also
/// return `false`.
template <size_t N>
inline bool hash_4digit_chunks(SequentialBlocksN<N> &messages,
                               const uint64_t chunk_start,
                               const size_t prefix_len,
                               std::invocable<VecT, uint64_t> auto &&sink_fn)
{
    DEBUG_ASSERT(chunk_start % 10000 == 0);

    const size_t lanes = md5::lanes();
    uint64_t tail = 0;
    std::array<uint32_t, N * max_lanes> lengths;

    auto sink = [&](const uint64_t n) {
        auto M = interleave(messages);

        // We want the steps of each MD5 block to be computed at the same time
        // in an interleaved fashion to exploit ILP. Unfortunately, after
        // inlining, GCC realizes that the lambda uses the output of only one
        // of the independent chains; code motion (-free-sink) then sinks the
        // other chains to immediately before they are used, turning the code
        // into:
        //
        //     (md5 -> sink) x N -> (md5 -> sink) x N -> ...
        //
        // instead of the desired:
        //
        //     (md5 x N) -> (sink x N) -> (md5 x N) -> (sink x N) -> ...
        //
        // which tanks performance since no interleaving means no ILP. Use
        // PreventElision from Highway (in practice, an asm barrier) to make
        // all hash states be computed before sink_fn is called. This is a
        // best-effort hack...
        return hash_block(M, initial_state(), [&](auto... states) {
            size_t i = 0;
            ((hwy::PreventElision(hn::GetLane(hn::Get4<0>(states)))), ...);
            return (sink_fn(hn::Get4<0>(states), n + (i++) * lanes) && ...);
        });
    };

    if (chunk_start == 0) [[unlikely]] {
        // The first 1,000 messages have a 1-3 digit suffix, so we cannot write
        // it out 4 digits at a time. Handle these the slow but simple way by
        // writing out the full suffix and length for each message.
        for (; tail < 1000; tail += N * lanes) {
            for (size_t i = 0; i < N * lanes; i++) {
                lengths[i] = prefix_len + digit_count_base10(tail + i);
                to_chars(&messages.data[i][prefix_len], tail + i);
            }
            prepare_final_blocks(messages, lengths);
            if (!sink(tail))
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
    for (size_t i = 0; i < N * lanes; i++)
        to_chars(&messages.data[i][prefix_len], chunk_start + tail + i);

    for (; tail < 10000; tail += N * lanes) {
        // Since we can write four digits at a time using a single 4-byte
        // store, this inner loop updates only the last four digits of each
        // message. We tolerate `tail` being slightly larger than 9999 here
        // since the digits_4x table contains a few extra entries wrapping
        // around to 0000.
        for (size_t i = 0; i < N * lanes; i++)
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
        if (!sink(chunk_start + tail))
            return false;
    }
    return true;
}

} // namespace md5
