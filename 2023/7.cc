#include "common.h"
#include <cstdlib>
#include <optional>
#include <x86intrin.h>

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
    alignas(16) std::array<uint8_t, 16> counts;
    std::array<uint8_t, 5> cards;
    uint8_t eval;
    uint16_t bet;
};

static std::pair<uint8_t, uint8_t> evaluate_hand(uint8_t *counts)
{
    const __m128i vcounts = _mm_lddqu_si128(reinterpret_cast<__m128i *>(counts));
    const int mask5 = _mm_movemask_epi8(_mm_cmpeq_epi8(vcounts, _mm_set1_epi8(5)));
    const int mask4 = _mm_movemask_epi8(_mm_cmpeq_epi8(vcounts, _mm_set1_epi8(4)));
    const int mask3 = _mm_movemask_epi8(_mm_cmpeq_epi8(vcounts, _mm_set1_epi8(3)));
    const int mask2 = _mm_movemask_epi8(_mm_cmpeq_epi8(vcounts, _mm_set1_epi8(2)));
    const int mask1 = _mm_movemask_epi8(_mm_cmpeq_epi8(vcounts, _mm_set1_epi8(1)));

    if (mask5)
        return {five_of_a_kind, __builtin_ctz(mask5)};

    if (mask4)
        return {four_of_a_kind, __builtin_ctz(mask4)};

    if (mask3)
        return {mask2 ? full_house : three_of_a_kind, __builtin_ctz(mask3)};

    if (mask2)
        return {(mask2 & (mask2 - 1)) ? two_pair : one_pair, __builtin_ctz(mask2)};

    ASSERT(mask1 != 0);
    return {high_card, __builtin_ctz(mask1)};
}

static uint8_t evaluate_hand_with_jokers(uint8_t *counts)
{
    const int n_jokers = std::exchange(counts[11], 0);

    // Special case: all jokers gives five aces as the best possible hand.
    if (n_jokers == 5)
        return five_of_a_kind;

    auto best = evaluate_hand(counts).second;
    counts[best] += n_jokers;
    return evaluate_hand(counts).first;
}

static int total_winnings(std::vector<Hand> hands)
{
    std::sort(hands.begin(), hands.end(), [](const Hand &h1, const Hand &h2) {
        if (h1.eval != h2.eval)
            return h1.eval < h2.eval;

        for (size_t k = 0; k < h1.cards.size(); k++) {
            if (h1.cards[k] != h2.cards[k])
                return h1.cards[k] < h2.cards[k];
        }

        return false;
    });

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

void run_2023_7(FILE *f)
{
    std::string s;
    std::vector<std::string_view> sv;
    std::vector<Hand> hands;
    while (getline(f, s)) {
        split(s, sv, ' ');
        Hand hand;
        hand.counts.fill(0);
        for (size_t i = 0; i < 5; i++) {
            hand.cards[i] = ascii_card_value_table[sv[0][i]];
            hand.counts[hand.cards[i]]++;
        }
        std::from_chars(sv[1].begin(), sv[1].end(), hand.bet);
        hand.eval = evaluate_hand(hand.counts.data()).first;
        hands.push_back(hand);
    }
    fmt::print("{}\n", total_winnings(hands));

    for (auto &h : hands) {
        for (auto &c : h.cards)
            c = c == 11 ? 0 : c;
        h.eval = evaluate_hand_with_jokers(h.counts.data());
    }
    fmt::print("{}\n", total_winnings(std::move(hands)));
}
