#include "common.h"
#include <fmt/core.h>

void run_2021_13(FILE *f)
{
    std::vector<std::pair<int, int>> pairs;
    std::string s;
    std::vector<int> v;
    while (getline(f, s) && !s.empty()) {
        find_numbers(s, v);
        pairs.emplace_back(v[0], v[1]);
    }

    int rows = 0;
    int cols = 0;
    for (auto [x, y] : pairs) {
        cols = std::max(cols, x);
        rows = std::max(rows, y);
    }

    Matrix<uint8_t> grid(cols + 1, rows + 1, 0);
    for (auto [x, y] : pairs)
        grid(x, y) = 1;

    while (getline(f, s)) {
        find_numbers(s, v);
        int w = v[0];

        if (s[sizeof("fold along")] == 'x') {
            int n = std::min(w, cols - w);

            for (int i = 1; i <= n; i++)
                for (int y = 0; y <= rows; y++) {
                    grid(w - i, y) |= grid(w + i, y);
                }

            cols = w - 1;
        } else {
            int n = std::min(w, rows - w);

            for (int x = 0; x <= cols; x++) {
                for (int i = 1; i <= n; i++)
                    grid(x, w - i) |= grid(x, w + i);
            }

            rows = w - 1;
        }
    }

    for (int y = 0; y <= rows; y++) {
        for (int x = 0; x <= cols; x++)
            fmt::print("{}", grid(x, y) ? '#' : '.');
        fmt::print("{}\n", "");
    }
}
