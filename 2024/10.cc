#include <algorithm>
#include <aoc/base.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace aoc_2024_10 {

template <bool AllowRevisit>
static int64_t trailhead_score(MatrixView<const char> grid,
                               Vec2i p,
                               std::vector<Vec2i> &queue,
                               Matrix<bool> &visited)
{
    if constexpr (!AllowRevisit)
        std::ranges::fill(visited.all(), false);

    queue.clear();
    queue.push_back(p);

    int64_t score = 0;

    for (size_t i = 0; i < queue.size(); i++) {
        auto u = queue[i];

        if constexpr (!AllowRevisit) {
            if (std::exchange(visited(u), true))
                continue;
        }

        score += grid(u) == '9';

        for (auto v : neighbors4(grid, u))
            if (grid(v) - grid(u) == 1)
                queue.push_back(v);
    }

    return score;
}

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    std::vector<Vec2i> queue;
    Matrix<bool> visited(grid.rows, grid.cols);

    int64_t s1 = 0;
    int64_t s2 = 0;
    for (size_t i = 0; i < grid.rows; ++i) {
        for (size_t j = 0; j < grid.cols; ++j) {
            if (const Vec2i u(j, i); grid(u) == '0') {
                s1 += trailhead_score<false>(grid, u, queue, visited);
                s2 += trailhead_score<true>(grid, u, queue, visited);
            }
        }
    }

    answer.add(s1);
    answer.add(s2);
}
AOC_REGISTER_SOLVER(2024, 10, run);

}
