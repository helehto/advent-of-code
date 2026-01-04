#include "common.h"

namespace aoc_2015_18 {

static int count_neighbors(const char *p, ssize_t stride)
{
    int count = 0;
    count += p[-stride - 1] == '#';
    count += p[-stride + 0] == '#';
    count += p[-stride + 1] == '#';
    count += p[+stride - 1] == '#';
    count += p[+stride + 0] == '#';
    count += p[+stride + 1] == '#';
    count += p[-1] == '#';
    count += p[+1] == '#';
    return count;
}

static void step(MatrixView<char> new_grid, MatrixView<const char> grid)
{
    for (size_t i = 1; i < grid.rows - 1; i++) {
        const char *src = &grid(i, 0);
        char *dst = &new_grid(i, 0);
        for (size_t j = 1; j < grid.cols - 1; j++) {
            const auto n = count_neighbors(&src[j], grid.cols);
            if (src[j] == '#')
                dst[j] = n == 2 || n == 3 ? '#' : '.';
            else
                dst[j] = n == 3 ? '#' : '.';
        }
    }
}

static int solve(Matrix<char> grid, int iterations, bool stuck)
{
    auto stuck_corners = [&] {
        if (stuck) {
            grid(1, 1) = '#';
            grid(1, grid.cols - 2) = '#';
            grid(grid.rows - 2, 1) = '#';
            grid(grid.rows - 2, grid.cols - 2) = '#';
        }
    };

    stuck_corners();
    Matrix<char> new_grid(grid.rows, grid.cols, '.');
    for (int i = 0; i < iterations; i++) {
        step(new_grid, grid);
        std::swap(new_grid, grid);
        stuck_corners();
    }

    return std::ranges::count(grid.all(), '#');
}

void run(std::string_view buf)
{
    Matrix<char> grid = Matrix<char>::from_lines(split_lines(buf)).padded(1, '.');

    constexpr int iterations = 100;
    fmt::print("{}\n", solve(grid, iterations, false));
    fmt::print("{}\n", solve(std::move(grid), iterations, true));
}

}
