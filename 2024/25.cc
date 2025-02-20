#include "common.h"

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
    std::vector<uint16_t> locks;
    std::vector<uint16_t> keys;
    locks.reserve(256);
    keys.reserve(256);

    size_t i = 0;
    for (; i + 3 < buf.size(); i += 6 * 7 + 1) {
        __m256i vchars =
            _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&buf[i + 6]));
        __m256i vfilled = _mm256_cmpeq_epi8(vchars, _mm256_set1_epi8('#'));
        unsigned int filled = _mm256_movemask_epi8(vfilled);

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
            locks.push_back(u);
        else
            keys.push_back(u);
    }

    // Make sure the key vector is a multiple of 128 so that we can use 8-way
    // unrolling in the loop below without needing a 1-way vectorized or scalar
    // fallback for the remaining elements. The value 0xffff will always result
    // in an overlap, so this does not affect the final value.
    if (auto r = keys.size() % 128; r != 0)
        for (size_t i = 0; i < 128 - r; ++i)
            keys.push_back(0xffff);

    uint64_t count = 0;
    __m256i vcount[8]{};

    const __m256i carry_bit_mask_16x16 = _mm256_set1_epi16(carry_bit_mask);

    auto compatible16 = [&](const __m256i vlock_biased, size_t j, __m256i &count) {
        __m256i vkeys = _mm256_loadu_si256(reinterpret_cast<__m256i *>(&keys[j]));
        __m256i vsum = _mm256_add_epi16(vlock_biased, vkeys);
        __m256i vcarries = _mm256_xor_si256(_mm256_xor_si256(vlock_biased, vkeys), vsum);
        __m256i vgroup_carry_in = _mm256_and_si256(vcarries, carry_bit_mask_16x16);
        __m256i vgood = _mm256_cmpeq_epi16(vgroup_carry_in, _mm256_setzero_si256());
        count = _mm256_sub_epi16(count, vgood);
    };

    for (size_t i = 0; i < locks.size(); ++i) {
        const __m256i counter_bias_16x16 = _mm256_set1_epi16(counter_bias);
        const __m256i vlock = _mm256_set1_epi16(locks[i]);
        const __m256i vlock_biased = _mm256_add_epi16(vlock, counter_bias_16x16);

        for (size_t j = 0; j < keys.size(); j += 128) {
            compatible16(vlock_biased, j + 0 * 16, vcount[0]);
            compatible16(vlock_biased, j + 1 * 16, vcount[1]);
            compatible16(vlock_biased, j + 2 * 16, vcount[2]);
            compatible16(vlock_biased, j + 3 * 16, vcount[3]);
            compatible16(vlock_biased, j + 4 * 16, vcount[4]);
            compatible16(vlock_biased, j + 5 * 16, vcount[5]);
            compatible16(vlock_biased, j + 6 * 16, vcount[6]);
            compatible16(vlock_biased, j + 7 * 16, vcount[7]);
        }
    }

    for (size_t j = 0; j < std::size(vcount); ++j) {
        std::array<uint16_t, 16> c;
        _mm256_store_si256(reinterpret_cast<__m256i *>(&c), vcount[j]);
        for (size_t k = 0; k < 16; ++k)
            count += c[k];
    }

    fmt::print("{}\n", count);
}

}
