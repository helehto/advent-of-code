#include "common.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2021_15 {

constexpr int solve(const Matrix<uint8_t> &grid)
{
    constexpr Vec2i start{0, 0};
    const Vec2i end(grid.cols - 1, grid.rows - 1);
    Matrix<uint16_t> dist(grid.rows, grid.cols, UINT16_MAX);

    MonotonicBucketQueue<Vec2i> q(10);
    q.emplace(0, start);
    dist(start) = 0;

    while (std::optional<Vec2i> u = q.pop()) {
        if (*u == end)
            return dist(end);

        for (const Vec2i v : neighbors4(grid, *u)) {
            if (const int alt = q.current_priority() + grid(v); alt < dist(v)) {
                q.emplace(alt, v);
                dist(v) = alt;
            }
        }
    }

    ASSERT_MSG(false, "Path not found!?");
}

constexpr Matrix<uint8_t> expand(const Matrix<uint8_t> &grid)
{
    Matrix<uint8_t> result(5 * grid.rows, 5 * grid.cols);

    for (size_t bi = 0; bi < 5; ++bi) {
        for (size_t bj = 0; bj < 5; ++bj) {
            for (size_t i = 0; i < grid.rows; ++i) {
                for (size_t j = 0; j < grid.cols; ++j) {
                    size_t ii = grid.rows * bi + i;
                    size_t jj = grid.cols * bj + j;
                    result(ii, jj) = ((grid(i, j) + bi + bj) - 1) % 9 + 1;
                }
            }
        }
    }

    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<uint8_t>::from_lines(lines, λx(x - '0'));

    fmt::print("{}\n", solve(grid));
    fmt::print("{}\n", solve(expand(grid)));
}

}
