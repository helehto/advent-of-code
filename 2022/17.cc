#include "common.h"
#include "dense_map.h"
#include "dense_set.h"
#include <climits>
#include <numeric>
#include <optional>

namespace aoc_2022_17 {

static std::vector<Point<int>> rock_templates[] = {
    // clang-format off
    {{0, 0}, {1, 0}, {2, 0}, {3, 0}},
    {{1, 0}, {0, 1}, {1, 1}, {2, 1}, {1, 2}},
    {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}},
    {{0, 0}, {0, 1}, {0, 2}, {0, 3}},
    {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
    // clang-format on
};

static bool try_move(const dense_set<Point<int>> &occupied,
                     std::vector<Point<int>> &rock,
                     int dx,
                     int dy)
{
    for (const auto &p : rock) {
        if (p.x + dx < 0 || p.x + dx >= 7 || p.y + dy < 0)
            return false;
        if (occupied.count(p.translate(dx, dy)))
            return false;
    }

    for (auto &p : rock)
        p = p.translate(dx,dy);

    return true;
}

struct CacheKey {
    size_t rock_idx;
    size_t jet_idx;

    constexpr bool operator==(const CacheKey &) const noexcept = default;
};

} // namespace aoc_2022_17

template <>
struct std::hash<aoc_2022_17::CacheKey> {
    size_t operator()(const aoc_2022_17::CacheKey &k) const noexcept
    {
        auto h = std::hash<size_t>{};
        return h(k.rock_idx) ^ h(k.jet_idx);
    }
};

namespace aoc_2022_17 {

struct State {
    int n;
    int height;
};

struct Cycle {
    int start;
    int period;
    int height;
};

static std::optional<Cycle> detect_cycle(const std::vector<State> &state_vec)
{
    // This has a boatload of stupid assumptions that happened to work on the
    // input, but I can't be bothered to go back and rework this now.

    constexpr static int streak_len = 3;
    if (state_vec.size() <= streak_len)
        return std::nullopt;
    auto streak = std::span(state_vec).last(streak_len);

    const int delta_n = streak[1].n - streak[0].n;
    const int delta_height = streak[1].height - streak[0].height;

    for (size_t i = 2; i < streak.size(); ++i) {
        if (streak[i].n - streak[i - 1].n != delta_n)
            return std::nullopt;
        if (streak[i].height - streak[i - 1].height != delta_height)
            return std::nullopt;
    }

    return Cycle{
        .start = streak.back().n,
        .period = delta_n,
        .height = delta_height,
    };
}

void run(FILE *f)
{
    std::string jets;
    getline(f, jets);

    dense_map<CacheKey, std::vector<State>> past_states;
    dense_set<Point<int>> occupied;
    std::vector<int> heights;
    size_t rock_idx = 0;
    size_t jet_idx = 0;
    int height = 0;
    int fallen_rocks = 0;
    Cycle cycle;

    while (true) {
        auto &past_occurrences = past_states[{rock_idx, jet_idx}];
        past_occurrences.emplace_back(fallen_rocks, height);

        // Are we in a cycle?
        if (auto c = detect_cycle(past_occurrences)) {
            cycle = *c;
            break;
        }

        // Spawn the rock.
        std::vector<Point<int>> rock;
        auto &rock_template = rock_templates[rock_idx];
        rock_idx = (rock_idx + 1) % std::size(rock_templates);
        for (const auto &[x, y] : rock_template)
            rock.emplace_back(x + 2, y + height + 3);

        // Drop the rock.
        while (true) {
            // It gets pushed by the jet of gas, if possible:
            const auto jet = jets[jet_idx];
            jet_idx = (jet_idx + 1) % jets.size();
            try_move(occupied, rock, (jet == '<' ? -1 : 1), 0);

            // It falls, if possible:
            if (!try_move(occupied, rock, 0, -1)) {
                occupied.insert(begin(rock), end(rock));

                int max_y = INT_MIN;
                for (auto &[_, y] : rock)
                    max_y = std::max(max_y, y);

                height = std::max(height, max_y + 1);
                heights.push_back(height);
                fallen_rocks++;
                break;
            }
        }
    }

    auto solve = [&](int64_t target) {
        auto cycles = (target - cycle.start) / cycle.period;
        auto offset = (target - cycle.start) % cycle.period;

        if (offset < 0) {
            // This occurs if we detect the cycle after the target number of
            // iterations (which occurs for part 1).
            cycles--;
            offset += cycle.period;
        }

        return height + cycles * cycle.height +
               heights[cycle.start - cycle.period - 1 + offset] -
               heights[cycle.start - cycle.period - 1];
    };

    fmt::print("{}\n", solve(2022));
    fmt::print("{}\n", solve(1000000000000));
}

}
