#include "common.h"
#include "dense_map.h"
#include "monotonic_bucket_queue.h"

namespace aoc_2018_22 {

constexpr Matrix<int> build_grid(const int depth, const Vec2i target)
{
    Matrix<int> result(3 * target.y + 1, 3 * target.x + 1);

    result(0, 0) = 0;
    for (size_t x = 1; x < result.rows; ++x)
        result(0, x) = (16807 * x + depth) % 20183;
    for (size_t y = 1; y < result.rows; ++y)
        result(y, 0) = (48271 * y + depth) % 20183;

    for (size_t y = 1; y < result.rows; ++y) {
        for (size_t x = 1; x < result.cols; ++x) {
            auto a = result(y, x - 1);
            auto b = result(y - 1, x);
            result(y, x) = (a * b + depth) % 20183;
        }
    }
    result(target) = 0;

    for (auto &e : result.all())
        e %= 3;

    return result;
}

constexpr int part1(const Matrix<int> &grid, const Vec2i target)
{
    int total_risk = 0;

    for (int y = 0; y <= target.y; ++y)
        for (int x = 0; x <= target.x; ++x)
            total_risk += grid(y, x);

    return total_risk;
}

constexpr int part2(const Matrix<int> &grid, const Vec2i target)
{
    enum class Gear : uint32_t { none, climbing_gear, torch };

    struct State {
        Gear gear;
        Vec2i pos;
        constexpr bool operator==(const State &) const noexcept = default;
    };

    auto is_valid_state = [&](const State &state) {
        switch (grid(state.pos)) {
        case 0:
            return state.gear == Gear::climbing_gear || state.gear == Gear::torch;
        case 1:
            return state.gear == Gear::climbing_gear || state.gear == Gear::none;
        case 2:
            return state.gear == Gear::torch || state.gear == Gear::none;
        }
        std::unreachable();
    };

    MonotonicBucketQueue<State> queue(7);
    queue.emplace(0, State(Gear::torch, Vec2i{0, 0}));

    Matrix<uint32_t> dist[3];
    for (auto &m : dist)
        m = Matrix<uint32_t>(grid.rows, grid.cols, UINT32_MAX);

    auto dist_of = [&](const State &state) -> uint32_t & {
        return dist[static_cast<int>(state.gear)](state.pos);
    };

    dist_of({Gear::torch, Vec2i{0, 0}}) = 0;

    while (auto u = queue.pop()) {
        if (*u == State(Gear::torch, target))
            return queue.current_priority();

        if (queue.current_priority() != dist_of(*u))
            continue;

        auto expand = [&](int cost, State next_state) {
            auto alt = queue.current_priority() + cost;
            if (auto &old = dist_of(next_state); alt < old) {
                old = alt;
                queue.emplace(alt, next_state);
            }
        };

        // Try equipping the other item suitable for this region:
        for (Gear g : {Gear::none, Gear::climbing_gear, Gear::torch})
            if (State next(g, u->pos); is_valid_state(next))
                expand(7, next);

        // Try moving to an adjacent region which is compatible with the item
        // that we have equipped:
        for (auto v : neighbors4(grid, u->pos))
            if (State next(u->gear, v); is_valid_state(next))
                expand(1, next);
    }

    ASSERT_MSG(false, "No path found!?");
}

void run(std::string_view buf)
{
    auto [depth, target_x, target_y] = find_numbers_n<int, 3>(buf);
    const Vec2i target{target_x, target_y};
    auto grid = build_grid(depth, target);
    fmt::print("{}\n", part1(grid, target));
    fmt::print("{}\n", part2(grid, target));
}

}
