#include "common.h"

namespace aoc_2020_12 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<int> nums;
    nums.reserve(lines.size());
    find_numbers(buf, nums);

    Vec2i p(0, 0);
    Vec2i d(1, 0);
    for (size_t i = 0; std::string_view line : lines) {
        const char c = line.front();
        const int value = nums[i];
        if (c == 'N') {
            p.y -= value;
        } else if (c == 'S') {
            p.y += value;
        } else if (c == 'E') {
            p.x += value;
        } else if (c == 'W') {
            p.x -= value;
        } else if (c == 'R') {
            for (int i = 0; i < value / 90; ++i)
                d = d.cw();
        } else if (c == 'L') {
            for (int i = 0; i < value / 90; ++i)
                d = d.ccw();
        } else if (c == 'F') {
            p += value * d;
        }
        i++;
    }
    fmt::print("{}\n", manhattan<int>(p, {0, 0}));

    p = {0, 0};
    d = {1, 0};
    Vec2i waypoint{10, -1};
    for (size_t i = 0; std::string_view line : lines) {
        const char c = line.front();
        const int value = nums[i];
        if (c == 'N') {
            waypoint.y -= value;
        } else if (c == 'S') {
            waypoint.y += value;
        } else if (c == 'E') {
            waypoint.x += value;
        } else if (c == 'W') {
            waypoint.x -= value;
        } else if (c == 'R') {
            for (int i = 0; i < value / 90; ++i)
                waypoint = waypoint.cw();
        } else if (c == 'L') {
            for (int i = 0; i < value / 90; ++i)
                waypoint = waypoint.ccw();
        } else if (c == 'F') {
            p += value * waypoint;
        }
        i++;
    }
    fmt::print("{}\n", manhattan<int>(p, {0, 0}));
}

}
