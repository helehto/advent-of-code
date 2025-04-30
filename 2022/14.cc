#include "common.h"
#include "dense_set.h"
#include <climits>

namespace aoc_2022_14 {

struct Cave {
    dense_set<Vec2i> occupied;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
};

static Cave make_cave(std::string_view buf)
{
    Cave cave;
    small_vector<int> nums;

    for (std::string_view s : split_lines(buf)) {
        find_numbers(s, nums);

        for (size_t i = 0; i + 3 < nums.size(); i += 2) {
            const auto x0 = nums[i];
            const auto y0 = nums[i + 1];
            const auto x1 = nums[i + 2];
            const auto y1 = nums[i + 3];

            if (x0 == x1) {
                for (int y = std::min(y0, y1); y <= std::max(y0, y1); y++)
                    cave.occupied.insert(Vec2{x0, y});
            } else {
                for (int x = std::min(x0, x1); x <= std::max(x0, x1); x++)
                    cave.occupied.insert(Vec2{x, y0});
            }
        }
    }

    cave.min_x = INT_MAX;
    cave.min_y = INT_MAX;
    cave.max_x = INT_MIN;
    cave.max_y = INT_MIN;
    for (auto &p : cave.occupied) {
        cave.min_x = std::min(cave.min_x, p.x);
        cave.max_x = std::max(cave.max_x, p.x);
        cave.min_y = std::min(cave.min_y, p.y);
        cave.max_y = std::max(cave.max_y, p.y);
    }

    return cave;
}

static std::tuple<Vec2i, bool>
drop_sand(Cave &cave, Vec2i pos, std::vector<Vec2i> &trajectory, int y_limit)
{
    while (true) {
        Vec2i candidate;

        if (pos.y + 1 == y_limit)
            return {pos, true};

        candidate = pos + Vec2i(0, 1);
        if (cave.occupied.count(candidate) == 0) {
            pos = candidate;
            trajectory.push_back(pos);
            continue;
        }

        candidate = pos + Vec2(-1, 1);
        if (cave.occupied.count(candidate) == 0) {
            pos = candidate;
            trajectory.push_back(pos);
            continue;
        }

        candidate = pos + Vec2(1, 1);
        if (cave.occupied.count(candidate) == 0) {
            pos = candidate;
            trajectory.push_back(pos);
            continue;
        }

        return {pos, false};
    }
}

static int solve(Cave cave, bool abyss)
{
    const int y_limit = cave.max_y + 2;
    const size_t num_walls = cave.occupied.size();
    constexpr Vec2 spawn = {500, 0};

    std::vector<Vec2i> trajectory;
    trajectory.reserve(y_limit);
    trajectory.push_back(spawn);

    while (!cave.occupied.count(spawn)) {
        const auto next_spawn = trajectory.back();
        auto [pos, reached_limit] = drop_sand(cave, next_spawn, trajectory, y_limit);
        if (reached_limit && abyss)
            break;

        cave.occupied.insert(pos);
        trajectory.pop_back();
    }

    return cave.occupied.size() - num_walls;
}

void run(std::string_view buf)
{
    Cave cave = make_cave(buf);
    fmt::print("{}\n", solve(cave, true));
    fmt::print("{}\n", solve(cave, false));
}

}
