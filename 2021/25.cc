#include "common.h"

namespace aoc_2021_25 {

constexpr int increment_wrap(size_t value, size_t m)
{
    return value + 1 < m ? value + 1 : 0;
}

void run(std::string_view buf)
{
    auto grid = Matrix<char>::from_lines(split_lines(buf));

    std::vector<Vec2i> r, d;
    for (const Vec2i p : grid.ndindex<int>()) {
        if (grid(p) == '>')
            r.push_back(p);
        if (grid(p) == 'v')
            d.push_back(p);
    }

    std::vector<uint32_t> pending;
    pending.reserve(std::max(d.size(), r.size()));
    for (int step = 1;; ++step) {
        bool change = false;

        for (size_t i = 0; i < r.size(); ++i) {
            const Vec2i p = r[i];
            const Vec2i q = {increment_wrap(p.x, grid.cols), p.y};
            if (grid(q) == '.')
                pending.push_back(i);
        }
        if (!pending.empty()) {
            change = true;
            for (const size_t i : pending) {
                grid(r[i]) = '.';
                r[i].x = increment_wrap(r[i].x, grid.cols);
                grid(r[i]) = '>';
            }
            pending.clear();
        }

        for (size_t i = 0; i < d.size(); ++i) {
            const Vec2i p = d[i];
            const Vec2i q = {p.x, increment_wrap(p.y, grid.rows)};
            if (grid(q) == '.')
                pending.push_back(i);
        }
        if (!pending.empty()) {
            change = true;
            for (const size_t i : pending) {
                grid(d[i]) = '.';
                d[i].y = increment_wrap(d[i].y, grid.rows);
                grid(d[i]) = 'v';
            }
            pending.clear();
        }

        if (!change) {
            fmt::print("{}\n", step);
            break;
        }
    }
}

}
