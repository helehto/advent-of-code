#include "common.h"
#include <algorithm>
#include <ranges>
#include <span>

using namespace std::ranges;
using namespace std::views;

static size_t count_col_mismatches(const Matrix<uint8_t> &m, size_t i, size_t j)
{
    size_t n = 0;

    for (size_t k = 0; k < m.cols; k++)
        if (m(i, k) != m(j, k))
            n++;

    return n;
}

static size_t count_row_mismatches(const Matrix<uint8_t> &m, size_t i, size_t j)
{
    size_t n = 0;

    for (size_t k = 0; k < m.rows; k++) {
        if (m(k, i) != m(k, j))
            n++;
    }

    return n;
}

struct ScanResult {
    size_t i;
    size_t j;
    size_t first_mismatch;
    size_t mismatches;
};

static ScanResult scanh(const Matrix<uint8_t> &m, size_t i, size_t j)
{
    for (size_t d = 0; i >= d && d + j < m.rows; d++) {
        for (size_t k = 0; k < m.cols; k++) {
            if (m(i - d, k) != m(j + d, k))
                return {i - d, j + d, k, count_col_mismatches(m, i - d, j + d)};
        }
    }

    return {0, 0, 0, 0};
}

static ScanResult scanv(const Matrix<uint8_t> &m, size_t i, size_t j)
{
    for (size_t d = 0; i >= d && d + j < m.cols; d++) {
        for (size_t k = 0; k < m.rows; k++) {
            if (m(k, i - d) != m(k, j + d))
                return {i - d, j + d, k, count_row_mismatches(m, i - d, j + d)};
        }
    }

    return {0, 0, 0, 0};
}

void run_2023_13(FILE *f)
{
    const auto lines = getlines(f);
    int part1 = 0;
    int part2 = 0;

    size_t curr = 0;
    do {
        size_t next = curr;
        while (next < lines.size() && !lines[next].empty())
            next++;

        Matrix<uint8_t> grid(next - curr, lines[curr].size());
        for (size_t i = curr; i < next; i++)
            for (size_t j = 0; j < lines[i].size(); j++)
                grid(i - curr, j) = lines[i][j];

        // look for vertical reflection lines
        for (size_t j = 0; j + 1 < grid.cols; j++) {
            if (auto sr = scanv(grid, j, j + 1); sr.mismatches == 0) {
                part1 += j + 1;
                goto done_part1;
            }
        }

        // look for horizontal reflection lines
        for (size_t j = 0; j + 1 < grid.rows; j++) {
            if (auto sr = scanh(grid, j, j + 1); sr.mismatches == 0) {
                part1 += 100 * (j + 1);
                goto done_part1;
            }
        }

done_part1:
        // look for vertical reflection lines 
        for (size_t j = 0; j + 1 < grid.cols; j++) {
            if (auto sr = scanv(grid, j, j + 1); sr.mismatches == 1) {
                if (sr.i == 0 || sr.j == grid.cols - 1 ||
                    scanv(grid, sr.i - 1, sr.j + 1).mismatches == 0) {
                    part2 += j + 1;
                    goto done_part2;
                }
            }
        }

        // look for horizontal reflection lines 
        for (size_t j = 0; j + 1 < grid.rows; j++) {
            if (auto sr = scanh(grid, j, j + 1); sr.mismatches == 1) {
                if (sr.i == 0 || sr.j == grid.rows - 1 ||
                    scanh(grid, sr.i - 1, sr.j + 1).mismatches == 0) {
                    part2 += 100*(j + 1);
                    goto done_part2;
                }
            }
        }

done_part2:
        curr = next + 1;
    } while (curr < lines.size());

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}
