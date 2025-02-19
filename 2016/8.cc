#include "common.h"

namespace aoc_2016_8 {

void run(std::string_view buf)
{
    Matrix<char> screen(6, 50, ' ');

    for (std::string_view line : split_lines(buf)) {
        if (line.starts_with("rect")) {
            auto [a, b] = find_numbers_n<int, 2>(line);
            for (int i = 0; i < b; ++i)
                for (int j = 0; j < a; ++j)
                    screen(i, j) = '#';
        } else if (line.starts_with("rotate column")) {
            auto [col, n] = find_numbers_n<size_t, 2>(line);
            auto range = screen.col(col);
            std::ranges::rotate(range, range.begin() + (screen.rows - n));
        } else {
            auto [row, n] = find_numbers_n<size_t, 2>(line);
            auto range = screen.row(row);
            std::ranges::rotate(range, range.begin() + (screen.cols - n));
        }
    }

    fmt::print("{}\n{}", std::ranges::count(screen.all(), '#'), screen);
}

}
