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
    auto [buf, lines] = slurp_lines(f);
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
    std::vector<int> nums;
    nums.reserve(3);
    for (; i < lines.size(); i++) {
        find_numbers(lines[i], nums);
        move_crates(crates1[nums[1]], crates1[nums[2]], nums[0], true);
        move_crates(crates2[nums[1]], crates2[nums[2]], nums[0], false);
    }

    print_crates(crates1);
    print_crates(crates2);
}

}
