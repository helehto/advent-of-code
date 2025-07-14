#include "common.h"
#include "dense_map.h"

namespace aoc_2017_16 {

constexpr std::array<uint8_t, 16> identity_permutation = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

/// Pre-computed 16 rotation shuffle control masks for the 's' move:
alignas(16) static constexpr std::array<uint8_t, 16> spin_shuffles[] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
    {14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
    {13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
    {12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
    {10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    {9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8},
    {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7},
    {7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6},
    {6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5},
    {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4},
    {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3},
    {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2},
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1},
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0},
};

/// Pre-computed 256 exchange shuffle control masks for the 'x' and 'p' moves:
alignas(16) static constexpr auto exchange_shuffles = [] consteval {
    std::array<std::array<uint8_t, 16>, 256> result;
    result.fill(identity_permutation);

    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            std::swap(result[16 * i + j][i], result[16 * i + j][j]);

    return result;
}();

/// Parsed form of the input moves.
///
/// Since the 's' and 'x' moves are fixed input permutations, any consecutive
/// sequence of them can be composed into a single permutation. After applying
/// this to compress the input sequence, we have exactly one fixed permutation
/// that has to be applied between every 'p' move. (Even if no permutation is
/// strictly required, we still apply an identity permutation for simplicity
/// and to avoid branching.)
struct Moves {
    struct CompressedMove {
        __m128i ctrl;    // combined permutation of 's'/'x'; applied first
        __m128i swap_c1; // broadcasted first operand for 'p'
        __m128i swap_c2; // broadcasted second operand for 'p'
    };

    std::vector<CompressedMove> moves;
    __m128i last_ctrl;
};

[[gnu::noinline]] static Moves parse_moves(std::string_view buf)
{
    const size_t num_moves = 1 + std::ranges::count(buf, ',');

    std::vector<uint8_t> nums;
    nums.reserve(2 * num_moves);
    find_numbers(buf, nums);

    const uint8_t *n = nums.data();
    std::vector<std::string_view> tmp;
    tmp.reserve(num_moves);
    split(buf, tmp, ',');

    Moves result;
    result.moves.reserve(num_moves);

    __m128i perm = _mm_loadu_si128((const __m128i *)&identity_permutation);
    for (std::string_view move : tmp) {
        if (move[0] == 's') {
            DEBUG_ASSERT(*n < 16);
            auto *shuf = (const __m128i *)&spin_shuffles[*n++];
            perm = _mm_shuffle_epi8(perm, _mm_loadu_si128(shuf));
        } else if (move[0] == 'x') {
            DEBUG_ASSERT(n[0] < 16);
            DEBUG_ASSERT(n[1] < 16);
            auto *shuf = (const __m128i *)&exchange_shuffles[n[0] * 16 + n[1]];
            perm = _mm_shuffle_epi8(perm, _mm_loadu_si128(shuf));
            n += 2;
        } else if (move[0] == 'p') {
            DEBUG_ASSERT(move[1] >= 'a' && move[1] <= 'p');
            DEBUG_ASSERT(move[3] >= 'a' && move[3] <= 'p');
            result.moves.emplace_back(perm, _mm_set1_epi8(move[1] - 'a'),
                                      _mm_set1_epi8(move[3] - 'a'));
            perm = _mm_loadu_si128((const __m128i *)&identity_permutation);
        }
    }

    result.last_ctrl = perm;
    return result;
}

static void dance(std::span<uint8_t, 16> programs, const Moves &m)
{
    __m128i perm = _mm_loadu_si128((__m128i *)programs.data());

    auto index_of = [&](__m128i c) {
        const __m128i eq = _mm_cmpeq_epi8(perm, c);
        const unsigned mask = _mm_movemask_epi8(eq);
        return std::countr_zero(mask);
    };

    for (const Moves::CompressedMove &move : m.moves) {
        perm = _mm_shuffle_epi8(perm, move.ctrl);
        const size_t i = index_of(move.swap_c1);
        const size_t j = index_of(move.swap_c2);
        auto *shuf = (const __m128i *)&exchange_shuffles[16 * i + j];
        perm = _mm_shuffle_epi8(perm, _mm_loadu_si128(shuf));
    }

    perm = _mm_shuffle_epi8(perm, m.last_ctrl);
    _mm_storeu_si128((__m128i *)programs.data(), perm);
}

static void print_permutation(std::span<uint8_t, 16> p)
{
    for (uint8_t c : p)
        putc(c + 'a', stdout);
    putc('\n', stdout);
}

void run(std::string_view buf)
{
    const auto moves = parse_moves(buf);

    alignas(16) std::array<uint8_t, 16> programs = identity_permutation;

    // Part 1:
    {
        dance(programs, moves);
        print_permutation(programs);
    }

    // Part 2:
    {
        small_vector<std::array<uint8_t, 16>, 64> sequence{programs};
        dense_map<std::array<uint8_t, 16>, int, CrcHasher> seen;
        seen.reserve(100);
        for (size_t i = 1; seen.emplace(programs, i).second; ++i) {
            sequence.push_back(programs);
            dance(programs, moves);
        }
        sequence.pop_back();
        print_permutation(sequence[1'000'000'000 % sequence.size()]);
    }
}

}
