#include "common.h"

namespace aoc_2016_16 {

struct Input {
    uint64_t a;   // original problem input in binary
    size_t nbits; // bit length of `a`
};

/// Reverse and complement bits [0:n-1] in `v`.
constexpr uint64_t bit_reverse_step(uint64_t v, const uint64_t n)
{
    v = (v & 0x5555555555555555) << 1 | (v & 0xaaaaaaaaaaaaaaaa) >> 1; // swap bits
    v = (v & 0x3333333333333333) << 2 | (v & 0xcccccccccccccccc) >> 2; // swap 2 bits
    v = (v & 0x0f0f0f0f0f0f0f0f) << 4 | (v & 0xf0f0f0f0f0f0f0f0) >> 4; // swap nibbles
    return ~std::byteswap(v) >> (64 - n);
}

// The first few iterations of the given sequence described in the problem are:
//
//     a
//     a 0 b
//     a 0 b 0 a 1 b
//     a 0 b 0 a 1 b 0 a 0 b 1 a 1 b
//     a 0 b 0 a 1 b 0 a 0 b 1 a 1 b 0 a 0 b 0 a 1 b 1 a 0 b 1 a 1 b
//
// i.e. a sequence of pairs of a and b interleaved with constant 0 or 1 bits.
// Call a portion `a X b Y' a block (where X and Y are constant).
//
// The interleaved sequence of 0s and 1s here is the dragon curve (or
// paper-folding seqeuence); see <https://oeis.org/A014707>. The formula given
// there is a(4n) = 0, a(4n+2) = 1, a(2n+1) = a(n); or in terms of the binary
// representation of `n`, after removing all trailing ones, a(n) is the
// next-to-least significant bit:
constexpr int dragon_curve(size_t n)
{
    return (n & (UINT64_C(1) << (std::countr_one(n) + 1))) != 0;
}

// The disk length in part two is 35651584, or 17 * 2^21. Since the checksum
// process stops once we hit an odd number, each output bit is the XNOR of 2^21
// or 2097152 expanded input bits. In other words, the inverted parity -- an
// even popcount results in an output of 1; an odd popcount, 0.
//
// Each pair of `a` and `b` contributes to the parity only if the original
// input (a) has odd length, since `b` is the bitwise NOT of `a` (reversing it
// does not matter for the bit count), so `a` and `b` will contribute the total
// length of `a`. Thus, the only contributions that are variable are from the
// intervening 0 and 1 bits, i.e. the dragon curve sequence.
//
// The parity of a block `a X b Y` is equal to the parity of `X xor Y`, which
// varies, combined with the parity of `a xor b`, which is constant and assumed
// to be 1 (which stems from the assumption that the input length is odd). This
// function computes the parity of a sequence of whole blocks, starting at a
// given block in the sequence:
constexpr int full_blocks_bitcount(size_t block_begin, size_t block_end)
{
    int result = 0;

    // We need to compute the dragon curve bit at offset 2*block and 2*block+1
    // for each block in the range [block_begin, block_end), but this can be
    // greatly simplified in two ways:
    //
    // (1) By definition, `2*block` has no trailing ones, so the value of the
    // dragon curve at that is just the next-to-least significant bit of
    // `2*block`, i.e. the least significant bit of `block`. This computation
    // can be hoisted outside of the loop by computing the number of such bits
    // in the given interval in constant time.
    result += (block_end - (block_begin & ~1)) / 2;

    // (2) dragon_curve(2*block+1) is just dragon_curve(block) by definition
    // (by the formula from the OEIS entry).
    for (size_t block = block_begin; block < block_end; ++block)
        result += dragon_curve(block);

    return result + block_end - block_begin;
}

constexpr uint64_t assemble_block(const Input &input, size_t block_num)
{
    // This is dragon_curve(2*block_num) and dragon_curve(2*block_num+1),
    // simplified by the same reasoning as in full_blocks_bitcount() above. This
    // is only called for the head and tail of each chunk of blocks, so not
    // performance sensitive.
    const uint64_t d2p0 = block_num & 1;
    const uint64_t d2p1 = dragon_curve(block_num);

    uint64_t m = 0;
    m |= input.a << 0;
    m |= d2p0 << input.nbits;
    m |= bit_reverse_step(input.a, input.nbits) << (input.nbits + 1);
    m |= d2p1 << (2 * input.nbits + 1);
    return m;
}

constexpr int head_bitcount(const Input &input, size_t block_num, size_t prefix_nbits)
{
    const uint64_t block = assemble_block(input, block_num);
    return std::popcount(block & (UINT64_MAX >> (64 - prefix_nbits)));
}

constexpr int tail_bitcount(const Input &input, size_t block_num, size_t suffix_nbits)
{
    const uint64_t block = assemble_block(input, block_num);
    const unsigned int start = 2 * input.nbits + 2 - suffix_nbits;
    const uint64_t mask = UINT64_MAX >> (64 - suffix_nbits);
    return std::popcount((block >> start) & mask);
}

// This computes the parity of the expanded input stream between start_bit and
// end_bit, taking into account any partial blocks that appear as the head and
// tail of the range.
constexpr bool blocks_parity(const Input &input, size_t start_bit, size_t end_bit)
{
    const size_t block_size = 2 * input.nbits + 2;
    const size_t full_block_begin = (start_bit + block_size - 1) / block_size;
    const size_t full_block_end = end_bit / block_size;

    // Hack for part 1: the number of input bits per output bit is larger than
    // the block size, so we need to support taking the popcount of a bit range
    // in the middle of a block here (rather than a full prefix or suffix).
    if (full_block_begin > full_block_end) {
        const uint64_t block = assemble_block(input, full_block_end);
        const uint64_t start = start_bit % block_size;
        const uint64_t len = end_bit % block_size - start_bit % block_size;
        return std::popcount((block >> start) & (UINT64_MAX >> (64 - len))) % 2 != 0;
    }

    size_t head_bits = 0;
    if (const auto r = start_bit % block_size)
        head_bits = tail_bitcount(input, full_block_begin - 1, block_size - r);

    size_t tail_bits = 0;
    if (const auto r = end_bit % block_size)
        tail_bits = head_bitcount(input, full_block_end, r);

    const size_t full_block_bits =
        full_block_begin != full_block_end
            ? full_blocks_bitcount(full_block_begin, full_block_end)
            : 0;

    return (head_bits + tail_bits + full_block_bits) % 2 != 0;
}

constexpr Input parse_input(std::string_view s)
{
    uint64_t a = 0;
    for (size_t i = 0; i < s.size(); ++i)
        a |= static_cast<uint64_t>(s[i] - '0') << i;

    return Input{a, s.size()};
}

void run(std::string_view buf)
{
    ASSERT(buf.size() % 2 == 1);
    const Input inp = parse_input(buf);

    auto solve = [&](size_t n) {
        const auto output_size = n >> std::countr_zero(n);
        const auto input_bits_per_output_bit = n / output_size;

        for (size_t k = 0; k < output_size; ++k) {
            const size_t start_bit = k * input_bits_per_output_bit;
            const size_t end_bit = std::min(n, (k + 1) * input_bits_per_output_bit);
            const bool parity = blocks_parity(inp, start_bit, end_bit);
            fputc(parity ? '0' : '1', stdout);
        }
        fputc('\n', stdout);
    };

    solve(272);
    solve(35651584);
}

}
