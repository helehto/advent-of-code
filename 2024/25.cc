#include "common.h"
#include "inplace_vector.h"
#include <hwy/highway.h>

namespace aoc_2024_25 {

// Each key and lock is represented using a 16-bit integer, with the lowest 15
// bit consisting of five 3-bit counters holding the number of occupied slots
// in that column, excluding the top row and bottom row. With a picture, where
// aaa to eee are the five counters, and the highest bit C only being used as a
// carry-out for the counter in bits 12 to 14 (aaa):
//
//    .---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---.
//    | C | a | a | a | b | b | b | c | c | c | d | d | d | e | e | e |
//    '---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---'
//     15  14  13  12  11   10  9   8   7   6   5   4   3   2   1   0
//
// With this representation, we can use a single uint16_t addition to add the
// number of occupied slots of all columns for a given (lock, key) pair. If the
// 3-bit counter for a group overflows, the carry will propagate into the next
// group. Checking whether there is a overlap in _any_ column is then just a
// matter of checking if a carry has propagated into _any_ group.
//
// To check whether the total number of occupied slots in a column is greater
// than 5, we bias the addition by making each 3-bit counter start at 2 (see
// `counter_bias` below). This means we can add anything lower than or equal to
// 5 to such a group without overflow, but anything greater will, again, result
// in a carry-out into the next group.
//
// Overflows are detected by comparing the actual result of the sum with carry-
// less addition (XOR): if sum = lock + key (a normal 16-bit addition), the
// expression (lock ^ key ^ sum) yields a mask with a bit set in all places
// that received a carry in. ANDing this with `carry_bit_mask` below, with the
// low bit of each counter set to 1, results in a non-zero value if any carry
// was propagated between groups, meaning that there was an overlap. A zero
// value means that here is no overlap.
constexpr uint16_t counter_bias = 0b0'010'010'010'010'010;
constexpr uint16_t carry_bit_mask = 0b1'001'001'001'001'001;

void run(std::string_view buf)
{
    inplace_vector<int16_t, 256> locks;

    // The key array is a fixed multiple of 128 so that we can unroll the core
    // loop below without needing a 1-way vectorized or scalar fallback for the
    // remaining elements. The value 0xffff will always result in an overlap,
    // so this does not affect the final value.
    std::array<int16_t, 256> keys;
    keys.fill(0xffff);
    size_t n_keys = 0;

    size_t i = 0;
    for (; i + 3 < buf.size(); i += 6 * 7 + 1) {
        constexpr hn::FixedTag<uint8_t, 32> d;
        auto vchars = hn::LoadU(d, reinterpret_cast<const uint8_t *>(&buf[i + 6]));
        auto vfilled = hn::Eq(vchars, hn::Set(d, '#'));
        unsigned int filled = hn::BitsFromMask(d, vfilled);

        // Add an entire column/group at a time to `u`. The highest bit in each
        // group of 6 here is the newline, which we ignore:
        uint16_t u = 0;
        constexpr uint32_t column_mask = 0b000001'000001'000001'000001'000001;
        u += std::popcount(filled & (column_mask << 0)) << 0;
        u += std::popcount(filled & (column_mask << 1)) << 3;
        u += std::popcount(filled & (column_mask << 2)) << 6;
        u += std::popcount(filled & (column_mask << 3)) << 9;
        u += std::popcount(filled & (column_mask << 4)) << 12;

        // Split locks and keys into separate vectors so that we never check a
        // lock against a lock, or a key against a key.
        if (buf[i] == '#')
            locks.unchecked_push_back(u);
        else
            keys[n_keys++] = u;
    }

    using D = hn::ScalableTag<int16_t>;
    constexpr D d;
    hn::Vec<D> count = hn::Zero(d);

    const hn::Vec<D> carry_bit_mask_16x16 = hn::Set(d, carry_bit_mask);
    const hn::Vec<D> counter_bias_16x16 = hn::Set(d, counter_bias);

    for (size_t i = 0; i < locks.size(); ++i) {
        const hn::Vec<D> vlock = hn::Set(d, locks[i]);
        const hn::Vec<D> vlock_biased = vlock + counter_bias_16x16;

        for (size_t j = 0; j < keys.size(); j += hn::Lanes(d)) {
            const hn::Vec<D> vkeys = hn::LoadU(d, &keys[j]);
            const hn::Vec<D> vcarries = vlock_biased ^ vkeys ^ (vlock_biased + vkeys);
            const hn::Vec<D> vgroup_carry_in = vcarries & carry_bit_mask_16x16;
            const hn::Mask<D> vgood = hn::Eq(vgroup_carry_in, hn::Zero(d));
            count = hn::MaskedAddOr(count, vgood, count, hn::Set(d, 1));
        }
    }

    fmt::print("{}\n", hn::ReduceSum(d, count));
}

}
