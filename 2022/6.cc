#include <aoc/base.h>
#include <bit>
#include <cstddef>
#include <string_view>

namespace aoc_2022_6 {

static size_t solve(std::string_view s, int n)
{
    int i = 0;
    unsigned int mask = 0;

    for (; i < n; i++)
        mask ^= 1U << (s[i] - 'a');

    for (; std::popcount(mask) != n; i++) {
        mask ^= 1U << (s[i - n] - 'a');
        mask ^= 1U << (s[i] - 'a');
    }

    return i;
}

void run(std::string_view buf, aoc::Answer &answer)
{
    answer.add(solve(buf, 4));
    answer.add(solve(buf, 14));
}
AOC_REGISTER_SOLVER(2022, 6, run);

}
