#include "common.h"

void run_2015_1(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);

    int p1 = 0;
    int p2 = -1;
    for (size_t i = 0; i < lines[0].size(); i++) {
        p1 = lines[0][i] == '(' ? p1 + 1 : p1 - 1;
        if (p1 < 0 && p2 < 0)
            p2 = i + 1;
    }
    fmt::print("{}\n", p1);
    fmt::print("{}\n", p2);
}
