#include "common.h"
#include "dense_set.h"

namespace aoc_2016_10 {

void run(std::string_view buf)
{
    constexpr int max_bots = 256;

    std::array<int16_t, max_bots> output;
    output.fill(-1);

    std::array<int16_t, 2 * max_bots> held;
    held.fill(-1);

    dense_set<int> remaining;
    std::vector<int> pending;

    auto take = [&](size_t bot) {
        int16_t *chips = &held[2 * bot];
        auto [a, b] = std::pair(chips[0], chips[1]);
        ASSERT(a >= 0);
        ASSERT(b >= 0);
        chips[0] = -1;
        chips[1] = -1;
        return std::pair(a, b);
    };

    auto give = [&](int16_t chip, int16_t to) {
        if (to < 0) {
            output[-to] = chip;
            remaining.erase(chip);
            return;
        }

        int16_t *chips = &held[2 * to];
        ASSERT(chips[1] < 0);
        if (chips[0] < 0) {
            chips[0] = chip;
        } else {
            chips[1] = std::max(chips[0], chip);
            chips[0] = std::min(chips[0], chip);
            pending.push_back(to);
        }
    };

    std::array<std::array<int16_t, 2>, max_bots> rules;
    rules.fill(std::array<int16_t, 2>{INT16_MIN, INT16_MIN});
    for (std::string_view line : split_lines(buf)) {
        if (line.starts_with("value"))
            continue;

        auto i = line.find(" low to ");
        ASSERT(i != std::string_view::npos);
        auto j = line.find(" and high to ");
        ASSERT(j != std::string_view::npos);

        auto [from] = find_numbers_n<int, 1>(line.substr(0, i));

        auto parse_rule = [&](std::string_view s) {
            auto [n] = find_numbers_n<int, 1>(s);
            return s.starts_with("output") ? -n : n;
        };

        rules[from][0] = parse_rule(line.substr(i + 8, j - i - 8));
        rules[from][1] = parse_rule(line.substr(j + 13));
    }

    for (std::string_view line : split_lines(buf)) {
        if (line.starts_with("value")) {
            auto [chip, bot] = find_numbers_n<int, 2>(line);
            give(chip, bot);
            remaining.insert(chip);
        }
    }

    int bot_17_61 = -1;
    while (remaining.size() > 1) {
        ASSERT(!pending.empty());
        auto bot = pending.back();
        pending.pop_back();

        auto [a, b] = take(bot);
        give(a, rules[bot][0]);
        give(b, rules[bot][1]);

        if (a == 17 && b == 61)
            bot_17_61 = bot;
    }
    ASSERT(bot_17_61 != -1);

    for (size_t i = 0; i < 3; ++i) {
        if (output[i] < 0) {
            output[i] = *remaining.begin();
            break;
        }
    }

    fmt::print("{}\n", bot_17_61);
    fmt::print("{}\n", output[0] * output[1] * output[2]);
}

}
