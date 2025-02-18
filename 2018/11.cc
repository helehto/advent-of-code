#include "common.h"
#include <algorithm>

namespace aoc_2018_11 {

static Matrix<int> generate_grid(int serial_number, size_t size)
{
    Matrix<int> grid(size, size);

    for (auto p : grid.ndindex<int>()) {
        int rack = p.x + 11;
        int power = (rack * (p.y + 1) + serial_number) * rack;
        grid(p) = (power / 100) % 10 - 5;
    }

    return grid;
}

template <std::ranges::input_range Range,
          std::output_iterator<std::ranges::range_value_t<Range>> Out>
static void prefix_sum(Range &&r, Out out)
{
    std::ranges::range_value_t<Range> sum{};
    for (auto x : r)
        *out++ = sum += x;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    const auto [serial_number] = find_numbers_n<int, 1>(lines[0]);
    const auto grid = generate_grid(serial_number, 300);

    Matrix<int> row_prefix_sum(grid.rows, grid.cols);
    for (size_t i = 0; i < grid.rows; i++)
        prefix_sum(grid.row(i), row_prefix_sum.row(i).begin());

    Matrix<int> col_prefix_sum(grid.rows, grid.cols);
    for (size_t j = 0; j < grid.cols; j++)
        prefix_sum(grid.col(j), col_prefix_sum.col(j).begin());

    auto search = [&](size_t min_n, size_t max_n) {
        std::tuple<int, int, int> result;
        int best_power = INT_MIN;
        for (auto [j, i] : grid.ndindex()) {
            const size_t limit = std::min({max_n + 1, grid.rows - i, grid.cols - j});

            int sum = grid(i, j);
            for (size_t n = 2; n < limit; n++) {
                const size_t ii = i + n - 1;
                const size_t jj = j + n - 1;
                sum += grid(ii, jj);

                sum += col_prefix_sum(ii - 1, jj);
                sum += row_prefix_sum(ii, jj - 1);

                if (ii >= n)
                    sum -= col_prefix_sum(ii - n, jj);
                if (jj >= n)
                    sum -= row_prefix_sum(ii, jj - n);

                if (n >= min_n && sum > best_power) {
                    best_power = sum;
                    result = {j + 1, i + 1, n};
                }
            }
        }

        return result;
    };

    auto [x1, y1, _] = search(3, 3);
    fmt::print("{},{}\n", x1, y1);
    auto [x2, y2, n2] = search(1, 300);
    fmt::print("{},{},{}\n", x2, y2, n2);
}

}
