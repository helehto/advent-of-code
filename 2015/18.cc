#include "common.h"

namespace aoc_2015_18 {

static int count_neighbors(const std::vector<std::string> &g, size_t x, size_t y)
{
    auto c = [&](int dx, int dy) {
        return x + dx < g[0].size() && y + dy < g.size() && g[y + dy][x + dx] == '#';
    };

    return c(-1, -1) + c(-1, 1) + c(1, 1) + c(1, -1) + c(0, -1) + c(0, 1) + c(-1, 0) +
           c(+1, 0);
}

static void step(std::vector<std::string> &new_grid, const std::vector<std::string> &grid)
{
    for (auto &s : new_grid)
        std::fill(begin(s), end(s), '.');

    for (size_t y = 0; y < grid.size(); y++) {
        for (size_t x = 0; x < grid[0].size(); x++) {
            const auto n = count_neighbors(grid, x, y);
            if (grid[y][x] == '#')
                new_grid[y][x] = n == 2 || n == 3 ? '#' : '.';
            else
                new_grid[y][x] = n == 3 ? '#' : '.';
        }
    }
}

static int count_on(const std::vector<std::string> &g)
{
    int count = 0;
    for (auto &s : g) {
        for (auto c : s)
            count += c == '#';
    }
    return count;
}

static int solve(std::vector<std::string> grid, int iterations, bool stuck)
{
    auto stuck_corners = [&] {
        if (stuck) {
            grid.front().front() = '#';
            grid.front().back() = '#';
            grid.back().front() = '#';
            grid.back().back() = '#';
        }
    };

    stuck_corners();
    std::vector<std::string> new_grid(grid.size(), std::string(grid[0].size(), '.'));
    for (int i = 0; i < iterations; i++) {
        step(new_grid, grid);
        std::swap(new_grid, grid);
        stuck_corners();
    }

    return count_on(grid);
}

void run(FILE *f)
{
    std::vector<std::string> grid;
    std::string s;

    while (getline(f, s))
        grid.push_back(std::move(s));

    constexpr int iterations = 100;
    fmt::print("{}\n", solve(grid, iterations, false));
    fmt::print("{}\n", solve(grid, iterations, true));
}

}
