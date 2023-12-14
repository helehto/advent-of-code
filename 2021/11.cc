#include "common.h"
#include "dense_set.h"
#include <algorithm>
#include <ranges>

void run_2021_11(FILE *f)
{
    auto lines = getlines(f);

    Matrix<char> grid(lines.size(), lines[0].size());
    for (auto p : grid.ndindex())
        grid(p) = lines[p.y][p.x] - '0';

    dense_set<Point<size_t>> flashed;
    std::vector<Point<size_t>> queue;
    int total_flashes = 0;
    for (int step = 1;; step++) {
        queue.clear();
        flashed.clear();

        for (auto &v : grid)
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
