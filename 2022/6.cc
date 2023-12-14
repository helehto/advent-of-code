#include "common.h"

static size_t solve(std::string_view s, size_t n)
{
    size_t i = 0;
    unsigned int mask = 0;

    for (; i < n; i++)
        mask ^= 1U << (s[i] - 'a');

    for (; __builtin_popcount(mask) != n; i++) {
        mask ^= 1U << (s[i - n] - 'a');
        mask ^= 1U << (s[i] - 'a');
    }

    return i;
}

void run_2022_6(FILE *f)
{
    std::string s;
    getline(f, s);
    fmt::print("{}\n", solve(s, 4));
    fmt::print("{}\n", solve(s, 14));
}
