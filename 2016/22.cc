#include "common.h"

namespace aoc_2016_22 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    int max_x = INT_MIN;
    int max_y = INT_MIN;
    int viable_pairs = 0;
    for (size_t i = 2; i < lines.size(); ++i) {
        const auto [x_a, y_a, size_a, used_a, avail_a, usepct_a] =
            find_numbers_n<int, 6>(lines[i]);

        for (size_t j = 2; j < lines.size(); ++j) {
            if (i == j)
                continue;

            const auto [x_b, y_b, size_b, used_b, avail_b, usepct_b] =
                find_numbers_n<int, 6>(lines[j]);

            if (used_a != 0 && used_a <= avail_b)
                viable_pairs++;
        }

        max_x = std::max(max_x, x_a);
        max_y = std::max(max_y, y_a);
    }
    fmt::print("{}\n", viable_pairs);

    Matrix<char> grid(max_y + 1, max_x + 1);
    Vec2i empty_node{};
    for (size_t i = 2; i < lines.size(); ++i) {
        const auto [x, y, size, used, avail, usepct] = find_numbers_n<int, 6>(lines[i]);
        if (usepct > 95) {
            grid(y, x) = '#';
        } else if (used == 0) {
            grid(y, x) = '@';
            empty_node = {x, y};
        } else {
            grid(y, x) = '.';
        }
    }
    grid(0, grid.cols - 1) = 'G';

    int steps = 0;

    auto move = [&](Vec2i dir) {
        const auto q = empty_node + dir;
        std::swap(grid(empty_node), grid(q));
        empty_node = q;
        steps++;
    };

    auto move_while = [&](Vec2i dir, auto &&pred) {
        while (pred(empty_node, empty_node + dir))
            move(dir);
    };

    //
    // TODO: The solution below is hard-coded for my input. Generalize it by
    // implementing A*.
    //

    // Move around the all and to the goal node in the top-right corner:
    move_while({0, -1}, [&](auto, auto q) { return grid(q) == '.'; });
    move_while({-1, 0}, [&](auto p, auto) { return grid(p + Vec2i{0, -1}) == '#'; });
    move_while({0, -1}, [&](auto, auto q) { return grid.in_bounds(q); });
    move_while({1, 0}, [&](auto, auto q) { return grid(q) != 'G'; });

    // Move the goal data to (0,0):
    constexpr Vec2i target{0, 0};
    while (true) {
        move({1, 0});
        if (grid(target) == 'G')
            break;
        move({0, 1});
        move({-1, 0});
        move({-1, 0});
        move({0, -1});
    }

    fmt::print("{}\n", steps);
}

}
