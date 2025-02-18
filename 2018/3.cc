#include "common.h"
#include <algorithm>

namespace aoc_2018_3 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<std::tuple<int, int, int, int>> claims;
    size_t max_x = 0;
    size_t max_y = 0;
    {
        claims.reserve(lines.size());
        for (auto line : lines) {
            auto [claim, x, y, w, h] = find_numbers_n<int, 5>(line);
            claims.emplace_back(x, y, x + w, y + h);
            max_x = std::max<size_t>(max_x, x + w + 1);
            max_y = std::max<size_t>(max_y, y + h + +1);
        }
    }

    Matrix<uint16_t> grid(max_x, max_y);
    for (auto [x0, y0, x1, y1] : claims) {
        for (int y = y0; y < y1; y++) {
            for (int x = x0; x < x1; x++) {
                grid(y, x)++;
            }
        }
    }

    int n_disputed = 0;
    for (auto p : grid.ndindex())
        n_disputed += grid(p) > 1;
    fmt::print("{}\n", n_disputed);

    for (size_t i = 0; i < claims.size(); ++i) {
        bool overlap = false;

        for (size_t j = 0; j < claims.size() && !overlap; ++j) {
            if (i != j) {
                auto [ax0, ay0, ax1, ay1] = claims[i];
                auto [bx0, by0, bx1, by1] = claims[j];
                overlap = bx0 < ax1 && ax0 < bx1 && by0 < ay1 && ay0 < by1;
            }
        }

        if (!overlap) {
            fmt::print("{}\n", i + 1);
            return;
        }
    }
}

}
