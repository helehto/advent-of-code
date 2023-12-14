#include "common.h"

void run_2015_1(FILE *f)
{
    std::string s;
    getline(f, s);

    int p1 = 0;
    int p2 = -1;
    for (size_t i = 0; i < s.size(); i++) {
        p1 = s[i] == '(' ? p1 + 1 : p1 - 1;
        if (p1 < 0 && p2 < 0)
            p2 = i + 1;
    }
    fmt::print("{}\n", p1);
    fmt::print("{}\n", p2);
}
