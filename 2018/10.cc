#include "common.h"
#include <algorithm>

namespace aoc_2018_10 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::vector<int64_t> px;
    std::vector<int64_t> py;
    std::vector<int64_t> vx;
    std::vector<int64_t> vy;
    const int n = lines.size();

    for (auto line : lines) {
        auto [a, b, c, d] = find_numbers_n<int64_t, 4>(line);
        px.push_back(a);
        py.push_back(b);
        vx.push_back(c);
        vy.push_back(d);
    }

    auto dispersion_x = [&](int64_t t) -> double {
        int64_t min = INT64_MAX;
        int64_t max = INT64_MIN;
        for (int i = 0; i < n; i++) {
            min = std::min(min, px[i] + t * vx[i]);
            max = std::max(max, px[i] + t * vx[i]);
        }
        return max - min;
    };

    // One iteration of Newton's method with x_0 = 0:
    int t0 = -2 * dispersion_x(0) / (dispersion_x(1) - dispersion_x(-1));
    int disp_min = dispersion_x(t0);
    for (int t = t0 - 20; t <= t0 + 20; t++) {
        if (dispersion_x(t) < disp_min) {
            t0 = t;
            disp_min = dispersion_x(t);
        }
    }

    for (int i = 0; i < n; i++) {
        px[i] += t0 * vx[i];
        py[i] += t0 * vy[i];
    }

    const auto [min_x, max_x] = std::ranges::minmax(px);
    const auto [min_y, max_y] = std::ranges::minmax(py);
    Matrix<char> grid(max_y - min_y + 1, max_x - min_x + 1, ' ');
    for (int i = 0; i < n; i++)
        grid(py[i] - min_y, px[i] - min_x) = '#';

    fmt::print("{}", grid);
    fmt::print("{}\n", t0);
}

}
