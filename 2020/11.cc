#include "common.h"

namespace aoc_2020_11 {

static size_t part1(Matrix<char> grid)
{
    auto next = grid;

    const __m256i vdot = _mm256_set1_epi8('.');
    const __m256i vl = _mm256_set1_epi8('L');
    const __m256i vhash = _mm256_set1_epi8('#');
    const __m256i vzero = _mm256_setzero_si256();

    do {
        grid = next;
        for (size_t i = 1; i < grid.rows - 1; i++) {
            size_t j = 1;

            for (; j + 31 < grid.cols - 1; j += 32) {
                const __m256i vgrid = _mm256_loadu_si256((__m256i *)&grid(i, j));
                const __m256i vnext = _mm256_loadu_si256((__m256i *)&next(i, j));
                const __m256i vskip = _mm256_cmpeq_epi8(vgrid, vdot);
                const __m256i vgrid_eql = _mm256_cmpeq_epi8(vgrid, vl);
                const __m256i vgrid_eqhash = _mm256_cmpeq_epi8(vgrid, vhash);

                // Load neighbor cells.
                const __m256i vneighbors[] = {
                    _mm256_loadu_si256((__m256i *)&grid(i - 1, j - 1)),
                    _mm256_loadu_si256((__m256i *)&grid(i - 1, j + 0)),
                    _mm256_loadu_si256((__m256i *)&grid(i - 1, j + 1)),
                    _mm256_loadu_si256((__m256i *)&grid(i + 0, j - 1)),
                    _mm256_loadu_si256((__m256i *)&grid(i + 0, j + 1)),
                    _mm256_loadu_si256((__m256i *)&grid(i + 1, j - 1)),
                    _mm256_loadu_si256((__m256i *)&grid(i + 1, j + 0)),
                    _mm256_loadu_si256((__m256i *)&grid(i + 1, j + 1)),
                };

                // Masks indicating which neighbors have '#'.
                const __m256i vhashes[] = {
                    _mm256_cmpeq_epi8(vneighbors[0], vhash),
                    _mm256_cmpeq_epi8(vneighbors[1], vhash),
                    _mm256_cmpeq_epi8(vneighbors[2], vhash),
                    _mm256_cmpeq_epi8(vneighbors[3], vhash),
                    _mm256_cmpeq_epi8(vneighbors[4], vhash),
                    _mm256_cmpeq_epi8(vneighbors[5], vhash),
                    _mm256_cmpeq_epi8(vneighbors[6], vhash),
                    _mm256_cmpeq_epi8(vneighbors[7], vhash),
                };

                // Number of occupied neighbors (actually negated here since we
                // are directly adding masks which are 0 or -1).
                const __m256i vn = _mm256_add_epi8(
                    _mm256_add_epi8(_mm256_add_epi8(vhashes[0], vhashes[1]),
                                    _mm256_add_epi8(vhashes[2], vhashes[3])),
                    _mm256_add_epi8(_mm256_add_epi8(vhashes[4], vhashes[5]),
                                    _mm256_add_epi8(vhashes[6], vhashes[7])));

                const __m256i vn_eq0 = _mm256_cmpeq_epi8(vn, vzero);
                const __m256i vn_gt3 = _mm256_cmpgt_epi8(_mm256_set1_epi8(-3), vn);

                const __m256i vnew_next_l = _mm256_blendv_epi8(
                    vnext, vhash,
                    _mm256_andnot_si256(vskip, _mm256_and_si256(vgrid_eql, vn_eq0)));

                const __m256i vnew_next_hash = _mm256_blendv_epi8(
                    vnew_next_l, vl,
                    _mm256_andnot_si256(vskip, _mm256_and_si256(vgrid_eqhash, vn_gt3)));

                _mm256_storeu_si256((__m256i *)&next(i, j), vnew_next_hash);
            }

            for (; j < grid.cols - 1; j++) {
                if (next(i, j) != '.') {
                    Vec2z p(j, i);
                    int occupied_neighbors = 0;
                    for (auto q : neighbors8(p))
                        occupied_neighbors += grid(q) == '#';

                    if (grid(p) == 'L' && occupied_neighbors == 0)
                        next(p) = '#';
                    else if (grid(p) == '#' && occupied_neighbors >= 4)
                        next(p) = 'L';
                }
            }
        }
    } while (grid != next);
    return std::ranges::count(grid.all(), '#');
}

static const char *scan_seat(const char *p, size_t stride)
{
    while (true) {
        p += stride;
        switch (*p) {
        case 'L':
        case '#':
            return p;
        case '.':
            break; // march on
        default:
            return nullptr; // hit padding; out of bounds
        }
    }
}

static size_t part2(Matrix<char> grid)
{
    // Pre-compute the visible seats in each direction for each position.
    // Visible seats are represented using a 16-bit relative offset from the
    // current position; the other slots in the array refer to the padding
    // area. (The main loop below only cares whether a neighbor is '#' or not,
    // so a padding value works fine to indicate "no occupied seat".)
    Matrix<std::array<int16_t, 8>> visible_seats(grid.rows, grid.cols);
    {
        for (auto &arr : visible_seats.all())
            arr.fill(INT16_MIN);
        const size_t strides[] = {
            -grid.cols - 1, -grid.cols + 0, -grid.cols + 1, -1zu,
            +1zu,           +grid.cols - 1, +grid.cols + 0, +grid.cols + 1,
        };
        for (size_t i = 1; i < grid.rows - 1; i++) {
            for (size_t j = 1; j < grid.cols - 1; j++) {
                if (grid(i, j) == '.')
                    continue;
                const char *p = &grid(i, j);
                for (size_t k = 0; k < 8; k++) {
                    ptrdiff_t offset = &grid(0, 0) - p;
                    if (const char *q = scan_seat(p, strides[k]))
                        offset = q - p;
                    ASSERT(offset > INT16_MIN && offset <= INT16_MAX);
                    visible_seats(i, j)[k] = offset;
                }
            }
        }
    }

    auto next = grid;

    do {
        grid = next;
        for (size_t i = 1; i < grid.rows - 1; i++) {
            for (size_t j = 1; j < grid.cols - 1; j++) {
                Vec2z p(j, i);
                if (next(p) != '.') {
                    const int16_t *offsets = visible_seats(p).data();
                    const char *k = &grid(p);
                    int occupied_neighbors =
                        (k[offsets[0]] == '#') + (k[offsets[1]] == '#') +
                        (k[offsets[2]] == '#') + (k[offsets[3]] == '#') +
                        (k[offsets[4]] == '#') + (k[offsets[5]] == '#') +
                        (k[offsets[6]] == '#') + (k[offsets[7]] == '#');

                    if (grid(p) == 'L' && occupied_neighbors == 0)
                        next(p) = '#';
                    else if (grid(p) == '#' && occupied_neighbors >= 5)
                        next(p) = 'L';
                }
            }
        }
    } while (grid != next);
    return std::ranges::count(grid.all(), '#');
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines).padded(1, '@');
    fmt::print("{}\n", part1(grid));
    fmt::print("{}\n", part2(grid));
}
}
