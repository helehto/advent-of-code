#include "common.h"
#include <algorithm>

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
        __m256i vbestpower = _mm256_set1_epi32(INT32_MIN);

        // Values of (n,i,j) with the best power, packed into a 32-bit integer
        // to reduce the number of blends required below.
        __m256 vbestnij = _mm256_undefined_ps();

        // FIXME: This only considers the interior of the grid...
        for (size_t n = min_n; n <= max_n; n++) {
            for (size_t i = n; i < grid.rows; ++i) {
                const int *elems = &rectangle_sum(i, n);
                const int *lsums = &rectangle_sum(i - n, n);
                const int *usums = &rectangle_sum(i, 0);
                const int *dsums = &rectangle_sum(i - n, 0);

                const __m256i vni = _mm256_set1_epi32(n << 20 | (i - n + 2) << 10);

                size_t j = 0;
                for (; j + 7 < grid.cols - n; j += 8) {
                    const __m256i vlsums = _mm256_loadu_si256((const __m256i *)&lsums[j]);
                    const __m256i vusums = _mm256_loadu_si256((const __m256i *)&usums[j]);
                    const __m256i vdsums = _mm256_loadu_si256((const __m256i *)&dsums[j]);
                    __m256i power = _mm256_loadu_si256((const __m256i *)&elems[j]);
                    power = _mm256_sub_epi32(power, vlsums);
                    power = _mm256_sub_epi32(power, vusums);
                    power = _mm256_add_epi32(power, vdsums);

                    const __m256i gt = _mm256_cmpgt_epi32(power, vbestpower);

                    // The tons of ugly casts here are required since AVX2
                    // doesn't have _mm256_blendv_epi32, but at least it all
                    // compiles down to a single blend instruction.
                    vbestpower = _mm256_castps_si256(_mm256_blendv_ps(
                        _mm256_castsi256_ps(vbestpower), _mm256_castsi256_ps(power),
                        _mm256_castsi256_ps(gt)));

                    const __m256i vj = _mm256_setr_epi32(j + 2, j + 3, j + 4, j + 5,
                                                         j + 6, j + 7, j + 8, j + 9);
                    const __m256 vnij = _mm256_castsi256_ps(_mm256_or_si256(vni, vj));
                    vbestnij = _mm256_blendv_ps(vbestnij, vnij, _mm256_castsi256_ps(gt));
                }
            }
        }

        alignas(32) std::array<int, 8> bestpower, bestnij;
        _mm256_store_si256((__m256i *)&bestpower, vbestpower);
        _mm256_store_si256((__m256i *)&bestnij, _mm256_castps_si256(vbestnij));

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
