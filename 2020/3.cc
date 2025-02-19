#include "common.h"

namespace aoc_2020_3 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    int slope_trees[5]{};
    constexpr Vec2z walks[] = {{1, 1}, {3, 1}, {5, 1}, {7, 1}, {1, 2}};
    for (size_t i = 0; const Vec2z d : walks) {
        Vec2z p{};
        while (true) {
            p += d;
            if (p.y >= grid.rows)
                break;
            if (p.x >= grid.cols)
                p.x -= grid.cols;
            slope_trees[i] += grid(p) == '#';
        }
        i++;
    }
    fmt::print("{}\n", slope_trees[1]);
    fmt::print("{}\n", static_cast<uint64_t>(slope_trees[0]) * slope_trees[1] *
                           slope_trees[2] * slope_trees[3] * slope_trees[4]);
}

}
