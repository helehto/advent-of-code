#include "common.h"
#include "thread_pool.h"

namespace aoc_2024_6 {

enum { N, E, S, W };

struct alignas(4) State {
    uint16_t offset = 0;
    uint16_t dir = N;
};
static_assert(sizeof(State) == 4);

struct Bitset {
    std::vector<uint64_t> blocks;

    Bitset() noexcept = default;

    Bitset(size_t n_bits) noexcept
        : blocks((n_bits + 63) / 64, 0)
    {
    }

    void clear() noexcept { memset(blocks.data(), 0, blocks.size() * sizeof(blocks[0])); }

    bool test_and_set(size_t i) noexcept
    {
        size_t &block = blocks[i / 64];
        const uint64_t mask = UINT64_C(1) << (i & 63);
        const bool was_set = (block & mask) != 0;
        block |= mask;
        return was_set;
    }

    void for_each_set_bit(std::invocable<size_t> auto &&fn) const noexcept
    {
        for (size_t b = 0; b < blocks.size(); ++b)
            for (uint64_t m = blocks[b]; m; m &= m - 1)
                fn(b * 64 + std::countr_zero(m));
    }
};

constexpr char pad_value = '@';

static bool walk(MatrixView<const char> grid,
                 const State start,
                 std::invocable<State &&> auto &&visit)
{
    const ssize_t stride_by_direction[] = {
        -static_cast<ssize_t>(grid.cols), // N
        1,                                // E
        static_cast<ssize_t>(grid.cols),  // S
        -1,                               // W
    };

    size_t dir = start.dir;
    const char *q;
    for (const char *p = grid.data() + start.offset;; p = q) {
        q = p + stride_by_direction[dir];
        if (*q == pad_value) [[unlikely]]
            break;

        while (*q == '#') {
            dir = (dir + 1) & 3;
            q = p + stride_by_direction[dir];
        }

        if (!visit(State(q - grid.data(), dir)))
            return false;
    }

    return true;
}

static std::vector<uint16_t> initial_walk(MatrixView<const char> grid,
                                          const State start,
                                          std::array<Bitset, 4> &visited)
{
    for (auto &v : visited)
        v.clear();

    walk(grid, start, [&](const State &s) {
        visited[s.dir].test_and_set(s.offset);
        return true;
    });

    Bitset combined(grid.rows * grid.cols);
    for (size_t dir = 0; dir < 4; dir++)
        for (size_t i = 0; uint64_t b : visited[dir].blocks)
            combined.blocks[i++] |= b;

    size_t n_unique_offsets = 0;
    for (uint64_t b : combined.blocks)
        n_unique_offsets += std::popcount(b);

    std::vector<uint16_t> result;
    result.reserve(n_unique_offsets);
    combined.for_each_set_bit([&](size_t b) { result.push_back(b); });

    return result;
}

static int count_loops_with_obstructions(const Matrix<char> &original_grid,
                                         const State start,
                                         std::span<const uint16_t> obstructions)
{
    // Take a copy so that each thread can modify it independently.
    Matrix<char> grid = original_grid;

    std::array<Bitset, 4> visited;
    for (auto &v : visited)
        v = Bitset(grid.rows * grid.cols);

    int loops = 0;
    for (uint16_t offset : obstructions) {
        char c = grid.data()[offset];
        if (c == '.' || c == '^') {
            grid.data()[offset] = '#';
            for (auto &v : visited)
                v.clear();
            loops += !walk(grid, start, [&](const State &s) {
                return !visited[s.dir].test_and_set(s.offset);
            });
            grid.data()[offset] = c;
        }
    }

    return loops;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines).padded(1, pad_value);
    ASSERT(grid.rows < 256);
    ASSERT(grid.cols < 256);

    State start{};
    for (auto p : grid.ndindex<uint16_t>()) {
        if (grid(p) == '^') {
            start.offset = &grid(p) - grid.data();
            break;
        }
    }

    std::array<Bitset, 4> visited_states;
    for (auto &v : visited_states)
        v = Bitset(grid.rows * grid.cols);
    auto visited = initial_walk(grid, start, visited_states);
    fmt::print("{}\n", visited.size() + 1);

    std::atomic<int> total_loops = 0;
    ThreadPool::get().for_each(visited, [&](std::span<const uint16_t> subspan) {
        const auto loops = count_loops_with_obstructions(grid, start, subspan);
        total_loops.fetch_add(loops);
    });

    fmt::print("{}\n", total_loops.load());
}

}
