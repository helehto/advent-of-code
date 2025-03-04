#include "common.h"

namespace aoc_2017_1 {

void run(std::string_view buf)
{
    int s1 = 0;
    for (size_t i = 1; i < buf.size(); ++i)
        if (buf[i - 1] == buf[i])
            s1 += buf[i] - '0';
    if (buf.front() == buf.back())
        s1 += buf.back() - '0';
    fmt::print("{}\n", s1);

    int s2 = 0;
    for (size_t i = 0; i < buf.size() / 2; ++i)
        if (buf[i] == buf[i + buf.size() / 2])
            s2 += buf[i] - '0';
    for (size_t i = buf.size() / 2; i < buf.size(); ++i)
        if (buf[i] == buf[i - buf.size() / 2])
            s2 += buf[i] - '0';
    fmt::print("{}\n", s2);
}
}
