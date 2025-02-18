#include "common.h"
#include <numeric>

namespace aoc_2015_6 {

void run(std::string_view buf)
{
    std::vector<int8_t> lights(1'000'000);
    std::vector<int16_t> brightness(1'000'000);

    for (std::string_view s : split_lines(buf)) {
        const auto [x0, y0, x1, y1] = find_numbers_n<int, 4>(s);

        if (s.starts_with("toggle")) {
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    lights[y * 1000 + x] = !lights[y * 1000 + x];
                    brightness[y * 1000 + x] += 2;
                }
            }

        } else if (s.starts_with("turn off")) {
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    lights[y * 1000 + x] = 0;
                    brightness[y * 1000 + x] = std::max(brightness[y * 1000 + x] - 1, 0);
                }
            }
        } else if (s.starts_with("turn on")) {
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    lights[y * 1000 + x] = 1;
                    brightness[y * 1000 + x]++;
                }
            }
        }
    }

    fmt::print("{}\n", std::count(begin(lights), end(lights), 1));
    fmt::print("{}\n", std::accumulate(begin(brightness), end(brightness), 0));
}

}
