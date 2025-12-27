#include "common.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2021_15 {

constexpr uint8_t pad_value = UINT8_MAX;
constexpr size_t n_pad = 1;

constexpr int solve(MatrixView<const uint8_t> grid,
                    uint16_t *dist,
                    MonotonicBucketQueue<uint32_t, small_vector<uint32_t>> &q)
{
    const uint32_t start = &grid(n_pad, n_pad) - grid.data();
    const uint32_t end =
        &grid(grid.rows - n_pad - 1, grid.cols - n_pad - 1) - grid.data();

    MatrixView<uint16_t> dist_grid(dist, grid.rows, grid.cols);
    std::ranges::fill(dist_grid.all(), UINT16_MAX);

    // Set the minimum distance of all padding cells to 0 to ensure that they
    // are never visited. This saves a bounds check for every neighbor in the
    // main loop below.
    std::ranges::fill(dist_grid.col(0), 0);
    std::ranges::fill(dist_grid.col(grid.cols - 1), 0);
    std::ranges::fill(dist_grid.row(0), 0);
    std::ranges::fill(dist_grid.row(grid.rows - 1), 0);

    q.clear();
    q.emplace(0, start);
    dist[start] = 0;

    const ssize_t neighbor_strides[] = {
        1,                                // right
        static_cast<ssize_t>(grid.cols),  // down
        static_cast<ssize_t>(-1),         // left
        static_cast<ssize_t>(-grid.cols), // up
    };

    while (std::optional<const uint32_t> u = q.pop()) {
        if (*u == end)
            return dist[*u];

        if (dist[*u] != q.current_priority())
            continue;

        for (const size_t stride : neighbor_strides) {
            const size_t v = *u + stride;
            if (const int alt = q.current_priority() + grid.data()[v]; alt < dist[v]) {
                q.emplace(alt, v);
                dist[v] = alt;
            }
        }
    }

    ASSERT_MSG(false, "Path not found!?");
}

constexpr Matrix<uint8_t> expand(MatrixView<const uint8_t> grid)
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
    auto expanded_grid = expand(grid).padded(n_pad, pad_value);
    grid = grid.padded(n_pad, pad_value);

    auto dist = std::make_unique_for_overwrite<uint16_t[]>(5 * grid.rows * 5 * grid.cols);
    MonotonicBucketQueue<uint32_t, small_vector<uint32_t>> q(10);
    fmt::print("{}\n", solve(grid, dist.get(), q));
    fmt::print("{}\n", solve(expanded_grid, dist.get(), q));
}

}
