#include "common.h"
#include "dense_set.h"
#include <algorithm>
#include <ranges>

namespace aoc_2021_11 {

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines, λx(x - '0'));

    dense_set<Vec2z> flashed;
    std::vector<Vec2z> queue;
    int total_flashes = 0;
    for (int step = 1;; step++) {
        queue.clear();
        flashed.clear();

        for (auto &v : grid.all())
            v += 1;

        for (size_t i = 0; i < grid.rows; ++i)
            for (size_t j = 0; j < grid.cols; ++j)
                if (grid(i, j) > 9)
                    queue.emplace_back(j, i);

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

        for (size_t i = 0; i < grid.rows; ++i) {
            for (size_t j = 0; j < grid.cols; ++j) {
                if (grid(i, j) > 9) {
                    total_flashes++;
                    grid(i, j) = 0;
                }
            }
        }

        if (step == 100) {
            answer.add(total_flashes);
        } else if (flashed.size() == grid.size()) {
            answer.add(step);
            return;
        }
    }
}
AOC_REGISTER_SOLVER(2021, 11, run);

}
