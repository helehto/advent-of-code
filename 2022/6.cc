#include "common.h"

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

void run(std::string_view buf)
{
    fmt::print("{}\n", solve(buf, 4));
    fmt::print("{}\n", solve(buf, 14));
}

}
