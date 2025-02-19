#include "common.h"
#include "dense_set.h"

namespace aoc_2016_1 {

static int part1(const std::vector<std::string_view> &fields)
{
    Vec2i p{}, d{0, 1};
    for (std::string_view field : fields) {
        field = strip(field);
        d = (field[0] == 'L') ? d.cw() : d.ccw();
        auto [n] = find_numbers_n<int, 1>(field);
        p += n * d;
    }
    return manhattan(p);
}

static int part2(const std::vector<std::string_view> &fields)
{
    dense_set<Vec2i> visited;
    visited.reserve(10'000);

    Vec2i p{}, d{0, 1};
    for (std::string_view field : fields) {
        field = strip(field);
        d = (field[0] == 'L') ? d.cw() : d.ccw();
        auto [n] = find_numbers_n<int, 1>(field);
        for (int i = 0; i < n; i++, p += d)
            if (auto [_, inserted] = visited.insert(p); !inserted)
                return manhattan(p);
    }
    __builtin_trap();
}

void run(std::string_view buf)
{
    std::vector<std::string_view> fields;
    split(buf, fields, ',');
    fmt::print("{}\n", part1(fields));
    fmt::print("{}\n", part2(fields));
}

}
