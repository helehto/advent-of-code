#include "common.h"
#include "thread_pool.h"

namespace aoc_2025_9 {

template <typename T>
struct Tiles {
    std::vector<T> xs;
    std::vector<T> ys;

    int64_t rectangle_area(size_t i, size_t j) const
    {
        const int64_t w = std::abs(xs[i] - xs[j]) + 1;
        const int64_t h = std::abs(ys[i] - ys[j]) + 1;
        return w * h;
    }
};

static Tiles<int32_t> parse_input(std::string_view buf)
{
    auto lines = split_lines(buf);
    Tiles<int32_t> tiles;
    tiles.xs.resize(lines.size());
    tiles.ys.resize(lines.size());
    for (size_t i = 0; std::string_view line : lines) {
        std::tie(tiles.xs[i], tiles.ys[i]) = find_numbers_n<int32_t, 2>(line);
        i++;
    }
    return tiles;
}

static int64_t part1(const Tiles<int32_t> &tiles)
{
    int64_t max_area = INT64_MIN;

    for (size_t i = 0; i < tiles.xs.size(); ++i)
        for (size_t j = i + 1; j < tiles.xs.size(); ++j)
            max_area = std::max(max_area, tiles.rectangle_area(i, j));

    return max_area;
}

static std::vector<int32_t> sort_unique(std::span<const int32_t> r)
{
    std::vector<int32_t> result(r.begin(), r.end());
    std::ranges::sort(result);
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

static Tiles<uint16_t> compress_polygon(const Tiles<int32_t> &tiles)
{
    ASSERT(tiles.xs.size() < UINT16_MAX);

    std::vector<int32_t> xs_table = sort_unique(tiles.xs);
    std::vector<int32_t> ys_table = sort_unique(tiles.ys);

    Tiles<uint16_t> compressed;
    compressed.xs.resize(tiles.xs.size());
    compressed.ys.resize(tiles.ys.size());
    for (size_t i = 0; i < tiles.xs.size(); ++i) {
        compressed.xs[i] =
            std::ranges::lower_bound(xs_table, tiles.xs[i]) - xs_table.begin();
        compressed.ys[i] =
            std::ranges::lower_bound(ys_table, tiles.ys[i]) - ys_table.begin();
    }

    return compressed;
}

static void rasterize_polygon(MatrixView<char> grid, const Tiles<uint16_t> &tiles)
{
    Vec2u16 prev = {tiles.xs.back(), tiles.ys.back()};
    Vec2u16 curr;
    for (size_t i = 0; i < tiles.xs.size(); ++i, prev = curr) {
        curr = {tiles.xs[i], tiles.ys[i]};
        if (prev.x == curr.x) {
            for (int y = std::min(prev.y, curr.y); y <= std::max(prev.y, curr.y); ++y)
                grid(y, prev.x) = '#';
        } else {
            for (int x = std::min(prev.x, curr.x); x <= std::max(prev.x, curr.x); ++x)
                grid(prev.y, x) = '#';
        }
    }
}

static void flood_fill(const Tiles<uint16_t> &tiles, MatrixView<char> grid)
{
    // Instead of a traditional flood fill via BFS, we fill an entire line at a
    // time by tracking whether we are inside or outside the polygon. (At the
    // left-most column, we are always outside due to the padding.)
    //
    // This requires a bit of preprocessing: if traversing the grid from left
    // to right, we can either hit a vertical edge, a corner, or straddle a
    // horizontal edge. This results in two additional passes, but is faster
    // overall.

    const size_t rows = grid.rows;
    const size_t cols = grid.cols;

    enum { WALL_BELOW = 1 << 0, WALL_ABOVE = 1 << 1 };
    Matrix<uint8_t> info(rows, cols, 0);

    Vec2u16 prev = {tiles.xs.back(), tiles.ys.back()};
    Vec2u16 curr;
    for (size_t i = 0; i < tiles.xs.size(); ++i, prev = curr) {
        curr = {tiles.xs[i], tiles.ys[i]};
        if (prev.x == curr.x) {
            info(std::min(prev.y, curr.y), prev.x) |= WALL_BELOW;
            info(std::max(prev.y, curr.y), prev.x) |= WALL_ABOVE;
            const auto [y0, y1] = std::minmax(prev.y, curr.y);
            for (int y = y0 + 1; y <= y1 - 1; ++y)
                info(y, prev.x) |= WALL_ABOVE | WALL_BELOW;
        }
    }

    // Each cell state is the prefix XOR of the row so far.
    Matrix<uint8_t> prefix_xor(info.rows, info.cols, 0);
    for (size_t i = 1; i < info.rows; ++i)
        for (size_t j = 1; j < info.cols; ++j)
            prefix_xor(i, j) ^= prefix_xor(i, j - 1) ^ info(i, j - 1);

    // Fill all cells that are outside the polygon. Given the the preprocessing
    // done above, this becomes trivial.
    for (size_t i = 0; i < rows; ++i) {
        // Pull out the row data pointer once, otherwise GCC fails to vectorize
        // the inner loop. (Possibly aliasing issues since this is a char*?)
        char *row = grid.row(i).data();
        for (size_t j = 0; j < cols; ++j)
            row[j] = (info(i, j) | prefix_xor(i, j)) ? row[j] : '~';
    }
}

static int64_t part2(const Tiles<int32_t> &tiles)
{
    // The coordinates in the original input are too large to be practically
    // representable using a grid. Compress them to unique smaller values
    // first.
    Tiles<uint16_t> compressed = compress_polygon(tiles);

    // Construct a grid with the (compressed) polygon drawn on it.
    Matrix<char> grid(std::ranges::max(compressed.ys) + 1,
                      std::ranges::max(compressed.xs) + 1, ' ');
    rasterize_polygon(grid, compressed);

    // Pad the grid. Padding guarantees that all points at the borders of the
    // left edge are outside the polygon, simplifying the flood fill. It does
    // have the side effect of shifting all coordinates by 1, which we need to
    // fix up.
    grid = grid.padded(1, ' ');
    for (auto &x : compressed.xs)
        x += 1;
    for (auto &y : compressed.ys)
        y += 1;

    // Flood fill to mark all points outside the polygon.
    flood_fill(compressed, grid);

    // Construct a table where (i,j) contains the number of cells inside the
    // polygon for all rows ≤ i and columns ≤ j. This allows for checking
    // whether a rectangle is contained entirely inside the polygon in O(1)
    // time. (cf. <https://en.wikipedia.org/wiki/Summed-area_table>)
    Matrix<uint16_t> A(grid.rows, grid.cols, 0);
    for (size_t i = 1; i < grid.rows; ++i) {
        for (size_t j = 1; j < grid.cols; ++j) {
            int inside = (grid(i, j) != '~');
            A(i, j) = A(i - 1, j) + A(i, j - 1) - A(i - 1, j - 1) + inside;
        }
    }

    // Translates a "linear" index into an packed strict upper-triangular
    // matrix into (i,j) coordinates. E.g. for a 6x6 matrix:
    //
    //     x  0  1  2  3  4
    //     x  x  5  6  7  8
    //     x  x  x  9 10 11
    //     x  x  x  x 12 13
    //     x  x  x  x  x 14
    //
    // With a linear index, we can trivially split all pairs equally across
    // multiple threads, but we need to convert back to (i,j) coordinates for
    // the actual search.
    auto linear_to_pair = [&](size_t idx) {
        size_t n = tiles.xs.size();
        // Triangular number inversion, plus some painful algebra.
        size_t i = n - 2 -
                   static_cast<size_t>(std::floor(
                       std::sqrt(-8.0 * idx + 4.0 * n * (n - 1) - 7) / 2.0 - 0.5));
        size_t j = idx + i + 1 - n * (n - 1) / 2 + (n - i) * ((n - i) - 1) / 2;
        return std::make_pair(i, j);
    };

    // Compute the area of a valid rectangle, or INT64_MIN if the rectangle is
    // not fully contained inside the polygon.
    //
    // With the preprocessing above, checking if a rectangle is valid can be
    // done in constant time: if the area of the compressed rectangle equals
    // the number of cells inside the compressed polygon, it is valid. If not,
    // there is at least one cell outside of the compressed polygon, and hence
    // outside the original polygon as well.
    auto valid_rectangle_area = [&](size_t i, size_t j) -> int64_t {
        const auto compressed_area = compressed.rectangle_area(i, j);

        const auto [xi, xj] = std::minmax(compressed.xs[i], compressed.xs[j]);
        const auto [yi, yj] = std::minmax(compressed.ys[i], compressed.ys[j]);
        const auto cells_inside = A(yj, xj)            // bottom right
                                  - A(yi - 1, xj)      // top right
                                  - A(yj, xi - 1)      // bottom left
                                  + A(yi - 1, xi - 1); // top left

        return compressed_area == cells_inside ? tiles.rectangle_area(i, j) : INT64_MIN;
    };

    std::atomic_int64_t result = INT64_MIN;

    auto worker_slice = [&](size_t k0, size_t k1) {
        const auto [i0, j0] = linear_to_pair(k0);
        const auto [i1, j1] = k1 < tiles.xs.size() * (tiles.xs.size() - 1) / 2
                                  ? linear_to_pair(k1)
                                  : std::pair(tiles.xs.size() - 1, tiles.xs.size() - 1);
        int64_t local_max = INT64_MIN;

        // Partial initial row:
        for (size_t j = j0; j < tiles.xs.size(); ++j)
            local_max = std::max(local_max, valid_rectangle_area(i0, j));

        // Full rows:
        for (size_t i = i0 + 1; i < i1; ++i)
            for (size_t j = i + 1; j < tiles.xs.size(); ++j)
                local_max = std::max(local_max, valid_rectangle_area(i, j));

        // Partial final row:
        for (size_t j = i1 + 1; j <= j1; ++j)
            local_max = std::max(local_max, valid_rectangle_area(i1, j));

        atomic_store_max(result, local_max);
    };

    ThreadPool &pool = ThreadPool::get();
    pool.for_each_index(0, tiles.xs.size() * (tiles.xs.size() - 1) / 2,
                        std::move(worker_slice));
    return result.load();
}

void run(std::string_view buf)
{
    Tiles tiles = parse_input(buf);
    fmt::print("{}\n", part1(tiles));
    fmt::print("{}\n", part2(tiles));
}

}
