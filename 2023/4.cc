#include "common.h"
#include <numeric>

namespace aoc_2023_4 {

void run(std::string_view buf)
{
    small_vector<int, 256> matches;
    small_vector<int, 64> v;
    for (std::string_view s : split_lines(buf)) {
        find_numbers(s, v);

        uint64_t winning[2] = {0};
        for (size_t i = 1; i < 11; i++)
            winning[v[i] / 64] |= UINT64_C(1) << (v[i] % 64);

        uint64_t have[2] = {0};
        for (size_t i = 11; i < v.size(); i++)
            have[v[i] / 64] |= UINT64_C(1) << (v[i] % 64);

        int n_correct =
            std::popcount(winning[0] & have[0]) + std::popcount(winning[1] & have[1]);
        matches.push_back(n_correct);
    }

    int score_sum = 0;
    for (int v : matches) {
        if (v)
            score_sum += 1 << (v - 1);
    }
    fmt::print("{}\n", score_sum);

    small_vector<int, 256> n_cards(matches.size(), 1);
    for (size_t i = 0; i < n_cards.size(); i++) {
        for (int j = 0; j < matches[i]; j++)
            n_cards[i + j + 1] += n_cards[i];
    }
    fmt::print("{}\n", std::accumulate(n_cards.begin(), n_cards.end(), 0));
}

}
