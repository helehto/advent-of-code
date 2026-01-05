#include "common.h"

namespace aoc_2023_14 {

enum { N, W, S, E };

constexpr void
roll(MatrixView<char> grid, std::span<const uint16_t> segments, const ssize_t stride)
{
    for (const uint16_t offset : segments) {
        auto *dst = grid.data() + offset;
        auto *src = grid.data() + offset;

        while (*src != '#') {
            if (*src == 'O') {
                *src = '.';
                *dst = 'O';
                dst += stride;
            }
            src += stride;
        }
    }
}

struct MatrixHasher {
    size_t operator()(MatrixView<const char> m) const noexcept
    {
        return CrcHasher{}(std::string_view(m.data(), m.size()));
    }
};

static Matrix<char>
find_cycle(Matrix<char> grid, int which, std::span<const small_vector<uint16_t>> segments)
{
    std::unordered_map<Matrix<char>, int, MatrixHasher> cache;
    std::vector<const Matrix<char> *> seq;

    for (int j = 0;; j++) {
        auto [it, inserted] = cache.try_emplace(grid, j);
        if (!inserted) {
            auto mu = it->second;
            auto period = j - it->second;
            return *seq[mu + (which - mu) % period];
        }

        seq.push_back(&it->first);

        roll(grid, segments[N], +grid.cols);
        roll(grid, segments[W], +1);
        roll(grid, segments[S], -grid.cols);
        roll(grid, segments[E], -1);
    }
}

constexpr size_t total_load(MatrixView<const char> grid)
{
    size_t total = 0;
    for (size_t i = 0; i < grid.rows; ++i)
        for (size_t j = 0; j < grid.cols; ++j)
            total += grid(i, j) == 'O' ? grid.rows - i - 1 : 0;
    return total;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines).padded(1, '#');
    ASSERT(grid.rows < 256);
    ASSERT(grid.cols < 256);

    small_vector<uint16_t> segments[4];
    for (size_t i = 0; i < grid.rows; ++i) {
        for (size_t j = 0; j < grid.cols; ++j) {
            if (grid(i, j) != '#')
                continue;

            if (i > 0 && grid(i - 1, j) != '#')
                segments[S].push_back(&grid(i - 1, j) - grid.data());

            if (i + 1 < grid.rows && grid(i + 1, j) != '#')
                segments[N].push_back(&grid(i + 1, j) - grid.data());

            if (j > 0 && grid(i, j - 1) != '#')
                segments[E].push_back(&grid(i, j - 1) - grid.data());

            if (j + 1 < grid.cols && grid(i, j + 1) != '#')
                segments[W].push_back(&grid(i, j + 1) - grid.data());
        }
    }

    // Part 1:
    {
        auto g = grid;
        roll(g, segments[N], g.cols);
        fmt::print("{}\n", total_load(g));
    }

    // Part 2:
    {
        auto g = find_cycle(std::move(grid), 1'000'000'000, segments);
        fmt::print("{}\n", total_load(g));
    }
}

}
