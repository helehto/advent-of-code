#include "common.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace aoc_2022_5 {

static void move_crates(std::string &src, std::string &dst, int n, bool reverse)
{
    dst.insert(dst.begin(), src.begin(), src.begin() + n);
    if (reverse)
        std::reverse(dst.begin(), dst.begin() + n);
    src.erase(src.begin(), src.begin() + n);
}

static void print_crates(std::span<std::string> crates)
{
    for (auto &s : crates) {
        if (!s.empty())
            putc(s.front(), stdout);
    }
    putc('\n', stdout);
}

void run(FILE *f)
{
    std::string s;
    std::array<std::string, 30> crates1;
    while (getline(f, s)) {
        if (s.empty() || isdigit(s[1]))
            break;
        for (size_t i = 1; i < s.size(); i += 4) {
            if (s[i] != ' ')
                crates1[(i + 3) / 4] += s[i];
        }
    }

    auto crates2 = crates1;
    while (getline(f, s)) {
        int n, f, t;
        if (sscanf(s.c_str(), "move %d from %d to %d", &n, &f, &t) != 3)
            continue;

        move_crates(crates1[f], crates1[t], n, true);
        move_crates(crates2[f], crates2[t], n, false);
    }

    print_crates(crates1);
    print_crates(crates2);
}

}
