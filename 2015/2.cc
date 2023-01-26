#include "common.h"
#include <fmt/core.h>

void run_2015_2(FILE *f)
{
    std::vector<std::tuple<int, int, int>> input;

    int l,w,h;
    while (fscanf(f, "%dx%dx%d\n", &l, &w, &h) == 3)
        input.emplace_back(l,w,h);

    int part1 = 0;
    int part2 = 0;
    for (auto [l, w, h] : input) {
        part1 += 2 * l * w + 2 * w * h + 2 * h * l + std::min({l * w, w * h, h * l});
        std::array<int, 3> a = {{l, w, h}};
        std::sort(begin(a), end(a));
        part2 += 2 * a[0] + 2 * a[1] + l * w * h;
    }
    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}
