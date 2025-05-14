#include "common.h"
#include "dense_set.h"

namespace aoc_2020_22 {

struct Deck {
    std::array<uint8_t, 65> cards;

    Deck()
    {
        const __m256i zero = _mm256_setzero_si256();
        _mm256_storeu_si256((__m256i *)&cards[0], zero);
        _mm256_storeu_si256((__m256i *)&cards[32], zero);
        cards[64] = 0;
    }

    Deck prefix(size_t len) const
    {
        Deck result;
        std::copy(cards.begin(), cards.begin() + len, result.cards.begin());
        return result;
    }

    uint8_t draw_card()
    {
        auto result = cards[0];

        // Left-rotate by two pairs of overlapping 32-byte loads/stores that
        // are offset by one byte. (Note that this requires the array to be 65
        // bytes long to not read out of bounds.)
        const __m256i asrc = _mm256_loadu_si256((const __m256i *)&cards[1]);
        const __m256i bsrc = _mm256_loadu_si256((const __m256i *)&cards[33]);
        _mm256_storeu_si256((__m256i *)&cards[0], asrc);
        _mm256_storeu_si256((__m256i *)&cards[32], bsrc);
        cards[64] = 0;

        return result;
    }

    size_t size() const
    {
        const __m256i zero = _mm256_setzero_si256();
        const __m256i a = _mm256_loadu_si256((const __m256i *)&cards[0]);
        const __m256i b = _mm256_loadu_si256((const __m256i *)&cards[32]);
        const unsigned int cmplo = _mm256_movemask_epi8(_mm256_cmpgt_epi8(a, zero));
        const unsigned int cmphi = _mm256_movemask_epi8(_mm256_cmpgt_epi8(b, zero));
        return std::popcount((uint64_t)cmphi << 32 | cmplo);
    }

    void push(int card1, int card2)
    {
        uint8_t *e = cards.data() + size();
        e[0] = card1;
        e[1] = card2;
    }

    bool operator==(const Deck &other) const
    {
        const __m256i a0 = _mm256_loadu_si256((const __m256i *)&cards[0]);
        const __m256i a1 = _mm256_loadu_si256((const __m256i *)&other.cards[0]);
        const __m256i b0 = _mm256_loadu_si256((const __m256i *)&cards[32]);
        const __m256i b1 = _mm256_loadu_si256((const __m256i *)&other.cards[32]);
        return (uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(a0, a1)) == 0xffffffff &&
               (uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(b0, b1)) == 0xffffffff;
    }

    size_t score() const
    {
        size_t result = 0;
        for (size_t i = 0, n = size(); i < n; ++i)
            result += cards[i] * (n - i);
        return result;
    }
};

}

template <>
struct std::hash<std::pair<aoc_2020_22::Deck, aoc_2020_22::Deck>> {
    size_t
    operator()(const std::pair<aoc_2020_22::Deck, aoc_2020_22::Deck> &p) const noexcept
    {
        size_t h1 = 0;
        size_t h2 = 0;

        auto *a = reinterpret_cast<const uint64_t *>(p.first.cards.data());
        auto *b = reinterpret_cast<const uint64_t *>(p.second.cards.data());

        for (size_t i = 0; i < 8; ++i)
            h1 = _mm_crc32_u64(h1, a[i]);
        for (size_t i = 0; i < 8; ++i)
            h2 = _mm_crc32_u64(h2, b[i]);

        return h1 ^ h2;
    }
};

namespace aoc_2020_22 {

using namespace std::literals;

static int part1(Deck a, Deck b)
{
    while (a.cards[0] != 0 && b.cards[0] != 0) {
        auto ca = a.draw_card();
        auto cb = b.draw_card();
        if (ca > cb)
            a.push(ca, cb);
        else
            b.push(cb, ca);
    }

    return a.cards[0] != 0 ? a.score() : b.score();
}

static bool recursive_combat(Deck &&a, Deck &&b, bool is_root_game = true)
{
    const auto a_initial_size = a.size();
    const auto b_initial_size = b.size();

    // Credits for the optimization below to /u/curious_sapi3n on Reddit:
    // <https://old.reddit.com/r/adventofcode/comments/khyjgv/2020_day_22_solutions/ggpcsnd/>
    uint8_t max_a = *std::ranges::max_element(a.cards);
    uint8_t max_b = *std::ranges::max_element(b.cards);
    if (max_a > max_b && max_a > a_initial_size + b_initial_size)
        return true;

    dense_set<std::pair<Deck, Deck>> seen;
    seen.reserve(4 * a_initial_size * b_initial_size);

    while (true) {
        if (a.cards[0] == 0 || b.cards[0] == 0) {
            if (is_root_game)
                fmt::print("{}\n", a.cards[0] != 0 ? a.score() : b.score());
            return a.cards[0] != 0;
        }

        if (auto [it, inserted] = seen.emplace(a, b); !inserted) {
            if (is_root_game)
                fmt::print("{}\n", a.score());
            return true;
        }

        const auto ca = a.draw_card();
        const auto cb = b.draw_card();
        bool a_wins = a.cards[ca - 1] != 0 && b.cards[cb - 1] != 0
                          ? recursive_combat(a.prefix(ca), b.prefix(cb), false)
                          : ca > cb;

        if (a_wins)
            a.push(ca, cb);
        else
            b.push(cb, ca);
    }
}

void run(std::string_view buf)
{
    auto sep = buf.find("\n\n");
    auto a = find_numbers<uint8_t>(buf.substr("Player x:"sv.size(), sep));
    auto b = find_numbers<uint8_t>(buf.substr(sep + "\n\nPlayer x:"sv.size()));

    Deck deck_a;
    Deck deck_b;
    std::ranges::copy(a, deck_a.cards.data());
    std::ranges::copy(b, deck_b.cards.data());

    fmt::print("{}\n", part1(deck_a, deck_b));
    recursive_combat(std::move(deck_a), std::move(deck_b));
}

}
