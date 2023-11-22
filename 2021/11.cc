#include "common.h"
#include <algorithm>
#include <boost/container/static_vector.hpp>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <ranges>
#include <span>
#include "dense_set.h"
#include <vector>

template <typename T, size_t N>
using static_vector = boost::container::static_vector<T, N>;

static std::array<Point<size_t>, 8> neighbors8(Point<size_t> p)
{
    return {{
        {p.x - 1, p.y - 1},
        {p.x - 1, p.y},
        {p.x - 1, p.y + 1},
        {p.x, p.y - 1},
        {p.x, p.y + 1},
        {p.x + 1, p.y - 1},
        {p.x + 1, p.y},
        {p.x + 1, p.y + 1},
    }};
}

static static_vector<Point<size_t>, 8>
neighbors8(const Matrix<char> &grid, Point<size_t> p)
{
    static_vector<Point<size_t>, 8> result;
    for (auto n : neighbors8(p))
        if (n.x < grid.cols && n.y < grid.rows)
            result.push_back(n);

    return result;
}

int run_2021_11(FILE *f)
{
    auto lines = getlines(f);

    Matrix<char> grid(lines.size(), lines[0].size());
    for (size_t y = 0; y < grid.rows; ++y) {
        for (size_t x = 0; x < grid.cols; ++x)
            grid(y, x) = lines[y][x] - '0';
    }

    dense_set<Point<size_t>> flashed;
    std::vector<Point<size_t>> queue;
    int total_flashes = 0;
    for (int step = 1;; step++) {
        queue.clear();
        flashed.clear();

        for (auto &v : grid)
            v += 1;

        for (size_t y = 0; y < grid.rows; y++) {
            for (size_t x = 0; x < grid.cols; x++) {
                Point<size_t> p(x, y);
                if (grid(p) > 9)
                    queue.push_back(p);
            }
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

        for (size_t y = 0; y < grid.rows; y++) {
            for (size_t x = 0; x < grid.cols; x++) {
                Point<size_t> p(x, y);
                if (grid(p) > 9) {
                    total_flashes++;
                    grid(p) = 0;
                }
            }
        }

        if (step == 100) {
            fmt::print("{}\n", total_flashes);
        } else if (flashed.size() == grid.size()) {
            fmt::print("{}\n", step);
            return 0;
        }
    }
}
