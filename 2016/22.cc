#include "common.h"

namespace aoc_2016_22 {

constexpr int part1(std::span<const uint16_t> nums)
{
    const auto n_nodes = nums.size() / 6;

    std::vector<uint16_t> used(n_nodes);
    for (size_t i = 0; i < n_nodes; ++i)
        used[i] = nums[6 * i + 3];
    std::ranges::sort(used);

    std::vector<uint16_t> avail(n_nodes);
    for (size_t i = 0; i < n_nodes; ++i)
        avail[i] = nums[6 * i + 4];
    std::ranges::sort(avail);

    ASSERT(used[0] == 0);
    ASSERT(used[1] != 0);
    int viable_pairs = 0;
    for (size_t i = 1, j = 0; i < used.size(); ++i) {
        while (j < avail.size() && avail[j] < used[i])
            ++j;
        viable_pairs += avail.size() - j;
    }

    return viable_pairs;
}

constexpr int part2(std::span<const uint16_t> nums)
{
    const auto n_nodes = nums.size() / 6;

    uint16_t max_x = 0;
    for (size_t i = 0; i < n_nodes; ++i)
        max_x = std::max<uint16_t>(max_x, nums[6 * i + 0]);

    uint16_t max_y = 0;
    for (size_t i = 0; i < n_nodes; ++i)
        max_y = std::max<uint16_t>(max_y, nums[6 * i + 1]);

    Matrix<char> grid(max_y + 1, max_x + 1);
    Vec2i empty_node{};
    for (size_t i = 0; i < n_nodes; ++i) {
        const auto x = nums[6 * i + 0];
        const auto y = nums[6 * i + 1];

        if (nums[6 * i + 5] > 95) {
            grid(y, x) = '#';
        } else if (nums[6 * i + 3] == 0) {
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
    move_while({0, -1}, λ(p, q, grid(q) == '.'));
    move_while({-1, 0}, λ(p, q, grid(p + Vec2i{0, -1}) == '#'));
    move_while({0, -1}, λ(p, q, grid.in_bounds(q)));
    move_while({1, 0}, λ(p, q, grid(q) != 'G'));

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

    return steps;
}

void run(std::string_view buf)
{
    std::vector<uint16_t> nums;
    nums.reserve(2 + buf.size() / 48);
    find_numbers(buf, nums);
    ASSERT(nums.size() % 6 == 0);

    fmt::print("{}\n", part1(nums));
    fmt::print("{}\n", part2(nums));
}

}
