#include <algorithm>
#include <aoc/base.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aoc_2021_13 {

void run(std::string_view buf, aoc::Answer &answer)
{
    std::vector<std::pair<int, int>> pairs;
    auto lines = split_lines(buf);
    size_t i = 0;
    for (; !lines[i].empty(); i++) {
        auto [x, y] = find_numbers_n<int, 2>(lines[i]);
        pairs.emplace_back(x, y);
    }
    i++;

    int rows = 0;
    int cols = 0;
    for (auto [x, y] : pairs) {
        cols = std::max(cols, x);
        rows = std::max(rows, y);
    }

    Matrix<uint8_t> grid(cols + 1, rows + 1, 0);
    for (auto [x, y] : pairs)
        grid(x, y) = 1;

    for (; i < lines.size(); i++) {
        std::string_view s = lines[i];
        auto [w] = find_numbers_n<int, 1>(lines[i]);

        if (s[sizeof("fold along")] == 'x') {
            int n = std::min(w, cols - w);

            for (int i = 1; i <= n; i++)
                for (int y = 0; y <= rows; y++) {
                    grid(w - i, y) |= grid(w + i, y);
                }

            cols = w - 1;
        } else {
            int n = std::min(w, rows - w);

            for (int x = 0; x <= cols; x++) {
                for (int i = 1; i <= n; i++)
                    grid(x, w - i) |= grid(x, w + i);
            }

            rows = w - 1;
        }
    }

    std::string output;
    for (int y = 0; y <= rows; y++) {
        if (y)
            output.push_back('\n');
        for (int x = 0; x <= cols; x++)
            output.push_back(grid(x, y) ? '#' : '.');
    }
    answer.add(output);
}
AOC_REGISTER_SOLVER(2021, 13, run);

}
