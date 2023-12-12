#include "common.h"
#include <cstdio>
#include <fmt/core.h>
#include <string>
#include <vector>

void run_2022_8(FILE *f)
{
    std::vector<std::string> trees;
    std::string s;

    while (getline(f, s)) {
        if (!s.empty())
            trees.push_back(std::move(s));
    }
    const size_t n = trees.size();

    int visible = 0;
    for (size_t i = 1; i < n - 1; i++) {
        for (size_t j = 1; j < n - 1; j++) {
            const char t = trees[i][j];
            int vis = 4;

            for (size_t k = i; k--;) {
                if (t <= trees[k][j]) {
                    vis--;
                    break;
                }
            }

            for (size_t k = j; k--;) {
                if (t <= trees[i][k]) {
                    vis--;
                    break;
                }
            }

            for (size_t k = i + 1; k < n; k++) {
                if (t <= trees[k][j]) {
                    vis--;
                    break;
                }
            }

            for (size_t k = j + 1; k < n; k++) {
                if (t <= trees[i][k]) {
                    vis--;
                    break;
                }
            }

            visible += !!vis;
        }
    }
    fmt::print("{}\n", n * n - (n - 2) * (n - 2) + visible);

    int max_scenic_score = 0;
    for (size_t i = 1; i < n - 1; i++) {
        for (size_t j = 1; j < n - 1; j++) {
            const char t = trees[i][j];
            int u = i, d = n - i - 1, l = j, r = n - j - 1;

            for (size_t k = i; k--;) {
                if (t <= trees[k][j]) {
                    u = i - k;
                    break;
                }
            }

            for (size_t k = j; k--;) {
                if (t <= trees[i][k]) {
                    l = j - k;
                    break;
                }
            }

            for (size_t k = i + 1; k < n; k++) {
                if (t <= trees[k][j]) {
                    d = k - i;
                    break;
                }
            }

            for (size_t k = j + 1; k < n; k++) {
                if (t <= trees[i][k]) {
                    r = k - j;
                    break;
                }
            }

            max_scenic_score = std::max(max_scenic_score, u * d * l * r);
        }
    }
    fmt::print("{}\n", max_scenic_score);
}
