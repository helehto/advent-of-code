#include "common.h"
#include <hwy/highway.h>

namespace aoc_2023_7 {

enum : uint8_t {
    high_card,
    one_pair,
    two_pair,
    three_of_a_kind,
    full_house,
    four_of_a_kind,
    five_of_a_kind,
};

struct Hand {
    // The evaulation and cards are the key used when sorting the hands, in
    // that order; if we pack both together as 4-bit quantities in the correct
    // order, we can compare two hands with a single 32-bit comparison.
    //
    // Bits [0:3]   = Card #5
    // Bits [4:7]   = Card #4
    // Bits [8:12]  = Card #3
    // Bits [13:16] = Card #3
    // Bits [17:20] = Card #1
    // Bits [20:22] = Hand evaulation
    uint32_t cards_and_eval;

    uint16_t bet;
};

[[gnu::noinline]]
static std::pair<uint32_t, uint8_t> evaluate_hand(uint8_t *counts)
{
    using D = hn::FixedTag<uint8_t, 16>;
    constexpr D d;
    const hn::Vec<D> v = hn::Load(d, counts);

    const hn::Mask<D> mask5 = hn::Eq(v, hn::Set(d, 5));
    if (intptr_t b5 = hn::FindFirstTrue(d, mask5); b5 >= 0)
        return {five_of_a_kind, static_cast<uint8_t>(b5)};

    const hn::Mask<D> mask4 = hn::Eq(v, hn::Set(d, 4));
    if (intptr_t b4 = hn::FindFirstTrue(d, mask4); b4 >= 0)
        return {four_of_a_kind, static_cast<uint8_t>(b4)};

    const hn::Mask<D> mask3 = hn::Eq(v, hn::Set(d, 3));
    const hn::Mask<D> mask2 = hn::Eq(v, hn::Set(d, 2));
    const intptr_t b2 = hn::FindFirstTrue(d, mask2);
    if (intptr_t b3 = hn::FindFirstTrue(d, mask3); b3 >= 0)
        return {b2 >= 0 ? full_house : three_of_a_kind, static_cast<uint8_t>(b3)};

    if (b2 >= 0) {
        const uint64_t m = hn::BitsFromMask(d, mask2);
        return {std::has_single_bit(m) ? one_pair : two_pair, static_cast<uint8_t>(b2)};
    }

    const hn::Mask<D> mask1 = hn::Eq(v, hn::Set(d, 1));
    intptr_t b1 = hn::FindFirstTrue(d, mask1);
    ASSERT(b1 >= 0);
    return {high_card, static_cast<uint8_t>(b1)};
}

static uint32_t evaluate_hand_with_jokers(std::array<uint8_t, 16> &counts)
{
    const int n_jokers = std::exchange(counts[11], 0);

    // Special case: all jokers gives five aces as the best possible hand.
    if (n_jokers == 5)
        return five_of_a_kind;

    counts[std::ranges::max_element(counts) - counts.begin()] += n_jokers;
    return evaluate_hand(counts.data()).first;
}

static int total_winnings(std::vector<Hand> hands)
{
    std::ranges::sort(hands, {}, λx(x.cards_and_eval));

    int sum = 0;
    for (size_t i = 0; i < hands.size(); i++)
        sum += (i + 1) * hands[i].bet;
    return sum;
}

constexpr std::array<uint8_t, 256> ascii_card_value_table = [] {
    std::array<uint8_t, 256> v;
    v.fill(-1);
    for (int i = 0; i < 10; i++)
        v[i + '0'] = i;
    v['T'] = 10;
    v['J'] = 11;
    v['Q'] = 12;
    v['K'] = 13;
    v['A'] = 14;
    return v;
}();

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::vector<std::string_view> sv;
    std::vector<Hand> hands;
    hands.reserve(lines.size());
    std::vector<std::array<uint8_t, 16>> counts;
    counts.resize(lines.size());
    for (size_t i = 0; std::string_view s : lines) {
        split(s, sv, ' ');
        Hand hand{};
        counts[i].fill(0);
        for (size_t j = 0; j < 5; j++) {
            uint8_t c = ascii_card_value_table[sv[0][4 - j]];
            hand.cards_and_eval |= static_cast<uint32_t>(c) << (4 * j);
            counts[i][c]++;
        }
        std::from_chars(sv[1].begin(), sv[1].end(), hand.bet);
        hand.cards_and_eval |= evaluate_hand(counts[i].data()).first << 20;
        hands.push_back(hand);
        i++;
    }
    fmt::print("{}\n", total_winnings(hands));

    for (size_t i = 0; auto &h : hands) {
        // Clear jacks/jokers to 0 so that they get compared lower than anything else.
        for (int i = 0; i < 5; i++) {
            uint32_t card_mask = 0xf << (4 * i);
            if (((h.cards_and_eval ^ 0xbbbbb) & card_mask) == 0)
                h.cards_and_eval &= ~card_mask;
        }

        // Clear the old evaluation and re-evaluate with jokers.
        h.cards_and_eval &= ~0xf00000;
        h.cards_and_eval |= evaluate_hand_with_jokers(counts[i]) << 20;
        i++;
    }
    fmt::print("{}\n", total_winnings(std::move(hands)));
}

}
