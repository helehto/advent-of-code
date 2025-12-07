#include "common.h"

namespace aoc_2025_7 {

void run(std::string_view buf)
{
    auto grid = Matrix<char>::from_lines(split_lines(buf));

    uint8_t start_x = UINT8_MAX;
    for (auto p : grid.ndindex()) {
        if (grid(p) == 'S') {
            start_x = p.x;
            break;
        }
    }
    ASSERT(start_x != UINT8_MAX);

    std::vector<uint8_t> beams{start_x};
    std::vector<uint8_t> new_beams;

    // Part 1:
    {
        int splits = 0;
        for (size_t i = 2; i < grid.rows; i += 2) {
            for (auto j : beams) {
                if (grid(i, j) == '^') {
                    splits++;
                    new_beams.push_back(j - 1);
                    new_beams.push_back(j + 1);
                } else {
                    new_beams.push_back(j);
                }
            }

            std::ranges::sort(new_beams);
            auto u = std::ranges::unique(new_beams);
            new_beams.erase(u.begin(), u.end());

            beams.swap(new_beams);
            new_beams.clear();
        }
        fmt::print("{}\n", splits);
    }

    // Part 2:
    {
        std::vector<uint64_t> timelines(grid.cols);
        timelines[start_x] = 1;

        for (size_t i = 2; i < grid.rows; i += 2) {
            for (size_t j = 1; j < grid.cols - 1; ++j) {
                if (grid(i, j) == '^') {
                    timelines[j - 1] += timelines[j];
                    timelines[j + 1] += timelines[j];
                    timelines[j] = 0;
                }
            }
        }

        auto total_timelines = std::ranges::fold_left(timelines, 0, λab(a + b));
        fmt::print("{}\n", total_timelines);
    }
}

}
