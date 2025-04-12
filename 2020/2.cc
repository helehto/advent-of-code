#include "common.h"

namespace aoc_2020_2 {

void run(std::string_view buf)
{
    int part1 = 0, part2 = 0, a = 0, b = 0;
    for (std::string_view line : split_lines(buf)) {
        auto r1 = std::from_chars(line.begin(), line.end(), a);
        auto r2 = std::from_chars(r1.ptr + 1, line.end(), b);
        char c = r2.ptr[1];
        std::string_view password(r2.ptr + 4, line.data() + line.size());

        uint16_t char_count[26]{};
        for (uint8_t ch : password)
            char_count[ch - 'a']++;
        part1 += char_count[c - 'a'] >= a && char_count[c - 'a'] <= b;
        part2 += (password[a - 1] == c) ^ (password[b - 1] == c);
    }
    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
