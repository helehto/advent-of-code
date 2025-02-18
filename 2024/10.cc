#include "common.h"

namespace aoc_2024_10 {

template <bool AllowRevisit>
static int64_t trailhead_score(const Matrix<char> &grid,
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

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    std::vector<Vec2i> queue;
    Matrix<bool> visited(grid.rows, grid.cols);

    int64_t s1 = 0;
    int64_t s2 = 0;
    for (auto u : grid.ndindex<int>()) {
        if (grid(u) == '0') {
            s1 += trailhead_score<false>(grid, u, queue, visited);
            s2 += trailhead_score<true>(grid, u, queue, visited);
        }
    }

    fmt::print("{}\n", s1);
    fmt::print("{}\n", s2);
}

}
