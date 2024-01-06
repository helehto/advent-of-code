#include "common.h"
#include "dense_set.h"
#include <algorithm>
#include <ranges>

namespace aoc_2021_11 {

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines, [](char c) { return c - '0'; });

    dense_set<Point<size_t>> flashed;
    std::vector<Point<size_t>> queue;
    int total_flashes = 0;
    for (int step = 1;; step++) {
        queue.clear();
        flashed.clear();

        for (auto &v : grid.all())
            v += 1;

        for (auto p : grid.ndindex()) {
            if (grid(p) > 9)
                queue.push_back(p);
        }

        for (size_t j = 0; j < queue.size(); j++) {
            const auto c = queue[j];
            if (auto [_, inserted] = flashed.insert(c); inserted) {
                for (auto n : neighbors8(grid, c)) {
                    grid(n)++;
                    if (grid(n) > 9)
                        queue.push_back(n);
                }
            }
        }

        for (auto p : grid.ndindex()) {
            if (grid(p) > 9) {
                total_flashes++;
                grid(p) = 0;
            }
        }

        if (step == 100) {
            fmt::print("{}\n", total_flashes);
        } else if (flashed.size() == grid.size()) {
            fmt::print("{}\n", step);
            return;
        }
    }
}

}
