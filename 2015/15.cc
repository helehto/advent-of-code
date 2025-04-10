#include "common.h"

namespace aoc_2015_15 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<std::array<int64_t, 5>> ingredients;
    ingredients.reserve(lines.size());

    for (std::string_view line : lines)
        ingredients.push_back(find_numbers_n<int64_t, 5>(line));

    int64_t part1 = 0;
    int64_t part2 = 0;
    for (int a = 0; a <= 100; ++a) {
        for (int b = 0; a + b <= 100; ++b) {
            for (int c = 0; a + b + c <= 100; ++c) {
                int d = 100 - a - b - c;

                auto property_sum = [&](int p) {
                    auto result = a * ingredients[0][p] + b * ingredients[1][p] +
                                  c * ingredients[2][p] + d * ingredients[3][p];
                    return std::max<int64_t>(result, 0);
                };

                const int64_t capacity = property_sum(0);
                const int64_t durabiltity = property_sum(1);
                const int64_t flavor = property_sum(2);
                const int64_t texture = property_sum(3);
                const int64_t calories = property_sum(4);

                const int64_t score = capacity * durabiltity * flavor * texture;
                part1 = std::max(part1, score);
                part2 = std::max(part2, calories == 500 ? score : 0);
            }
        }
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
