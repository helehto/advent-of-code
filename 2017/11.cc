#include <algorithm>
#include <aoc/base.h>
#include <aoc/math.h>
#include <climits>
#include <string_view>

namespace aoc_2017_11 {

void run(std::string_view buf, aoc::Answer &answer)
{
    const char *p = buf.data();

    int furthest = INT_MIN;
    Vec2i pos{};
    for (; *p; ++p) {
        if (const char c = *p++; c == 'n') {
            if (*p == 'w') {
                --pos.x;
                ++p;
            } else if (*p == 'e') {
                pos += Vec2i{+1, -1};
                ++p;
            } else {
                --pos.y;
            }
        } else {
            if (*p == 'w') {
                pos += Vec2i{-1, +1};
                ++p;
            } else if (*p == 'e') {
                ++pos.x;
                ++p;
            } else {
                ++pos.y;
            }
        }
        furthest = std::max(furthest, manhattan(pos));
    }

    answer.add(manhattan(pos));
    answer.add(furthest);
}
AOC_REGISTER_SOLVER(2017, 11, run);

}
