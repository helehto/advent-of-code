#include "common.h"

namespace aoc_2023_18 {

static int64_t signed_area(std::span<const Vec2i64> points)
{
    int64_t result = 0;
    for (size_t i = 0; i < points.size(); i++) {
        auto p0 = i ? points[i - 1] : points.back();
        auto p1 = points[i];
        result += p0.x * p1.y - p1.x * p0.y;
    }
    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    int n = 0;

    std::vector<Vec2i64> polygon;
    uint64_t npoints = 0;
    for (Vec2i64 curr{0, 0}; std::string_view s : lines) {
        std::from_chars(s.begin() + 2, s.end(), n);
        auto next = s.front() == 'R'   ? curr + Vec2i64(n, 0)
                    : s.front() == 'D' ? curr + Vec2i64(0, n)
                    : s.front() == 'L' ? curr + Vec2i64(-n, 0)
                                       : curr + Vec2i64(0, -n);
        npoints += manhattan(curr, next);
        polygon.push_back(curr);
        curr = next;
    }
    fmt::print("{}\n", signed_area(polygon) / 2 + npoints / 2 + 1);

    polygon.clear();
    npoints = 0;
    for (Vec2i64 curr{0, 0}; std::string_view s : lines) {
        std::from_chars(s.begin() + s.find('#') + 1, s.end() - 2, n, 16);
        auto next = s[s.size() - 2] == '0'   ? curr + Vec2i64(n, 0)
                    : s[s.size() - 2] == '1' ? curr + Vec2i64(0, n)
                    : s[s.size() - 2] == '2' ? curr + Vec2i64(-n, 0)
                                             : curr + Vec2i64(0, -n);
        npoints += manhattan(curr, next);
        polygon.push_back(curr);
        curr = next;
    }
    fmt::print("{}\n", signed_area(polygon) / 2 + npoints / 2 + 1);
}

}
