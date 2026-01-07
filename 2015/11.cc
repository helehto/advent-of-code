#include "common.h"

namespace aoc_2015_11 {

static void increment(std::span<uint8_t, 8> s)
{
    for (size_t i = s.size(); i--;) {
        s[i]++;
        if (s[i] == 'i' - 'a' || s[i] == 'o' - 'a' || s[i] == 'l' - 'a')
            s[i]++;
        if (s[i] < 26)
            return;
        s[i] = 0;
    }
}

static bool has_increasing_triple(std::span<const uint8_t, 8> s)
{
    return (s[0] + 1 == s[1] && s[1] + 1 == s[2]) ||
           (s[1] + 1 == s[2] && s[2] + 1 == s[3]) ||
           (s[2] + 1 == s[3] && s[3] + 1 == s[4]) ||
           (s[3] + 1 == s[4] && s[4] + 1 == s[5]) ||
           (s[4] + 1 == s[5] && s[5] + 1 == s[6]) ||
           (s[5] + 1 == s[6] && s[6] + 1 == s[7]);
}

static bool has_nonoverlapping_pairs(std::span<const uint8_t, 8> s)
{
    uint32_t mask = 0;

    for (size_t i = 1; i < s.size(); i++) {
        if (s[i - 1] == s[i])
            mask |= 1U << s[i];
    }

    return std::popcount(mask) >= 2;
}

void run(std::string_view buf)
{
    ASSERT(buf.size() == 8);
    std::array<uint8_t, 8> s;
    std::ranges::transform(buf, s.begin(), λx(x - 'a'));

    for (int i = 0; i < 2; i++) {
        do {
            increment(s);
        } while (!has_nonoverlapping_pairs(s) || !has_increasing_triple(s));
        for (uint8_t c : s)
            fputc(c + 'a', stdout);
        fputc('\n', stdout);
    }
}

}
