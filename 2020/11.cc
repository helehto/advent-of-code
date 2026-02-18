#include "common.h"
#include <hwy/highway.h>

namespace aoc_2020_11 {

static size_t part1(Matrix<int8_t> grid)
{
    auto next = grid;
    const ssize_t cols = grid.cols;

    using D = hn::ScalableTag<decltype(grid)::value_type>;
    constexpr D d;
    const hn::Vec<D> vdot = hn::Set(d, '.');
    const hn::Vec<D> vl = hn::Set(d, 'L');
    const hn::Vec<D> vhash = hn::Set(d, '#');
    const hn::Vec<D> one = hn::Set(d, 1);

    do {
        grid = next;
        for (size_t i = 1; i < grid.rows - 1; i++) {
            size_t j = 1;
            const auto *p = &grid(i, j);

            for (; j + hn::Lanes(d) - 1 < grid.cols - 1; j += hn::Lanes(d), p += hn::Lanes(d)) {
                const hn::Vec<D> v = hn::LoadU(d, p);
                const hn::Vec<D> vnext = hn::LoadU(d, &next(i, j));
                const hn::Mask<D> eq_dot = hn::Eq(v, vdot);
                const hn::Mask<D> eq_l = hn::Eq(v, vl);
                const hn::Mask<D> eq_hash = hn::Eq(v, vhash);

                // Load neighbor cells.
                const hn::Vec<D> neighbors0 = hn::LoadU(d, p - cols - 1);
                const hn::Vec<D> neighbors1 = hn::LoadU(d, p - cols);
                const hn::Vec<D> neighbors2 = hn::LoadU(d, p - cols + 1);
                const hn::Vec<D> neighbors3 = hn::LoadU(d, p - 1);
                const hn::Vec<D> neighbors4 = hn::LoadU(d, p + 1);
                const hn::Vec<D> neighbors5 = hn::LoadU(d, p + cols - 1);
                const hn::Vec<D> neighbors6 = hn::LoadU(d, p + cols);
                const hn::Vec<D> neighbors7 = hn::LoadU(d, p + cols + 1);

                // Compute number of occupied neighbors.
                hn::Vec<D> n = hn::Zero(d);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors0, vhash), one, n);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors1, vhash), one, n);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors2, vhash), one, n);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors3, vhash), one, n);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors4, vhash), one, n);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors5, vhash), one, n);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors6, vhash), one, n);
                n = hn::MaskedAddOr(n, hn::Eq(neighbors7, vhash), one, n);

                const hn::Mask<D> vn_eq0 = hn::Eq(n, hn::Zero(d));
                const hn::Mask<D> vn_gt3 = hn::Gt(n, hn::Set(d, 3));

                const hn::Vec<D> vnew_next_l = hn::IfThenElse(
                    hn::AndNot(eq_dot, hn::And(eq_l, vn_eq0)), vhash, vnext);

                const hn::Vec<D> vnew_next_hash = hn::IfThenElse(
                    hn::AndNot(eq_dot, hn::And(eq_hash, vn_gt3)), vl, vnew_next_l);

                hn::StoreU(vnew_next_hash, d, &next(i, j));
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

static const int8_t *scan_seat(const int8_t *p, size_t stride)
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

static size_t part2(Matrix<int8_t> grid)
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
                const auto *p = &grid(i, j);
                for (size_t k = 0; k < 8; k++) {
                    ptrdiff_t offset = &grid(0, 0) - p;
                    if (const auto *q = scan_seat(p, strides[k]))
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
                    const int8_t *k = &grid(p);
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
    auto grid = Matrix<int8_t>::from_lines(lines).padded(1, '@');
    fmt::print("{}\n", part1(grid));
    fmt::print("{}\n", part2(grid));
}
}
