#include "common.h"

namespace aoc_2017_19 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    size_t cols = 0;
    for (std::string_view line : lines) {
        while (!line.empty() && line.back() == ' ')
            line.remove_suffix(1);
        cols = std::max(cols, line.size());
    }

    Matrix<char> grid(lines.size(), cols, ' ');
    for (size_t i = 0; i < lines.size(); ++i)
        for (size_t j = 0; j < std::min(lines[i].size(), grid.cols); ++j)
            grid(i, j) = lines[i][j];

    const Vec2i start = [&grid] {
        for (size_t i = 0; i < grid.cols; ++i)
            if (grid(0, i) == '|')
                return Vec2i(i, 0);
        ASSERT_MSG(false, "No start position!?");
    }();

    std::string letters;
    letters.reserve(26);

    Vec2i p = start;
    Vec2i d{0, 1};
    int steps = 0;
    while (true) {
        const char c = grid(p);

        if (c >= 'A' && c <= 'Z') {
            letters.push_back(c);
        } else if (c == ' ') {
            fmt::print("{}\n{}\n", letters, steps);
            return;
        }
        steps++;

        if (c == '+') {
            auto q1 = p + d.cw();
            auto q2 = p + d.ccw();

            if (grid.in_bounds(q1) && grid(q1) != ' ') {
                p = q1;
                d = d.cw();
                continue;
            } else if (grid.in_bounds(q2) && grid(q2) != ' ') {
                p = q2;
                d = d.ccw();
                continue;
            } else {
                DEBUG_ASSERT_MSG(false, "Dead end intersection at {}!?", p);
            }
        }

        p += d;
    }
}

}
