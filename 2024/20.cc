#include "common.h"

namespace aoc_2024_20 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    auto grid = Matrix<char>::from_lines(lines);
    Vec2i start, end;
    for (auto p : grid.ndindex<int>()) {
        if (grid(p) == 'S')
            start = p;
        else if (grid(p) == 'E')
            end = p;
    }

    Matrix<int> dist(grid.rows, grid.cols, -1);
    std::vector<std::pair<int, Vec2i>> queue{{0, end}};
    for (size_t i = 0; i < queue.size(); ++i) {
        auto [d, u] = queue[i];

        dist(u) = d;

        for (auto v : neighbors4(grid, u))
            if (grid(v) != '#' && dist(v) < 0)
                queue.emplace_back(d + 1, v);
    }

    const int bound = grid.rows >= 20 ? 100 : 2;
    auto solve = [&](int n) {
        int cheats = 0;
        for (auto &[_, u] : queue) {
            for (int dy = -n; dy <= n; ++dy) {
                for (int dx = -n; dx <= n; ++dx) {
                    auto d = std::abs(dx) + std::abs(dy);
                    if (d <= n) {
                        auto v = u.translate(dx, dy);
                        if (grid.in_bounds(v) && grid(v) != '#')
                            cheats += (dist(u) - dist(v) - d >= bound);
                    }
                }
            }
        }

        return cheats;
    };

    fmt::print("{}\n", solve(2));
    fmt::print("{}\n", solve(20));
}

}
