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

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    size_t i = 0;

    std::array<std::string, 30> crates1;
    for (i = 0; i < lines.size(); i++) {
        std::string_view s = lines[i];
        if (s.empty() || isdigit(s[1]))
            break;
        for (size_t i = 1; i < s.size(); i += 4) {
            if (s[i] != ' ')
                crates1[(i + 3) / 4] += s[i];
        }
    }
    i += 2;

    auto crates2 = crates1;
    for (; i < lines.size(); i++) {
        auto [crate, from, to] = find_numbers_n<int, 3>(lines[i]);
        move_crates(crates1[from], crates1[to], crate, true);
        move_crates(crates2[from], crates2[to], crate, false);
    }

    print_crates(crates1);
    print_crates(crates2);
}

}
