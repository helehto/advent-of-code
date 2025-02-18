#include "common.h"

namespace aoc_2015_5 {

static bool part1(std::string_view s)
{
    int vowels = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (0x104111 & (1 << (s[i] - 'a')))
            vowels++;
    }

    bool has_doubles = false;
    for (size_t i = 1; i < s.size(); i++) {
        if (s[i - 1] == s[i]) {
            has_doubles = true;
            break;
        }
    }

    for (size_t i = 1; i < s.size(); i++) {
        auto v = std::string_view(s.data() + i - 1, 2);
        if (v == "ab" || v == "cd" || v == "pq" || v == "xy")
            return false;
    }

    return vowels >= 3 && has_doubles;
}

static bool part2(std::string_view s)
{
    bool has_repeated = false;
    for (size_t i = 2; i < s.size(); i++) {
        if (s[i - 2] == s[i]) {
            has_repeated = true;
            break;
        }
    }

    bool has_pair = false;
    for (size_t i = 0; i + 3 < s.size(); i++) {
        for (size_t j = i + 2; j + 1 < s.size(); j++) {
            auto a = std::string_view(s.data() + i, 2);
            auto b = std::string_view(s.data() + j, 2);
            if (a == b) {
                has_pair = true;
                break;
            }
        }
    }

    return has_repeated && has_pair;
}

void run(std::string_view buf)
{
    int count1 = 0;
    int count2 = 0;

    for (std::string_view s : split_lines(buf)) {
        count1 += part1(s);
        count2 += part2(s);
    }

    fmt::print("{}\n", count1);
    fmt::print("{}\n", count2);
}

}
