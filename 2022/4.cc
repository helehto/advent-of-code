#include "common.h"

void run_2022_4(FILE *f)
{
    int a0, a1, b0, b1;
    int part1 = 0;
    int part2 = 0;

    while (fscanf(f, "%d-%d,%d-%d", &a0, &a1, &b0, &b1) == 4) {
        part1 += (a0 >= b0 && a1 <= b1) || (b0 >= a0 && b1 <= a1);
        part2 += a0 <= b1 && b0 <= a1;
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}
