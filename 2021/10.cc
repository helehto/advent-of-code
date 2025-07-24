#include "common.h"

namespace aoc_2021_10 {

constexpr auto part1_score_table = [] {
    std::array<int, 256> t{};
    t[')'] = 3;
    t[']'] = 57;
    t['}'] = 1197;
    t['>'] = 25137;
    return t;
}();

constexpr static auto part2_score_table = [] {
    std::array<int, 256> t{};
    t[')'] = 1;
    t[']'] = 2;
    t['}'] = 3;
    t['>'] = 4;
    return t;
}();

constexpr static auto matching_pairs = [] {
    std::array<char, 256> t{};
    t['('] = ')';
    t['['] = ']';
    t['{'] = '}';
    t['<'] = '>';
    return t;
}();

void run(std::string_view buf)
{
    int score1 = 0;
    std::array<char, 64> stack;
    small_vector<uint64_t, 64> scores2;

    for (std::string_view s : split_lines(buf)) {
        char *p = stack.data();
        int mismatch_score = 0;

        for (const char c : s) {
            if (const char m = matching_pairs[c]) {
                *p++ = m;
            } else {
                if (c != *--p)
                    mismatch_score += part1_score_table[static_cast<uint8_t>(c)];
            }
        }

        if (mismatch_score == 0) {
            uint64_t score2 = 0;
            for (size_t i = p - stack.data(); i--;)
                score2 = 5 * score2 + part2_score_table[stack[i]];
            if (score2)
                scores2.push_back(score2);
        }

        score1 += mismatch_score;
    }

    fmt::print("{}\n", score1);
    std::ranges::nth_element(scores2, scores2.begin() + scores2.size() / 2);
    fmt::print("{}\n", scores2[scores2.size() / 2]);
}

}
