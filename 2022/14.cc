#include "common.h"

namespace aoc_2022_14 {

enum { EMPTY = '.', WALL = '#', SAND = 'o' };

struct Cave {
    Matrix<char> grid;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
};

static Cave make_cave(std::string_view buf)
{
    Cave cave;
    small_vector<int> nums;
    auto lines = split_lines(buf);

    int min_x = 499;
    int min_y = 0;
    int max_x = 501;
    int max_y = 0;

    auto read_xy_pair = [&](int *n) {
        return std::tuple(std::min(n[0], n[2]), std::min(n[1], n[3]),
                          std::max(n[0], n[2]), std::max(n[1], n[3]));
    };

    for (std::string_view s : lines) {
        find_numbers(s, nums);

        for (size_t i = 0; i + 3 < nums.size(); i += 2) {
            const auto [x0, y0, x1, y1] = read_xy_pair(&nums[i]);
            min_x = std::min(min_x, x0);
            max_x = std::max(max_x, x1);
            min_y = std::min(min_y, y0);
            max_y = std::max(max_y, y1);
        }
    }

    // Sand might spill over the sides defined by the walls. Extend the grid to
    // allow for that. I *think* that it can extend as far as the height of the
    // cave to either side (forming a triangle once it piles up to the spawn),
    // but I don't have an ironclad proof of that...
    const auto dy = max_y - min_y;
    min_x -= dy;
    max_x += dy;

    cave.grid = Matrix<char>(max_y - min_y + 2, max_x - min_x + 1, EMPTY);

    for (std::string_view s : lines) {
        find_numbers(s, nums);

        for (size_t i = 0; i + 3 < nums.size(); i += 2) {
            const auto [x0, y0, x1, y1] = read_xy_pair(&nums[i]);
            if (x0 == x1) {
                for (int y = y0; y <= y1; y++)
                    cave.grid(y - min_y, x0 - min_x) = WALL;
            } else {
                for (int x = x0; x <= x1; x++)
                    cave.grid(y0 - min_y, x - min_x) = WALL;
            }
        }
    }

    cave.min_x = min_x;
    cave.max_x = max_x;
    cave.min_y = min_y;
    cave.max_y = max_y;
    return cave;
}

static std::tuple<char *, bool>
drop_sand(MatrixView<char> grid, char *pos, char **&trajectory, char *limit)
{
    auto try_place = [&](ssize_t x_offset) {
        char *next = pos + grid.cols + x_offset;
        if (*next != EMPTY)
            return false;
        pos = next;
        *++trajectory = pos;
        return true;
    };

    while (pos < limit) {
        if (!try_place(0) && !try_place(-1) && !try_place(1))
            return {pos, false};
    }
    return {pos, true};
}

static int solve(Cave cave, bool abyss)
{
    size_t at_rest = 0;
    char *spawn = &cave.grid(-cave.min_y, 500 - cave.min_x);
    int y_limit = cave.max_y + 1;
    char *limit = cave.grid.data() + y_limit * cave.grid.cols;

    auto trajectory_stack = std::make_unique_for_overwrite<char *[]>(y_limit + 1);
    char **trajectory = trajectory_stack.get();
    *trajectory = spawn;

    while (*spawn == EMPTY) {
        const auto next_spawn = *trajectory;
        auto [pos, reached_limit] = drop_sand(cave.grid, next_spawn, trajectory, limit);
        if (reached_limit && abyss)
            break;

        *pos = SAND;
        at_rest++;
        trajectory--;
    }

    return at_rest;
}

void run(std::string_view buf)
{
    Cave cave = make_cave(buf);
    fmt::print("{}\n", solve(cave, true));
    fmt::print("{}\n", solve(std::move(cave), false));
}

}
