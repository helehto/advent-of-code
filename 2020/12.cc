#include "common.h"

namespace aoc_2020_12 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<int> nums;
    nums.reserve(lines.size());
    find_numbers(buf, nums);

    Point<int> p(0, 0);
    int dx = 1;
    int dy = 0;
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
            int turns = value / 90;
            for (int i = 0; i < turns; ++i) {
                std::tie(dx, dy) = std::pair(-dy, dx);
            }
        } else if (c == 'L') {
            int turns = value / 90;
            for (int i = 0; i < turns; ++i) {
                std::tie(dx, dy) = std::pair(dy, -dx);
            }
        } else if (c == 'F') {
            p = p.translate(value * dx, value * dy);
        }
        i++;
    }
    fmt::print("{}\n", manhattan<int>(p, {0, 0}));

    std::tie(dx, dy) = std::pair(1, 0);
    p = {0, 0};
    Point<int> waypoint{10, -1};
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
            int turns = value / 90;
            for (int i = 0; i < turns; ++i) {
                std::tie(waypoint.x, waypoint.y) = std::pair(-waypoint.y, waypoint.x);
            }
        } else if (c == 'L') {
            int turns = value / 90;
            for (int i = 0; i < turns; ++i) {
                std::tie(waypoint.x, waypoint.y) = std::pair(waypoint.y, -waypoint.x);
            }
        } else if (c == 'F') {
            p = p.translate(value * waypoint.x, value * waypoint.y);
        }
        i++;
    }
    fmt::print("{}\n", manhattan<int>(p, {0, 0}));
}

}
