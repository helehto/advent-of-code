#include "common.h"
#include <algorithm>

namespace aoc_2018_6 {

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    std::vector<int> nums;
    std::vector<int> xs;
    std::vector<int> ys;
    xs.reserve(lines.size());
    ys.reserve(lines.size());

    for (auto line : lines) {
        find_numbers(line, nums);
        xs.push_back(nums[0]);
        ys.push_back(nums[1]);
    }

    const auto max_x = std::ranges::max(xs);
    const auto max_y = std::ranges::max(ys);
    Matrix<uint8_t> closest_grid(max_x + 2, max_y + 2, xs.size());

    std::vector<int> cell_count(xs.size() + 1);
    std::vector<int> d(xs.size());

    for (auto p : closest_grid.ndindex<int>()) {
        for (size_t i = 0; i < xs.size(); i++)
            d[i] = manhattan({xs[i], ys[i]}, p);

        std::pair<int16_t, int16_t> min1 = {0, d[0]};
        std::pair<int16_t, int16_t> min2 = {1, d[1]};
        if (min1.second < min2.second)
            std::swap(min1, min2);
        for (size_t i = 2; i < xs.size(); i++) {
            if (d[i] < min1.second)
                min2 = std::exchange(min1, {i, d[i]});
            else if (d[i] < min2.second)
                min2 = {i, d[i]};
        }

        if (min1.second != min2.second) {
            closest_grid(p) = min1.first;
            cell_count[min1.first]++;
        }
    }

    // Anything around the rim is infinite, don't consider them for the maximum
    // by setting them to -1.
    for (int k : closest_grid.row(0))
        cell_count[k] = -1;
    for (int k : closest_grid.row(closest_grid.rows - 1))
        cell_count[k] = -1;
    for (int k : closest_grid.col(0))
        cell_count[k] = -1;
    for (int k : closest_grid.col(closest_grid.cols - 1))
        cell_count[k] = -1;
    fmt::print("{}\n", std::ranges::max(cell_count));

    int region_size = 0;
    for (int x = 0; x < max_x; x++) {
        for (int y = 0; y < max_y; y++) {
            int distance_sum = 0;
            for (size_t i = 0; i < xs.size(); i++)
                distance_sum += manhattan<int>({xs[i], ys[i]}, {x, y});
            if (distance_sum < 10000)
                region_size++;
        }
    }
    fmt::print("{}\n", region_size);
}

}
