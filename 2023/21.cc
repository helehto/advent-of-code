// #define DEBUG

#include "common.h"
#include "dense_map.h"
#include "dense_set.h"
#include <queue>
#include <ranges>

namespace aoc_2023_21 {

static dense_map<Point<int>, int>
walk(const Matrix<char> &grid, Point<int> start, int max_steps)
{
    dense_map<Point<int>, int> dist{{start, 0}};
    std::queue<std::pair<Point<int>, int>> queue;
    queue.emplace(start, 0);
    while (!queue.empty()) {
        auto [p, d] = queue.front();
        queue.pop();

        for (auto q : neighbors4<int>(p)) {
            Point<int> qwrap = {
                modulo<int>(q.x, grid.cols),
                modulo<int>(q.y, grid.rows),
            };
            if (d < max_steps && grid(qwrap) != '#' && dist.emplace(q, d + 1).second)
                queue.emplace(q, d + 1);
        }
    }

    return dist;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    Point<int> start;
    for (Point<size_t> p : grid.ndindex())
        if (grid(p) == 'S')
            start = p.cast<int>();

    ASSERT(grid.rows == grid.cols);
    const int64_t N = grid.rows;

    auto dist = walk(grid, start, N / 2 + 2 * N);

    int64_t part1 = 0;
    int64_t y0 = 0;
    int64_t s0 = N / 2 + 0 * N;
    int64_t y1 = 0;
    int64_t s1 = N / 2 + 1 * N;
    int64_t y2 = 0;
    int64_t s2 = N / 2 + 2 * N;
    for (int d : std::ranges::views::values(dist)) {
        part1 += (d % 2 == 0 && d <= 64);
        y0 += (d % 2 == (s0 & 1) && d <= s0);
        y1 += (d % 2 == (s1 & 1) && d <= s1);
        y2 += (d % 2 == (s2 & 1) && d <= s2);
    }

    int64_t steps = 26501365;
    ASSERT(26501365 % N == N / 2);

    // Lagrange interpolation for x = 0,1,2:
    auto P = [&](double x) -> double {
        return y0 * (x - 1) * (x - 2) / ((0 - 1) * (0 - 2)) +
               y1 * (x - 0) * (x - 2) / ((1 - 0) * (1 - 2)) +
               y2 * (x - 0) * (x - 1) / ((2 - 0) * (2 - 1));
    };
    fmt::print("{}\n", part1);
    fmt::print("{}\n", P((steps - N / 2) / N));
}

}
