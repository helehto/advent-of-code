#include "common.h"

namespace aoc_2020_11 {

static size_t part1(Matrix<char> grid)
{
    auto next = grid;

    do {
        grid = next;
        for (auto p : grid.ndindex()) {
            if (next(p) != '.') {
                int occupied_neighbors = 0;
                for (auto q : neighbors8(next, p))
                    occupied_neighbors += (grid(q) == '#');

                if (grid(p) == 'L' && occupied_neighbors == 0)
                    next(p) = '#';
                else if (grid(p) == '#' && occupied_neighbors >= 4)
                    next(p) = 'L';
            }
        }
    } while (grid != next);
    return std::ranges::count(grid.all(), '#');
}

static bool scan_occupied(const Matrix<char> &g, Point<size_t> p, int dx, int dy)
{
    while (true) {
        p = p.translate(dx, dy);
        if (!g.in_bounds(p))
            return false;
        if (g(p) != '.')
            return g(p) == '#';
    }
}

static size_t part2(Matrix<char> grid)
{
    auto next = grid;

    do {
        grid = next;
        for (auto p : grid.ndindex()) {
            if (next(p) != '.') {
                int occupied_neighbors = 0;
                occupied_neighbors += scan_occupied(grid, p, -1, -1);
                occupied_neighbors += scan_occupied(grid, p, -1, +0);
                occupied_neighbors += scan_occupied(grid, p, -1, +1);
                occupied_neighbors += scan_occupied(grid, p, +0, -1);
                occupied_neighbors += scan_occupied(grid, p, +0, +1);
                occupied_neighbors += scan_occupied(grid, p, +1, -1);
                occupied_neighbors += scan_occupied(grid, p, +1, +0);
                occupied_neighbors += scan_occupied(grid, p, +1, +1);

                if (grid(p) == 'L' && occupied_neighbors == 0)
                    next(p) = '#';
                else if (grid(p) == '#' && occupied_neighbors >= 5)
                    next(p) = 'L';
            }
        }
    } while (grid != next);
    return std::ranges::count(grid.all(), '#');
}

void run(FILE *f)
{
    const auto [buf, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines);
    fmt::print("{}\n", part1(grid));
    fmt::print("{}\n", part2(grid));
}
}
