#include "common.h"

namespace aoc_2021_5 {

void run(std::string_view buf)
{
    struct Pair {
        Vec2i a;
        Vec2i b;
    };

    int w = 0;
    int h = 0;

    std::vector<Pair> input;
    for (auto line : split_lines(buf)) {
        auto [x1, y1, x2, y2] = find_numbers_n<int, 4>(line);
        input.push_back({{x1, y1}, {x2, y2}});
        w = std::max(w, x2);
        h = std::max(h, y2);
    }

    Matrix<uint16_t> count1(w + 2, h + 2);
    Matrix<uint16_t> count2(w + 2, h + 2);

    for (auto &[a, b] : input) {
        auto [x1, y1] = a;
        auto [x2, y2] = b;

        if (x1 == x2 || y1 == y2) {
            int xmin = std::min(x1, x2);
            int xmax = std::max(x1, x2);
            int ymin = std::min(y1, y2);
            int ymax = std::max(y1, y2);

            for (int x = xmin; x <= xmax; x++) {
                for (int y = ymin; y <= ymax; y++) {
                    count1(x, y)++;
                    count2(x, y)++;
                }
            }
        } else {
            if (x1 > x2) {
                std::swap(x1, x2);
                std::swap(y1, y2);
            }

            if (y1 < y2) {
                for (int i = 0; i <= y2 - y1; i++)
                    count2(x1 + i, y1 + i)++;
            } else {
                for (int i = 0; i <= y1 - y2; i++)
                    count2(x1 + i, y1 - i)++;
            }
        }
    }

    int part1 = 0;
    for (auto n : count1.all())
        if (n >= 2)
            part1++;

    int part2 = 0;
    for (auto n : count2.all())
        if (n >= 2)
            part2++;

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
