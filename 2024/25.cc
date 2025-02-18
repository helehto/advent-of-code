#include "common.h"

namespace aoc_2024_25 {

// Each key and lock is represented using a 32-bit integer, with the lowest 20
// bits grouped into 5 groups of 4 bits each. The upper 12 bits are unused.
// Each group consists of a dedicated carry bit along with a 3-bit counter that
// holds the number of occupied slots in that column, excluding the top row and
// bottom row.
//
//                        carry bit --.   .-- 3-bit counter
//                                    |   |
//                                    v v-v-v
//   .-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-.
//   |            0          |C|x|y|z|C|x|y|z|C|x|y|z|C|x|y|z|C|x|y|z|
//   '-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-'
//
// With this representation, we can use a single uint32_t addition to add the
// number of occupied slots of all columns for a given (lock, key) pair. If the
// 3-bit counter for a group overflows, the carry bit for the group will be
// set. Checking whether addition has overflowed for _any_ column is then just
// a matter of checking if _any_ carry bit is set, i.e. (x & 0x88888) != 0.
//
// To check whether the total number of occupied slots in a column is greater
// than 5, we bias the addition by making each 3-bit counter start at 2 (or
// 0x22222 seen as a 32-bit word). This means we can add anything lower than 5
// to such a group without overflow, but anything greater will, again, set the
// carry bit.
//
// For a given lock + key, they fit if ((lock + key + 0x22222) & 0x88888) != 0.
// This is expression is trivially vectorizable to check a given lock against 8
// keys in parallel (with AVX).
constexpr uint32_t carry_bit_mask = 0x88888;
constexpr uint32_t counter_bias = 0x22222;

void run(std::string_view buf)
{
    std::vector<uint32_t> locks;
    std::vector<uint32_t> keys;
    locks.reserve(buf.size() / (6 * 7));
    keys.reserve(buf.size() / (6 * 7));

    size_t i = 0;
    for (; i + 3 < buf.size(); i += 6 * 7 + 1) {
        __m256i vchars =
            _mm256_loadu_si256(reinterpret_cast<const __m256i *>(&buf[i + 6]));
        __m256i vfilled = _mm256_cmpeq_epi8(vchars, _mm256_set1_epi8('#'));
        unsigned int filled = _mm256_movemask_epi8(vfilled);

        // Add an entire column/group at a time to `u`. The highest bit in each
        // group of 6 here is the newline, which we ignore:
        uint32_t u = 0;
        constexpr uint32_t column_mask = 0b000001'000001'000001'000001'000001;
        u += std::popcount(filled & (column_mask << 0)) << 0;
        u += std::popcount(filled & (column_mask << 1)) << 4;
        u += std::popcount(filled & (column_mask << 2)) << 8;
        u += std::popcount(filled & (column_mask << 3)) << 12;
        u += std::popcount(filled & (column_mask << 4)) << 16;

        // Split locks and keys into separate vectors so that we never check a
        // lock against a lock, or a key against a key.
        if (buf[i] == '#')
            locks.push_back(u);
        else
            keys.push_back(u);
    }

    uint64_t count = 0;
    __m256i vcount[4]{};

    const __m256i carry_bit_mask_8x32 = _mm256_set1_epi32(carry_bit_mask);
    const __m256i counter_bias_8x32 = _mm256_set1_epi32(counter_bias);

    auto compatible8 = [&](const __m256i vlock_biased, size_t j, __m256i &count) {
        __m256i vkeys = _mm256_loadu_si256(reinterpret_cast<__m256i *>(&keys[j]));
        __m256i vcols = _mm256_add_epi32(vlock_biased, vkeys);
        __m256i vcarry = _mm256_and_si256(vcols, carry_bit_mask_8x32);
        __m256i vgood = _mm256_cmpeq_epi32(vcarry, _mm256_setzero_si256());
        count = _mm256_sub_epi32(count, vgood);
    };

    for (size_t i = 0; i < locks.size(); ++i) {
        __m256i vlock = _mm256_set1_epi32(locks[i]);
        __m256i vlock_biased = _mm256_add_epi32(vlock, counter_bias_8x32);

        size_t j = 0;
        for (; j + 31 < keys.size(); j += 32) {
            compatible8(vlock_biased, j, vcount[0]);
            compatible8(vlock_biased, j + 8, vcount[1]);
            compatible8(vlock_biased, j + 16, vcount[2]);
            compatible8(vlock_biased, j + 24, vcount[3]);
        }

        for (; j + 7 < keys.size(); j += 8)
            compatible8(vlock_biased, j, vcount[0]);

        for (; j < keys.size(); ++j)
            count += ((locks[i] + keys[j] + counter_bias) & carry_bit_mask) == 0;
    }

    for (size_t j = 0; j < std::size(vcount); ++j) {
        std::array<uint32_t, 8> c;
        _mm256_store_si256(reinterpret_cast<__m256i *>(&c), vcount[j]);
        for (size_t k = 0; k < 8; ++k)
            count += c[k];
    }

    fmt::print("{}\n", count);
}

}
