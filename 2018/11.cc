#include "common.h"
#include <hwy/highway.h>

namespace aoc_2018_11 {

static Matrix<int> generate_grid(int serial_number, size_t size)
{
    Matrix<int> grid(size, size);

    for (auto p : grid.ndindex<int>()) {
        int rack = p.x + 11;
        int power = (rack * (p.y + 1) + serial_number) * rack;
        grid(p) = (power / 100) % 10 - 5;
    }

    return grid;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    const auto [serial_number] = find_numbers_n<int, 1>(lines[0]);
    auto grid = generate_grid(serial_number, 300);

    // The element at index (i,j) in rectangle_sum contains the cumulative sum
    // for all values in the grid at rows ≤ i and columns ≤ j. This allows for
    // computing power (i.e. sum) of an arbitrary rectangle in just a few ops,
    // and additionally allows for vectorizing the search for the best square
    // below.
    Matrix<int32_t> rectangle_sum(grid.rows + 7, grid.cols + 7, -1'000'000);
    rectangle_sum(0, 0) = grid(0, 0);
    for (size_t i = 1; i < grid.rows; i++)
        rectangle_sum(i, 0) = grid(i, 0) + rectangle_sum(i - 1, 0);
    for (size_t i = 1; i < grid.cols; i++)
        rectangle_sum(0, i) = grid(0, i) + rectangle_sum(0, i - 1);
    for (size_t i = 1; i < grid.rows; ++i) {
        for (size_t j = 1; j < grid.cols; ++j) {
            rectangle_sum(i, j) = grid(i, j) + rectangle_sum(i - 1, j) +
                                  rectangle_sum(i, j - 1) - rectangle_sum(i - 1, j - 1);
        }
    }

    auto search = [&](size_t min_n, size_t max_n) {
        using D = hn::ScalableTag<int32_t>;
        const D d;
        hn::Vec<D> vbestpower = hn::Set(d, INT32_MIN);

        // Values of (n,i,j) with the best power, packed into a 32-bit integer
        // to reduce the number of blends required below.
        hn::Vec<D> vbestnij = hn::Undefined(d);

        // FIXME: This only considers the interior of the grid...
        for (size_t n = min_n; n <= max_n; n++) {
            for (size_t i = n; i < grid.rows; ++i) {
                const int *elems = &rectangle_sum(i, n);
                const int *lsums = &rectangle_sum(i - n, n);
                const int *usums = &rectangle_sum(i, 0);
                const int *dsums = &rectangle_sum(i - n, 0);

                const hn::Vec<D> vni = hn::Set(d, n << 20 | (i - n + 2) << 10);

                size_t j = 0;
                for (; j + hn::Lanes(d) - 1 < grid.cols - n; j += hn::Lanes(d)) {
                    const hn::Vec<D> vlsums = hn::LoadU(d, lsums + j);
                    const hn::Vec<D> vusums = hn::LoadU(d, usums + j);
                    const hn::Vec<D> vdsums = hn::LoadU(d, dsums + j);
                    hn::Vec<D> power = hn::LoadU(d, elems + j) - vlsums - vusums + vdsums;
                    const hn::Mask<D> gt = hn::Gt(power, vbestpower);
                    vbestpower = hn::IfThenElse(gt, power, vbestpower);
                    vbestnij = hn::IfThenElse(gt, vni | hn::Iota(d, j + 2), vbestnij);
                }
            }
        }

        std::array<int32_t, hn::MaxLanes(d)> bestpower, bestnij;
        bestpower.fill(INT32_MIN);
        hn::StoreU(vbestpower, d, bestpower.data());
        hn::StoreU(vbestnij, d, bestnij.data());

        const size_t k = std::ranges::max_element(bestpower) - bestpower.begin();
        return std::tuple((bestnij[k]) & 0x3ff, (bestnij[k] >> 10) & 0x3ff,
                          (bestnij[k] >> 20) & 0x3ff);
    };

    auto [x1, y1, _] = search(3, 3);
    fmt::print("{},{}\n", x1, y1);
    auto [x2, y2, n2] = search(1, 300);
    fmt::print("{},{},{}\n", x2, y2, n2);
}

}
