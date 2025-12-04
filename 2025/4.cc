#include "common.h"

namespace aoc_2025_4 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines).padded(1, '.');

    Matrix<int8_t> num_neighbors(grid.rows, grid.cols);
    for (size_t i = 1; i < grid.rows - 1; ++i) {
        // GCC autovectorizes this. Color me impressed.
        for (size_t j = 1; j < grid.cols - 1; ++j) {
            int n = 0;
            n += (grid(i - 1, j - 1) == '@');
            n += (grid(i - 1, j + 0) == '@');
            n += (grid(i - 1, j + 1) == '@');
            n += (grid(i + 0, j - 1) == '@');
            n += (grid(i + 0, j + 1) == '@');
            n += (grid(i + 1, j - 1) == '@');
            n += (grid(i + 1, j + 0) == '@');
            n += (grid(i + 1, j + 1) == '@');
            num_neighbors(i, j) = n;
        }
    }

    // TODO: Use SIMD to pack cell positions into a fixed-size buffer instead.
    small_vector<Vec2z> queue;
    for (const auto p : grid.ndindex<size_t>())
        if (grid(p) == '@' && num_neighbors(p) < 4)
            queue.emplace_back(p.x, p.y);
    fmt::print("{}\n", queue.size());

    int part2 = 0;
    small_vector<Vec2z> pending;
    do {
        while (!queue.empty()) {
            auto p = queue.back();
            queue.pop_back();

            part2++;

            for (const auto q : neighbors8(p)) {
                const auto old_num_neighbors = num_neighbors(q)--;
                if (old_num_neighbors == 4 && grid(q) == '@')
                    pending.push_back(q);
            }
            grid(p) = 'x';
        }

        queue.swap(pending);
        pending.clear();
    } while (!queue.empty());

    fmt::print("{}\n", part2);
}

}
