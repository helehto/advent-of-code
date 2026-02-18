#include "common.h"
#include <hwy/highway.h>

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

/// Compute the next state of many acres at a time using SIMD.
template <typename D>
static hn::Vec<D> step_acres_simd(D d, const uint8_t *p, const size_t cols)
{
    const hn::Vec<D> ke = hn::Set(d, EMPTY);
    const hn::Vec<D> kt = hn::Set(d, TREE);
    const hn::Vec<D> ky = hn::Set(d, YARD);

    // Load neighbors from above, the current row and below:
    const hn::Vec<D> u0 = hn::LoadU(d, p - cols - 1);
    const hn::Vec<D> u1 = hn::LoadU(d, p - cols);
    const hn::Vec<D> u2 = hn::LoadU(d, p - cols + 1);
    const hn::Vec<D> c0 = hn::LoadU(d, p - 1);
    const hn::Vec<D> c2 = hn::LoadU(d, p + 1);
    const hn::Vec<D> d0 = hn::LoadU(d, p + cols - 1);
    const hn::Vec<D> d1 = hn::LoadU(d, p + cols);
    const hn::Vec<D> d2 = hn::LoadU(d, p + cols + 1);

    // Count the number of neighboring trees and lumberyards. As trees are
    // represented as 0x01 and lumberyards as 0x10, the sum of all
    // neighboring acres will contain the total number of neighboring
    // lumberyards in the upper 4 bits and trees in the lower 4 bits.
    const hn::Vec<D> neighbors = u0 + u1 + u2 + c0 + c2 + d0 + d1 + d2;
    const hn::Vec<D> n_trees = neighbors & hn::Set(d, 0xf * TREE);
    const hn::Vec<D> n_yards = neighbors & hn::Set(d, 0xf * YARD);

    // The input acres and masks corresponding to their current states.
    const hn::Vec<D> input = hn::LoadU(d, p);
    const hn::Mask<D> emask = hn::Eq(input, ke);
    const hn::Mask<D> tmask = hn::Eq(input, kt);
    const hn::Mask<D> ymask = hn::Eq(input, ky);

    // Comparison masks for n(trees) >= {1,3} and n(yards) >= {1,3} among the
    // neighboring acres.
    const hn::Mask<D> t1 = hn::Ge(n_trees, hn::Set(d, 1 * TREE));
    const hn::Mask<D> y1 = hn::Ge(n_yards, hn::Set(d, 1 * YARD));
    const hn::Mask<D> t3 = hn::Ge(n_trees, hn::Set(d, 3 * TREE));
    const hn::Mask<D> y3 = hn::Ge(n_yards, hn::Set(d, 3 * YARD));

    // Next state for each acre, for each of the three possible current states.
    const hn::Vec<D> eresult = hn::IfThenElse(t3, kt, ke);
    const hn::Vec<D> tresult = hn::IfThenElse(y3, ky, kt);
    const hn::Vec<D> yresult = hn::IfThenElse(hn::And(t1, y1), ky, ke);

    // Combine the results from empty/tree/lumberyard acres.
    const hn::Vec<D> result = hn::IfThenElseZero(emask, eresult) |
                              hn::IfThenElseZero(tmask, tresult) |
                              hn::IfThenElseZero(ymask, yresult);
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
    uint8_t *q = &grid(grid.rows - 2, grid.cols - 1);
    uint8_t *out = &tmp(1, 1);

    // Process the entire input matrix in a single contiguous pass. (This
    // clobbers the border/padding acres which need to be fixed up below, but
    // avoids having to special case neighbor computations for those areas.)
    using D = hn::ScalableTag<uint8_t>;
    const ptrdiff_t lanes = hn::Lanes(D());
    for (; q - p >= lanes; p += lanes, out += lanes)
        hn::StoreU(step_acres_simd(D(), p, grid.cols), D(), out);
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
    auto padded = grid.padded(1, EMPTY);
    fmt::print("{}\n", part1(padded));
    fmt::print("{}\n", part2(std::move(padded)));
}

}
