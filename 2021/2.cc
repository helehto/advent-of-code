#include "common.h"

namespace aoc_2021_2 {

void run(std::string_view buf, aoc::Answer &answer)
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;

    for (std::string_view s : split_lines(buf)) {
        int v = 0;

        if (s.starts_with("up")) {
            std::from_chars(s.data() + 3, s.data() + s.size(), v);
            y1 -= v;
        } else if (s.starts_with("down")) {
            std::from_chars(s.data() + 5, s.data() + s.size(), v);
            y1 += v;
        } else { // forward
            std::from_chars(s.data() + 8, s.data() + s.size(), v);
            x1 += v;
            x2 += v;
            y2 += y1 * v;
        }
    }

    answer.add(x1 * y1);
    answer.add(x2 * y2);
}
AOC_REGISTER_SOLVER(2021, 2, run);

}
