#include "common.h"
#include <thread>

namespace aoc_2024_20 {

/// Count the number of cheats from an interior square `p` in the grid.
///
/// Since it is an interior square, which we guarantee by padding the input
/// grid, we can elide all bounds checking and complex branching. Additionally,
/// since N is known at compile time (being either 2 or 20), we use a bit of
/// template metaprogramming here to make the entire set of squares known to
/// the compiler ahead of time.
///
/// Doing this, flattening the function, and letting the autovectorizer go to
/// town effectively generates a completely branchless and statically scheduled
/// kernel for counting the number of cheats from a specific square for a given
/// N.
///
/// The total flattened machine code size of the function grows as O(N^2), so
/// the technique is less viable for larger values of N, but for N = 20 it
/// generates about ~8K of code on my setup, which fits comfortably in a 32K L1
/// icache (e.g. Zen 4).
template <int N>
[[gnu::noinline, gnu::flatten]] constexpr int
count_cheats_from_square(const Matrix<int32_t> &dist, const Vec2i p)
{
    const int32_t dist_p = dist(p);

    auto count_single_row = [&]<int DY>() {
        constexpr int row_offset = N - std::abs(DY);
        const int32_t *dist_cheat = &dist(p.y + DY, p.x - row_offset);

        int32_t cheats = 0;
        for (int32_t dx = -row_offset; dx <= row_offset; ++dx, ++dist_cheat) {
            // Branchless sequence to conditionally increment the number of
            // cheats. This uses the fact that the distance matrix contains a
            // huge number for walls, making sure that we never consider time
            // entering such a square to be a cheat.
            const int32_t cheat_cost = std::abs(dx) + std::abs(DY);
            const int32_t path_cost = dist_p - *dist_cheat;
            const int32_t time_saved = path_cost - cheat_cost;
            cheats -= (99 - time_saved) >> 31;
        }

        return cheats;
    };

    auto count_all_rows = [&]<int... Idxs>(std::integer_sequence<int, Idxs...>) {
        return (count_single_row.template operator()<Idxs - N>() + ...);
    };

    return count_all_rows(std::make_integer_sequence<int, 2 * N + 1>());
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    constexpr int N = 20;
    auto grid = Matrix<char>::from_lines(lines).padded(20, '#');

    Vec2i start, end;
    for (auto p : grid.ndindex<int>()) {
        if (grid(p) == 'S')
            start = p;
        else if (grid(p) == 'E')
            end = p;
    }

    // First run a straight-forward BFS to determine which squares are
    // accessible and their distance to the end.
    Matrix<int32_t> dist(grid.rows, grid.cols, INT32_MAX / 2);
    std::vector<std::pair<int, Vec2i>> queue{{0, end}};
    for (size_t i = 0; i < queue.size(); ++i) {
        auto [d, u] = queue[i];
        dist(u) = d;

        for (auto v : neighbors4(grid, u))
            if (grid(v) != '#' && dist(v) == INT32_MAX / 2)
                queue.emplace_back(d + 1, v);
    }

    auto solve = [&]<int N>(size_t begin, size_t end) {
        int cheats = 0;
        for (size_t i = begin; i < end; ++i)
            cheats += count_cheats_from_square<N>(dist, queue[i].second);
        return cheats;
    };

    // Solve the problem in parallel by slicing the grid up among multiple
    // threads. The input grid isn't all that big (340x340 after padding), so
    // we won't gain much by launching a ton of threads; 2-4 appears to work
    // best here.
    std::atomic<int> part1{0};
    std::atomic<int> part2{0};
    {
        std::array<std::jthread, 4> threads;
        const size_t slice_size = (queue.size() / threads.size()) + 1;

        for (size_t i = 0; i < threads.size(); ++i) {
            threads[i] = std::jthread([&, i] noexcept {
                const size_t begin = i * slice_size;
                const size_t end = std::min((i + 1) * slice_size, queue.size());
                part1.fetch_add(solve.operator()<2>(begin, end));
                part2.fetch_add(solve.operator()<N>(begin, end));
            });
        }
    }

    fmt::print("{}\n", part1.load());
    fmt::print("{}\n", part2.load());
}
}
