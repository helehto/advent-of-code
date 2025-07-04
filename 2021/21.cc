#include "common.h"
#include "dense_map.h"

namespace aoc_2021_21 {

constexpr int part1(std::array<int, 2> pos)
{
    std::array<int, 2> scores = {0, 0};

    int die = 1;
    auto roll = [&die] {
        if (die >= 101)
            die = 1;
        return die++;
    };

    int p, r;
    for (r = 0;; ++r) {
        p = r % 2;
        pos[p] = (pos[p] + roll() + roll() + roll() - 1) % 10 + 1;
        scores[p] += pos[p];
        if (scores[p] >= 1000)
            break;
    }

    return scores[1 - p] * 3 * (r + 1);
}

constexpr auto rolls = [] {
    std::array<int, 10> result{};
    for (int a = 1; a <= 3; ++a)
        for (int b = 1; b <= 3; ++b)
            for (int c = 1; c <= 3; ++c)
                result[a + b + c]++;
    return result;
}();

struct State {
    std::array<int64_t, 2> score{0, 0};
    std::array<uint8_t, 2> pos;
    uint8_t turn = 0;
    std::array<uint8_t, 5> padding{0, 0, 0, 0, 0};

    constexpr bool operator==(const State &) const = default;
};
static_assert(sizeof(State) == 24);

constexpr std::pair<int64_t, int64_t>
part2(dense_map<State, std::pair<int64_t, int64_t>, CrcHasher> &cache, const State &state)
{
    if (state.score[0] >= 21)
        return {1, 0};
    if (state.score[1] >= 21)
        return {0, 1};

    auto [it, inserted] = cache.try_emplace(state, -1, -1);
    if (!inserted && it->second.first >= 0)
        return it->second;

    std::pair<int64_t, int64_t> result{0, 0};
    for (size_t sum = 0; sum < rolls.size(); ++sum) {
        const int freq = rolls[sum];
        if (!freq)
            continue;

        const int turn = state.turn;
        const uint8_t new_pos = (state.pos[turn] + sum - 1) % 10 + 1;

        State new_state = state;
        new_state.score[turn] += new_pos;
        new_state.pos[turn] = new_pos;
        new_state.turn = !turn;

        auto winner = part2(cache, new_state);
        result.first += freq * winner.first;
        result.second += freq * winner.second;
    }

    return it->second = result;
}

void run(std::string_view buf)
{
    auto [_1, a, _2, b] = find_numbers_n<uint8_t, 4>(buf);
    fmt::print("{}\n", part1({a, b}));

    dense_map<State, std::pair<int64_t, int64_t>, CrcHasher> cache;
    cache.reserve(1 << 16);
    auto [a_universes, b_universes] = part2(cache, State{.pos = {a, b}});
    fmt::print("{}\n", std::max(a_universes, b_universes));
}

}
