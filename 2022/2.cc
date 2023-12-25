#include "common.h"

namespace aoc_2022_2 {

void run(FILE *f)
{
    static constexpr int loses_to[] = {1, 2, 0};
    static constexpr int wins_against[] = {2, 0, 1};
    char ca, cb;
    int score1 = 0;
    int score2 = 0;

    while (fscanf(f, "%c %c\n", &ca, &cb) == 2) {
        const int a = ca >= 'X' ? ca - 'X' : ca - 'A';
        const int b = cb >= 'X' ? cb - 'X' : cb - 'A';

        // Part 1:
        if (b == loses_to[a])
            score1 += b + 7;
        else if (b == a)
            score1 += b + 4;
        else
            score1 += b + 1;

        // Part 2:
        if (b == 0)
            score2 += wins_against[a] + 1;
        else if (b == 1)
            score2 += a + 4;
        else
            score2 += loses_to[a] + 7;
    }

    fmt::print("{}\n", score1);
    fmt::print("{}\n", score2);
}

}
