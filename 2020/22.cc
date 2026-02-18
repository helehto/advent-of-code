#include "common.h"
#include "dense_set.h"
#include <hwy/highway.h>

namespace aoc_2020_22 {

using namespace std::literals;

struct Deck {
    static constexpr size_t NUM_CARDS = 64;

    using D = hn::CappedTag<uint8_t, NUM_CARDS>;
    static constexpr D d{};
    static_assert(NUM_CARDS % hn::Lanes(d) == 0);

    // Array of cards, in order, with the oldest card at index 0. All valid
    // cards are grouped at the start of the array, with remaining elements set
    // to zero.
    //
    // The size is +1 to be able to read single card (logically) out of bounds.
    std::array<uint8_t, NUM_CARDS + 1> cards{};

    Deck prefix(size_t len) const
    {
        Deck result;
        const hn::Vec<D> v = hn::LoadU(d, cards.data());
        hn::StoreN(v, d, result.cards.data(), len);
        return result;
    }

    uint8_t max() const
    {
        uint8_t result = 0;

        for (size_t i = 0; i < NUM_CARDS; i += hn::Lanes(d)) {
            const hn::Vec<D> v = hn::LoadU(d, &cards[i]);
            result = std::max<uint8_t>(result, hn::ReduceMax(d, v));
        }

        return result;
    }

    uint8_t draw_card()
    {
        auto result = cards[0];

        // Left-rotate by overlapping loads/stores that are offset by one byte.
        for (size_t i = 0; i < NUM_CARDS; i += hn::Lanes(d)) {
            const hn::Vec<D> v = hn::LoadU(d, &cards[i + 1]);
            hn::StoreU(v, d, &cards[i]);
        }

        return result;
    }

    size_t size() const
    {
        size_t result = 0;
        const hn::Vec<D> zero = hn::Zero(d);

        for (size_t i = 0; i < NUM_CARDS; i += hn::Lanes(d)) {
            const hn::Vec<D> v = hn::LoadU(d, &cards[i]);
            result += hn::CountTrue(d, hn::Ne(v, zero));
        }

        return result;
    }

    void push(int card1, int card2)
    {
        uint8_t *e = cards.data() + size();
        e[0] = card1;
        e[1] = card2;
    }

    bool operator==(const Deck &other) const
    {
        for (size_t i = 0; i < 64; i += hn::Lanes(d)) {
            const hn::Vec<D> a = hn::LoadU(d, &cards[i]);
            const hn::Vec<D> b = hn::LoadU(d, &other.cards[i]);
            if (!hn::AllTrue(d, hn::Eq(a, b)))
                return false;
        }

        return true;
    }

    size_t score() const
    {
        size_t result = 0;
        for (size_t i = 0, n = size(); i < n; ++i)
            result += cards[i] * (n - i);
        return result;
    }
};

struct Decks {
    Deck first, second;
    bool operator==(const Decks &) const noexcept = default;
};

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
    const uint8_t max_a = a.max();
    const uint8_t max_b = b.max();
    if (max_a > max_b && max_a > a_initial_size + b_initial_size)
        return true;

    dense_set<Decks, CrcHasher> seen;
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
