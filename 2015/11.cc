#include "common.h"

namespace aoc_2015_11 {

static void increment(std::span<char> s)
{
    for (size_t i = s.size(); i--;) {
        s[i]++;
        if (s[i] == 'i' || s[i] == 'o' || s[i] == 'l')
            s[i]++;
        if (s[i] <= 'z')
            return;
        s[i] = 'a';
    }
}

static bool has_increasing_triple(std::string_view s)
{
    return (s[0] + 1 == s[1] && s[1] + 1 == s[2]) ||
           (s[1] + 1 == s[2] && s[2] + 1 == s[3]) ||
           (s[2] + 1 == s[3] && s[3] + 1 == s[4]) ||
           (s[3] + 1 == s[4] && s[4] + 1 == s[5]) ||
           (s[4] + 1 == s[5] && s[5] + 1 == s[6]) ||
           (s[5] + 1 == s[6] && s[6] + 1 == s[7]);
}

static bool has_nonoverlapping_pairs(std::string_view s)
{
    uint32_t mask = 0;

    for (size_t i = 1; i < s.size(); i++) {
        if (s[i - 1] == s[i])
            mask |= 1U << (s[i] - 'a');
    }

    return std::popcount(mask) >= 2;
}

void run(std::string_view buf)
{
    ASSERT(buf.size() == 8);
    std::string s(buf);

    for (int i = 0; i < 2; i++) {
        do {
            increment(s);
        } while (!has_increasing_triple(s) || !has_nonoverlapping_pairs(s));
        fmt::print("{}\n", s);
    }
}

}
