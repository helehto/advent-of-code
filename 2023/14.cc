#include "common.h"
#include <algorithm>
#include <ranges>
#include <unordered_set>

namespace aoc_2023_14 {

static size_t scan_x(Matrix<char> &grid, Vec2z p, int dx)
{
    auto [i, j] = p;
    for (size_t k = j + dx;; k += dx) {
        if (grid(i, k) != '.')
            return k - dx;
        if (k + dx >= grid.cols)
            return k;
    }
}

static size_t scan_y(Matrix<char> &grid, Vec2z p, int dy)
{
    auto [i, j] = p;
    for (size_t k = i + dy;; k += dy) {
        if (grid(k, j) != '.')
            return k - dy;
        if (k + dy >= grid.rows)
            return k;
    }
}

static void roll_n(Matrix<char> &grid)
{
    for (auto [i, j] : grid.ndindex()) {
        if (i > 0 && grid(i, j) == 'O') {
            if (size_t k = scan_y(grid, {i, j}, -1); k != i)
                grid(k, j) = std::exchange(grid(i, j), '.');
        }
    }
}

static void roll_s(Matrix<char> &grid)
{
    for (size_t i = grid.rows - 1; i--;) {
        for (size_t j = 0; j < grid.cols; j++) {
            if (grid(i, j) == 'O') {
                if (size_t k = scan_y(grid, {i, j}, 1); k != i)
                    grid(k, j) = std::exchange(grid(i, j), '.');
            }
        }
    }
}

static void roll_w(Matrix<char> &grid)
{
    for (auto [i, j] : grid.ndindex()) {
        if (j > 0 && grid(i, j) == 'O') {
            if (size_t k = scan_x(grid, {i, j}, -1); k != j)
                grid(i, k) = std::exchange(grid(i, j), '.');
        }
    }
}

static void roll_e(Matrix<char> &grid)
{
    for (size_t j = grid.cols - 1; j--;) {
        for (size_t i = 0; i < grid.rows; i++) {
            if (grid(i, j) == 'O') {
                if (size_t k = scan_x(grid, {i, j}, 1); k != j)
                    grid(i, k) = std::exchange(grid(i, j), '.');
            }
        }
    }
}

struct MatrixHasher {
    size_t operator()(const Matrix<char> &m) const noexcept
    {
        return CrcHasher{}(std::string_view(m.data.get(), m.size()));
    }
};

static Matrix<char> find_cycle(Matrix<char> grid, int which)
{
    std::unordered_map<Matrix<char>, int, MatrixHasher> cache;
    std::vector<const Matrix<char> *> seq;

    for (int j = 0;; j++) {
        if (auto it = cache.find(grid); it != cache.end()) {
            auto mu = it->second;
            auto period = j - it->second;
            return *seq[mu + (which - mu) % period];
        }

        auto [it, _] = cache.emplace(grid, j);
        seq.push_back(&it->first);

        roll_n(grid);
        roll_w(grid);
        roll_s(grid);
        roll_e(grid);
    }
}

static int total_load(const Matrix<char> &grid)
{
    int sum = 0;
    for (auto p : grid.ndindex())
        if (grid(p) == 'O')
            sum += grid.rows - p.y;
    return sum;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);
    auto grid2 = grid;
    roll_n(grid);
    fmt::print("{}\n", total_load(grid));
    fmt::print("{}\n", total_load(find_cycle(grid2, 1000000000)));
}

}
