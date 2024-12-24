#include "common.h"

namespace aoc_2020_2 {

void run(FILE *f)
{
    char c, password[256];
    int part1 = 0, part2 = 0, a, b;
    while (fscanf(f, "%d-%d %c: %s", &a, &b, &c, password) == 4) {
        uint16_t char_count[26]{};
        for (size_t i = 0; password[i]; ++i)
            char_count[password[i] - 'a']++;
        part1 += char_count[c - 'a'] >= a && char_count[c - 'a'] <= b;
        part2 += (password[a - 1] == c) ^ (password[b - 1] == c);
    }
    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
