#include "common.h"

namespace aoc_2018_18 {

enum {
    EMPTY = 0x00,
    TREE = 0x01,
    YARD = 0x10,
};

constexpr int score_of(const std::span<uint8_t> grid)
{
    return std::ranges::count(grid, TREE) * std::ranges::count(grid, YARD);
}

/// Compute the next state of 32 acres at a time.
static __m256i step_acre_avx2(const uint8_t *p, const size_t cols)
{
    const __m256i ke = _mm256_set1_epi8(EMPTY);
    const __m256i kt = _mm256_set1_epi8(TREE);
    const __m256i ky = _mm256_set1_epi8(YARD);

    // Load neighbors from above, the current row and below:
    const __m256i u0 = _mm256_loadu_si256((const __m256i *)(p - cols - 1));
    const __m256i u1 = _mm256_loadu_si256((const __m256i *)(p - cols));
    const __m256i u2 = _mm256_loadu_si256((const __m256i *)(p - cols + 1));
    const __m256i c0 = _mm256_loadu_si256((const __m256i *)(p - 1));
    const __m256i c2 = _mm256_loadu_si256((const __m256i *)(p + 1));
    const __m256i d0 = _mm256_loadu_si256((const __m256i *)(p + cols - 1));
    const __m256i d1 = _mm256_loadu_si256((const __m256i *)(p + cols));
    const __m256i d2 = _mm256_loadu_si256((const __m256i *)(p + cols + 1));

    // Count the number of neighboring trees and lumberyards. As trees are
    // represented as 0x01 and lumberyards as 0x10, the sum of all
    // neighboring acres will contain the total number of neighboring
    // lumberyards in the upper 4 bits and trees in the lower 4 bits.
    const __m256i neighbors = _mm256_add_epi8(
        _mm256_add_epi8(_mm256_add_epi8(u0, u1), _mm256_add_epi8(u2, c0)),
        _mm256_add_epi8(_mm256_add_epi8(c2, d0), _mm256_add_epi8(d1, d2)));
    const __m256i n_trees = _mm256_and_si256(neighbors, _mm256_set1_epi8(0xf * TREE));
    const __m256i n_yards = _mm256_and_si256(neighbors, _mm256_set1_epi8(0xf * YARD));

    // The input acres and masks corresponding to their current states.
    const __m256i input = _mm256_loadu_si256((const __m256i *)p);
    const __m256i emask = _mm256_cmpeq_epi8(input, ke);
    const __m256i tmask = _mm256_cmpeq_epi8(input, kt);
    const __m256i ymask = _mm256_cmpeq_epi8(input, ky);

    // Comparison masks for n(trees) >= {1,3} and n(yards) >= {1,3} among the
    // neighboring acres.
    const __m256i t1 = _mm256_cmpgt_epi8(n_trees, _mm256_setzero_si256());
    const __m256i y1 = _mm256_cmpgt_epi8(n_yards, _mm256_setzero_si256());
    const __m256i t3 = _mm256_cmpgt_epi8(n_trees, _mm256_set1_epi8(2 * TREE));
    const __m256i y3 = _mm256_cmpgt_epi8(n_yards, _mm256_set1_epi8(2 * YARD));

    // Next state for each acre, for each of the three possible current states.
    const __m256i eresult = _mm256_blendv_epi8(ke, kt, t3);
    const __m256i tresult = _mm256_blendv_epi8(kt, ky, y3);
    const __m256i yresult = _mm256_blendv_epi8(ke, ky, _mm256_and_si256(t1, y1));

    // Combine the results from empty/tree/lumberyard acres.
    __m256i result = _mm256_and_si256(emask, eresult);
    result = _mm256_or_si256(result, _mm256_and_si256(tmask, tresult));
    result = _mm256_or_si256(result, _mm256_and_si256(ymask, yresult));
    return result;
}

/// Compute the next state of a single acre.
constexpr uint8_t step_acre(const uint8_t *p, size_t cols)
{
    const int neighbors = *(p - cols - 1) + *(p - cols) + *(p - cols + 1) + *(p - 1) +
                          *(p + 1) + *(p + cols - 1) + *(p + cols) + *(p + cols + 1);
    const int n_trees = neighbors & (0xf * TREE);
    const int n_yards = neighbors & (0xf * YARD);

    if (*p == EMPTY)
        return n_trees >= (3 * TREE) ? TREE : EMPTY;
    else if (*p == TREE)
        return n_yards >= (3 * YARD) ? YARD : TREE;
    else
        return n_yards >= (1 * YARD) && n_trees >= (1 * TREE) ? YARD : EMPTY;
}

static void step(Matrix<uint8_t> &grid, Matrix<uint8_t> &tmp)
{
    uint8_t *p = &grid(1, 1);
    uint8_t *q = p + (grid.rows - 1) * (grid.cols - 1);
    uint8_t *out = &tmp(1, 1);

    // Process the entire input matrix in a single contiguous pass. (This
    // clobbers the border/padding acres which need to be fixed up below, but
    // avoids having to special case neighbor computations for those areas.)
    for (; q - p >= 32; p += 32, out += 32)
        _mm256_storeu_si256((__m256i *)out, step_acre_avx2(p, grid.cols));
    for (; p < q; ++p, ++out)
        *out = step_acre(p, grid.cols);

    // Make the clobbered border acres empty again.
    std::ranges::fill(tmp.row(0), EMPTY);
    std::ranges::fill(tmp.row(tmp.rows - 1), EMPTY);
    std::ranges::fill(tmp.col(0), EMPTY);
    std::ranges::fill(tmp.col(tmp.cols - 1), EMPTY);

    std::swap(grid, tmp);
}

static int part1(Matrix<uint8_t> grid)
{
    Matrix<uint8_t> tmp(grid.rows, grid.cols);
    for (size_t i = 0; i < 10; ++i)
        step(grid, tmp);

    return score_of(grid.all());
}

static int part2(Matrix<uint8_t> grid)
{
    Matrix<uint8_t> &xj = grid;
    Matrix<uint8_t> tmp(grid.rows, grid.cols);
    std::vector<std::tuple<Matrix<uint8_t>, int, int>> stack;

    int cycle_i, cycle_j;
    for (cycle_j = 0;; ++cycle_j) {
        const auto score_j = score_of(xj.all());
        while (!stack.empty()) {
            const auto &[xi, score_i, prev_i] = stack.back();
            if (score_i > score_j ||
                (score_i == score_j &&
                 std::ranges::lexicographical_compare(xj.all(), xi.all()))) {
                stack.pop_back();
                continue;
            }
            if (score_i == score_j && xj == xi) {
                cycle_i = prev_i;
                goto found_cycle;
            }
            break;
        }

        stack.emplace_back(xj, score_j, cycle_j);
        step(xj, tmp);
    }

found_cycle:
    const auto lambda = cycle_j - cycle_i;
    const auto mu = cycle_j;

    int extra_steps = (1'000'000'000 - mu) % lambda;
    for (int i = 0; i < extra_steps; ++i)
        step(xj, tmp);

    return score_of(xj.all());
}

static uint8_t parse_char(char c)
{
    if (c == '.')
        return EMPTY;
    else if (c == '|')
        return TREE;
    else if (c == '#')
        return YARD;
    else
        ASSERT(false);
}

void run(std::string_view buf)
{
    auto grid = Matrix<uint8_t>::from_lines(split_lines(buf), parse_char);
    Matrix<uint8_t> padded(grid.rows + 2, grid.cols + 2, EMPTY);

    for (size_t i = 0; i < grid.rows; ++i)
        for (size_t j = 0; j < grid.rows; ++j)
            padded(i + 1, j + 1) = grid(i, j);

    fmt::print("{}\n", part1(padded));
    fmt::print("{}\n", part2(std::move(padded)));
}

}
