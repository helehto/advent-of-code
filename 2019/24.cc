#include "common.h"
#include "dense_set.h"
#include <deque>

namespace aoc_2019_24 {

constexpr static uint32_t cell(size_t j)
{
    return static_cast<uint32_t>(1) << j;
}

constexpr static uint32_t cell_index(size_t i, size_t j)
{
    return 5 * i + j;
}

constexpr static uint32_t cell_xy(size_t i, size_t j)
{
    return cell(cell_index(i, j));
}

constexpr uint32_t row_mask = 0b00000'00000'00000'00000'11111;
constexpr uint32_t col_mask = 0b00001'00001'00001'00001'00001;

/// Pre-computed neighbor mask for all 25 cells.
constexpr std::array<uint32_t, 25> local_neighbor_mask = [] {
    std::array<uint32_t, 25> result{};

    size_t idx = 0;
    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 5; j++, idx++) {
            if (i > 0)
                result[idx] |= cell_xy(i - 1, j);
            if (i < 4)
                result[idx] |= cell_xy(i + 1, j);
            if (j > 0)
                result[idx] |= cell_xy(i, j - 1);
            if (j < 4)
                result[idx] |= cell_xy(i, j + 1);
        }
    }

    return result;
}();

constexpr static uint32_t step_single(uint32_t grid)
{
    uint32_t result = grid;
    size_t idx = 0;
    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 5; j++, idx++) {
            int n = std::popcount(grid & local_neighbor_mask[idx]);
            const bool has_bug = (grid & (1 << idx)) != 0;

            if (has_bug && n != 1)
                result &= ~(1 << idx);
            if (!has_bug && (n == 1 || n == 2))
                result |= (1 << idx);
        }
    }

    return result;
}

constexpr std::array<uint32_t, 25> inner_neighbor_mask = [] {
    std::array<uint32_t, 25> result{};
    result[cell_index(1, 2)] = row_mask;
    result[cell_index(2, 1)] = col_mask;
    result[cell_index(2, 3)] = col_mask << 4;
    result[cell_index(3, 2)] = row_mask << 20;
    return result;
}();

constexpr std::array<uint32_t, 25> outer_neighbor_mask = [] {
    std::array<uint32_t, 25> result{};
    for (size_t i = 0; i < 5; ++i) {
        result[cell_index(i, 0)] |= cell_xy(2, 1);
        result[cell_index(i, 4)] |= cell_xy(2, 3);
    }
    for (size_t j = 0; j < 5; ++j) {
        result[cell_index(0, j)] |= cell_xy(1, 2);
        result[cell_index(4, j)] |= cell_xy(3, 2);
    }
    return result;
}();

constexpr static uint32_t step_recursive(uint32_t inner, uint32_t grid, uint32_t outer)
{
    uint32_t result = grid;

    size_t idx = 0;
    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 5; j++, idx++) {
            uint32_t mask = local_neighbor_mask[idx] & ~cell_xy(2, 2);
            int local_n = std::popcount(grid & mask);
            int outer_n = std::popcount(outer & outer_neighbor_mask[idx]);
            int inner_n = std::popcount(inner & inner_neighbor_mask[idx]);

            int n = local_n + inner_n + outer_n;
            const bool has_bug = (grid & (1 << idx)) != 0;

            if (has_bug && n != 1)
                result &= ~(1 << idx);
            if (!has_bug && (n == 1 || n == 2))
                result |= (1 << idx);
        }
    }

    return result;
}

static int part1(uint32_t grid)
{
    dense_set<uint32_t> seen;
    while (seen.insert(grid).second)
        grid = step_single(grid);

    return grid;
}

static int part2(uint32_t grid)
{
    // blergh, deque is slow
    std::deque<uint32_t> world{grid};
    std::deque<uint32_t> next;

    for (int i = 0; i < 200; i++) {
        // Always pad with two 0's at each end, because I'm lazy.
        if (world.front() != 0) {
            world.push_front(0);
            world.push_front(0);
        } else if (auto it = std::next(world.begin()); it != world.end() && *it != 0) {
            world.push_front(0);
        }
        if (world.back() != 0) {
            world.push_back(0);
            world.push_back(0);
        } else if (auto it = std::next(world.rbegin()); it != world.rend() && *it != 0) {
            world.push_back(0);
        }

        next.clear();
        next.push_back(0);

        auto a = world.begin();
        auto b = std::next(a);
        auto c = std::next(b);
        for (; c != world.end(); a = b, b = c, ++c)
            next.push_back(step_recursive(*a, *b, *c));
        next.push_back(0);

        std::swap(world, next);
    }

    int num_bugs = 0;
    for (uint32_t mask : world)
        num_bugs += std::popcount(mask & ~cell_xy(2, 2));

    return num_bugs;
}

void run(std::string_view buf)
{
    uint32_t grid = 0;
    for (size_t j = 0; char c : buf) {
        if (c == '#') {
            grid |= 1 << j;
            j++;
        } else if (c == '.') {
            j++;
        }
    }

    fmt::print("{}\n", part1(grid));
    fmt::print("{}\n", part2(grid));
}
}
