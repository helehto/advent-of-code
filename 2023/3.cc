#include "common.h"
#include "dense_set.h"
#include <fmt/core.h>

static boost::container::static_vector<Point<size_t>, 8>
surrounding_part_numbers(const Matrix<char> &grid, Point<size_t> p)
{
    boost::container::static_vector<Point<size_t>, 8> result;

    for (auto n : neighbors8(grid, p)) {
        if (isdigit(grid(n))) {
            while (n.x > 0 && isdigit(grid(n.y, n.x - 1)))
                n.x--;

            if (std::find(result.begin(), result.end(), n) == result.end())
                result.push_back(n);
        }
    }

    return result;
}

static int read_number(const Matrix<char> &grid, Point<size_t> p)
{
    int n = 0;
    for (auto [x, y] = p; x < grid.cols && isdigit(grid(y, x)); x++)
        n = 10 * n + grid(y, x) - '0';
    return n;
}

void run_2023_3(FILE *f)
{
    const auto lines = getlines(f);
    Matrix<char> grid(lines.size(), lines[0].size());
    for (size_t y = 0; y < lines.size(); y++) {
        for (size_t x = 0; x < lines[0].size(); x++)
            grid(y, x) = lines[y][x];
    }

    dense_set<Point<size_t>> part_number_positions;
    for (auto p : grid.ndindex()) {
        if (!isdigit(grid(p)) && grid(p) != '.') {
            for (auto n : surrounding_part_numbers(grid, p))
                part_number_positions.insert(n);
        }
    }

    int sum = 0;
    for (auto p : part_number_positions)
        sum += read_number(grid, p);
    fmt::print("{}\n", sum);

    int ratio_sum = 0;
    for (auto p : grid.ndindex()) {
        if (grid(p) == '*') {
            if (auto n = surrounding_part_numbers(grid, p); n.size() == 2)
                ratio_sum += read_number(grid, n[0]) * read_number(grid, n[1]);
        }
    }
    fmt::print("{}\n", ratio_sum);
}
