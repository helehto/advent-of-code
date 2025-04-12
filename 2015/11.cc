#include "common.h"

namespace aoc_2015_11 {

static void increment(std::string &s)
{
    for (size_t i = s.size(); i--;) {
        s[i]++;
        if (s[i] <= 'z')
            return;
        s[i] = 'a';
    }
}

static bool has_increasing_triple(std::string_view s)
{
    for (size_t i = 2; i < s.size(); i++) {
        if (s[i - 2] + 1 == s[i - 1] && s[i - 1] + 1 == s[i])
            return true;
    }

    return false;
}

static bool has_nonoverlapping_pairs(std::string_view s)
{
    uint32_t mask = 0;

    for (size_t i = 1; i < s.size(); i++) {
        if (s[i - 1] == s[i])
            mask |= 1U << (s[i] - 'a');
    }

    return __builtin_popcount(mask) >= 2;
}

static bool is_valid(std::string_view s)
{
    if (std::any_of(s.begin(), s.end(), λx(x == 'i' || x == 'o' || x == 'l')))
        return false;

    return has_increasing_triple(s) && has_nonoverlapping_pairs(s);
}

void run(std::string_view buf)
{
    std::string s(buf);

    for (int i = 0; i < 2; i++) {
        do {
            increment(s);
        } while (!is_valid(s));
        fmt::print("{}\n", s);
    }
}

}
