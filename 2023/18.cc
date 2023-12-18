#include "common.h"

static int64_t signed_area(std::span<const Point<int64_t>> points)
{
    int64_t result = 0;
    for (size_t i = 0; i < points.size(); i++) {
        auto p0 = i ? points[i - 1] : points.back();
        auto p1 = points[i];
        result += (int64_t)(p0.x * p1.y) - (int64_t)(p1.x * p0.y);
    }
    return result;
}

void run_2023_18(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    int n = 0;

    std::vector<Point<int64_t>> polygon;
    uint64_t npoints = 0;
    for (Point<int64_t> curr{0, 0}; std::string_view s : lines) {
        std::from_chars(s.begin() + 2, s.end(), n);
        auto next = s.front() == 'R'   ? curr.translate(n, 0)
                    : s.front() == 'D' ? curr.translate(0, n)
                    : s.front() == 'L' ? curr.translate(-n, 0)
                                       : curr.translate(0, -n);
        npoints += manhattan(curr, next);
        polygon.push_back(curr);
        curr = next;
    }
    fmt::print("{}\n", signed_area(polygon) / 2 + npoints / 2 + 1);

    polygon.clear();
    npoints = 0;
    for (Point<int64_t> curr{0, 0}; std::string_view s : lines) {
        std::from_chars(s.begin() + s.find('#') + 1, s.end() - 2, n, 16);
        auto next = s[s.size() - 2] == '0'   ? curr.translate(n, 0)
                    : s[s.size() - 2] == '1' ? curr.translate(0, n)
                    : s[s.size() - 2] == '2' ? curr.translate(-n, 0)
                                             : curr.translate(0, -n);
        npoints += manhattan(curr, next);
        polygon.push_back(curr);
        curr = next;
    }
    fmt::print("{}\n", signed_area(polygon) / 2 + npoints / 2 + 1);
}
