#include "common.h"

namespace aoc_2017_9 {

void run(std::string_view buf)
{
    int score = 0;
    int depth = 1;
    int chars = 0;
    for (size_t i = 0; i < buf.size(); ++i) {
        const char c = buf[i];
        if (c == '<') {
            for (++i; buf[i] != '>'; ++i) {
                if (buf[i] == '!')
                    ++i;
                else
                    ++chars;
            }
        } else if (c == '{') {
            score += depth;
            ++depth;
        } else if (c == '}') {
            --depth;
        }
    }
    ASSERT(depth == 1);

    fmt::print("{}\n", score);
    fmt::print("{}\n", chars);
}

}
