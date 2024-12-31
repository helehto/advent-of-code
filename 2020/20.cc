#include "common.h"
#include "dense_map.h"
#include "dense_set.h"
#include "small_vector.h"

// TODO: Clean up this unholy mess some day.

namespace aoc_2020_20 {

/// Pre-computed lookup table for reversing 10-bit integers.
constexpr auto bitrev10 = [] consteval {
    std::array<uint16_t, 1024> result;
    for (size_t x = 0; x < result.size(); ++x) {
        uint16_t b = 0;
        for (int i = 0; i < 10; ++i)
            b |= !!(x & (1 << i)) << (9 - i);
        result[x] = b;
    }
    return result;
}();

enum { U, R, D, L };

struct Tile {
    // Edge bitmasks in URDL order. The bits of horizontal edges are always
    // read out from left to right, and the bits of horizontal edges are always
    // read out from top to bottom.
    std::array<uint16_t, 4> edges;

    /// Flip this tile vertically.
    constexpr Tile vflip() const
    {
        return Tile{{edges[D], bitrev10[edges[R]], edges[U], bitrev10[edges[L]]}};
    }

    /// Rotate this tile clockwise.
    constexpr Tile rotate(int rot) const
    {
        std::array<uint16_t, 4> new_edges;

        // This gets a little tricky since we want to always have consistent
        // edge orientations (see the comment for `edges`), so some bitmasks
        // might need to be reversed to preserve this invariant.
        switch (rot & 3) {
        case 0:
            return *this;
            new_edges = edges;
            break;
        case 1:
            new_edges[U] = bitrev10[edges[L]];
            new_edges[R] = edges[U];
            new_edges[D] = bitrev10[edges[R]];
            new_edges[L] = edges[D];
            break;
        case 2:
            new_edges[U] = bitrev10[edges[D]];
            new_edges[R] = bitrev10[edges[L]];
            new_edges[D] = bitrev10[edges[U]];
            new_edges[L] = bitrev10[edges[R]];
            break;
        case 3:
            new_edges[U] = edges[R];
            new_edges[R] = bitrev10[edges[D]];
            new_edges[D] = edges[L];
            new_edges[L] = bitrev10[edges[U]];
            break;
        }

        return Tile(new_edges);
    }

    constexpr Tile transform(int trfm) const
    {
        Tile t = rotate(trfm & 3);
        return (trfm & 4) ? t.vflip() : t;
    }
};

struct CompatTileKey {
    int edge;
    int dir;

    constexpr bool operator==(const CompatTileKey &) const = default;
};

static void
complete_grid(const dense_map<int, Tile> &tiles,
              const dense_map<CompatTileKey, small_vector<std::pair<int, int>>, CrcHasher>
                  &compatible_tiles,
              Matrix<std::pair<int, int>> &tile_grid,
              std::span<const int> corner_tiles)
{
    auto get_neighbor = [&](const Tile &tile, int tile_id,
                            int dir) -> std::optional<std::pair<int, int>> {
        auto it = compatible_tiles.find(CompatTileKey(tile.edges[dir], dir));
        if (it == compatible_tiles.end())
            return {};

        auto v = it->second;
        erase_if(v, λa(a.first == tile_id));
        if (v.empty())
            return std::nullopt;

        ASSERT(v.size() == 1);
        return v.front();
    };

    std::ranges::fill(tile_grid.all(), std::pair(-1, -1));

    // Orient the first (top-left) corner piece correctly so that it has a
    // valid neighboring edge to the right and below it.
    for (size_t trfm = 0; trfm < 8; ++trfm) {
        const int corner_tile_id = corner_tiles[0];
        const Tile corner_tile = tiles.at(corner_tile_id).transform(trfm);

        tile_grid(0, 0) = {corner_tile_id, trfm};

        auto r = get_neighbor(corner_tile, corner_tile_id, R);
        auto d = get_neighbor(corner_tile, corner_tile_id, D);
        if (r && d) {
            tile_grid(0, 1) = *r;
            break;
        }
    }

    // Fill the rest of the grid, row by row.
    for (size_t i = 0; i < tile_grid.rows; ++i) {
        for (size_t j = 0; j + 1 < tile_grid.cols; ++j) {
            const Vec2i p(j, i);
            const Vec2i q = p + Vec2i{1, 0};
            const auto &[tile_id, trfm] = tile_grid(p);
            tile_grid(q) = *get_neighbor(tiles.at(tile_id).transform(trfm), tile_id, R);
        }

        // Fill the next entry in the first column of the next row.
        if (i + 1 < tile_grid.rows) {
            const Vec2i p(0, i);
            const Vec2i q = p + Vec2i{0, 1};
            const auto &[tile_id, trfm] = tile_grid(p);
            tile_grid(q) = *get_neighbor(tiles.at(tile_id).transform(trfm), tile_id, D);
        }
    }
}

template <typename T>
constexpr void transpose(Matrix<T> &m)
{
    ASSERT(m.rows == m.cols);
    const size_t n = m.rows;

    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
            std::swap(m(i, j), m(j, i));
}

template <typename T>
constexpr void hflip(Matrix<T> &m)
{
    for (size_t i = 0; i < m.rows; ++i)
        std::ranges::reverse(m.row(i));
}

template <typename T>
constexpr void vflip(Matrix<T> &m)
{
    for (size_t i = 0; i < m.cols; ++i)
        std::ranges::reverse(m.col(i));
}

template <typename T>
constexpr void rot90(Matrix<T> &m)
{
    transpose(m);
    hflip(m);
}

template <typename T>
constexpr void transform(Matrix<T> &m, int trfm)
{
    if ((trfm & 3) > 0)
        rot90(m);
    if ((trfm & 3) > 1)
        rot90(m);
    if ((trfm & 3) > 2)
        rot90(m);
    if (trfm & 4)
        vflip(m);
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    small_vector<int> nums;

    dense_map<int, Tile> tiles;
    dense_map<int, Matrix<char>> full_tiles;
    full_tiles.reserve(144);
    tiles.reserve(1 + lines.size() / 12);
    std::array<small_vector<int, 2>, 1024> edge_bitmask_to_tile;

    size_t i = 0;
    while (i < lines.size()) {
        auto [tile_id] = find_numbers_n<int, 1>(lines[i]);
        i++;

        uint16_t lmask = 0;
        uint16_t rmask = 0;
        uint16_t dmask = 0;
        uint16_t umask = 0;

        Matrix<char> full_tile(10, 10, '!');
        size_t ii = 0;

        ASSERT(lines[i].size() == 10);
        for (size_t j = 0; j < lines[i].size(); ++j) {
            umask = (umask << 1) | (lines[i][j] == '#');
            full_tile(0, j) = lines[ii][j];
        }

        for (; i < lines.size() && !lines[i].empty(); i++, ii++) {
            lmask = (lmask << 1) | (lines[i].front() == '#');
            rmask = (rmask << 1) | (lines[i].back() == '#');

            for (size_t k = 0; k < lines[i].size(); ++k)
                full_tile(ii, k) = lines[i][k];
        }

        for (size_t j = 0; j < lines[i - 1].size(); ++j) {
            dmask = (dmask << 1) | (lines[i - 1][j] == '#');
        }
        i++;

        for (auto mask : {lmask, rmask, dmask, umask}) {
            ASSERT((mask & ~1023) == 0);
            edge_bitmask_to_tile[std::min(mask, bitrev10[mask])].push_back(tile_id);
        }

        tiles[tile_id] = Tile{{umask, rmask, dmask, lmask}};
        full_tiles[tile_id] = full_tile;
    }

    // [edge, dir] -> [tile_id, trfm]
    dense_map<CompatTileKey, small_vector<std::pair<int, int>>, CrcHasher>
        compatible_tiles;
    compatible_tiles.reserve(tiles.size() * 32);
    for (auto &[tile_id, tile] : tiles) {
        for (int trfm = 0; trfm < 8; ++trfm) {
            const Tile transformed_tile = tile.transform(trfm);
            for (int dir = 0; dir < 4; ++dir) {
                CompatTileKey k(transformed_tile.edges[dir], dir ^ 0b10);
                compatible_tiles[k].emplace_back(tile_id, trfm);
            }
        }
    }

    small_vector<int> corner_tiles;
    for (const auto &[tile_id, tile] : tiles) {
        const size_t n_unique = std::ranges::count_if(
            tile.edges, λa(edge_bitmask_to_tile[std::min(a, bitrev10[a])].size() == 1));
        if (n_unique == 2)
            corner_tiles.push_back(tile_id);
    }
    fmt::print("{}\n", std::ranges::fold_left(corner_tiles, uint64_t(1), λab(a * b)));

    const auto B = static_cast<size_t>(std::round(std::sqrt(tiles.size())));
    ASSERT(B * B == tiles.size());
    Matrix<std::pair<int, int>> tile_grid(B, B);
    complete_grid(tiles, compatible_tiles, tile_grid, corner_tiles);

    // Merge all transformed tiles into a single grid, removing their border in
    // the process.
    Matrix<char> grid(8 * B, 8 * B);
    for (size_t i = 0; i < B; ++i) {
        for (size_t j = 0; j < B; ++j) {
            auto [tile_id, trfm] = tile_grid(i, j);
            Matrix<char> &full_tile = full_tiles.at(tile_id);
            transform(full_tile, trfm);
            for (size_t ii = 0; ii < 8; ++ii)
                for (size_t jj = 0; jj < 8; ++jj)
                    grid(8 * i + ii, 8 * j + jj) = full_tile(ii + 1, jj + 1);
        }
    }

    auto clear_monsters = [&] {
        static constexpr Vec2u8 pat[] = {
            Vec2u8(18, 0), Vec2u8(0, 1),  Vec2u8(5, 1),  Vec2u8(6, 1),  Vec2u8(11, 1),
            Vec2u8(12, 1), Vec2u8(17, 1), Vec2u8(18, 1), Vec2u8(19, 1), Vec2u8(1, 2),
            Vec2u8(4, 2),  Vec2u8(7, 2),  Vec2u8(10, 2), Vec2u8(13, 2), Vec2u8(16, 2),
        };

        bool ret = false;

        for (size_t i = 0; i + 2 < grid.rows; ++i) {
            for (size_t j = 0; j + 18 < grid.cols; ++j) {
                Vec2z p(j, i);

                const bool has_monster = std::ranges::all_of(pat, λa(grid(p + a) == '#'));
                ret |= has_monster;

                if (has_monster)
                    for (Vec2u8 offset : pat)
                        grid(p + offset) = '.';
            }
        }

        return ret;
    };

    clear_monsters();
    rot90(grid);
    clear_monsters();
    rot90(grid);
    clear_monsters();
    rot90(grid);
    clear_monsters();
    vflip(grid);
    clear_monsters();
    rot90(grid);
    clear_monsters();
    rot90(grid);
    clear_monsters();
    rot90(grid);
    clear_monsters();

    fmt::print("{}\n", std::ranges::count(grid.all(), '#'));
}

}
