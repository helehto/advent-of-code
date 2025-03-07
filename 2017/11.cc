#include "common.h"

namespace aoc_2017_11 {

void run(std::string_view buf)
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

    fmt::print("{}\n", manhattan(pos));
    fmt::print("{}\n", furthest);
}

}
