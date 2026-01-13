#include "common.h"
#include <regex>

namespace aoc_2024_3 {

constexpr int parse_mul(const char *p)
{
    DEBUG_ASSERT(*p == ')');

    const char *a1 = p--;
    if (!(*p >= '0' && *p <= '9'))
        return 0;

    do {
        --p;
    } while (*p >= '0' && *p <= '9');
    const char *a0 = p + 1;

    if (*p != ',')
        return 0;

    const char *b1 = p--;
    if (!(*p >= '0' && *p <= '9'))
        return 0;

    do {
        --p;
    } while (*p >= '0' && *p <= '9');
    const char *b0 = p + 1;

    if (*--p != 'l')
        return 0;
    if (*--p != 'u')
        return 0;
    if (*--p != 'm')
        return 0;

    int a = 0;
    for (const char *q = a0; q < a1; ++q)
        a = a * 10 + (*q - '0');

    int b = 0;
    for (const char *q = b0; q < b1; ++q)
        b = b * 10 + (*q - '0');

    return a * b;
}

constexpr bool is_do(const char *p)
{
    DEBUG_ASSERT(*p == ')');
    return p[-3] == 'd' && p[-2] == 'o' && p[-1] == '(';
}

constexpr bool is_dont(const char *p)
{
    DEBUG_ASSERT(*p == ')');
    return p[-6] == 'd' && p[-5] == 'o' && p[-4] == 'n' && p[-3] == '\'' &&
           p[-2] == 't' && p[-1] == '(';
}

void run(std::string_view buf)
{
    int part1 = 0;
    int part2 = 0;
    int mask = -1;

    size_t i = SIZE_MAX;
    while (true) {
        i = buf.find(')', i + 1);
        if (i == std::string_view::npos)
            break;

        const char *p = &buf[i];
        if (is_do(p)) {
            mask = -1;
        } else if (is_dont(p)) {
            mask = 0;
        } else {
            int value = parse_mul(p);
            part1 += value;
            part2 += value & mask;
        }
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}

}
