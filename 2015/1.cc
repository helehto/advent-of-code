#include "common.h"

namespace aoc_2015_1 {

void run(std::string_view buf)
{
    int p1 = 0;
    int p2 = -1;
    for (size_t i = 0; i < buf.size(); i++) {
        p1 = buf[i] == '(' ? p1 + 1 : p1 - 1;
        if (p1 < 0 && p2 < 0)
            p2 = i + 1;
    }
    fmt::print("{}\n", p1);
    fmt::print("{}\n", p2);
}

}
