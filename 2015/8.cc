#include "common.h"
#include "dense_map.h"

namespace aoc_2015_8 {

static int count_mem(std::string_view v)
{
    int mem_size = 0;
    ASSERT(v.front() == '"');

    while (true) {
        v.remove_prefix(1);
        ASSERT(!v.empty());
        char c = v.front();
        if (c == '"')
            break;

        if (c != '\\') {
            mem_size++;
            continue;
        }
        v.remove_prefix(1);

        switch (v.front()) {
        case '\\':
        case '"':
            mem_size++;
            break;
        case 'x':
            mem_size++;
            v.remove_prefix(2);
            break;
        default:
            ASSERT(!"bad escape");
        }
    }

    return mem_size;
}

static std::pair<int, int> part1(std::span<const std::string_view> lines)
{
    int total_code = 0;
    int total_mem = 0;

    for (const auto &s : lines) {
        total_code += s.size();
        total_mem += count_mem(s);
    }

    return {total_code, total_mem};
}

static int part2(std::span<const std::string_view> lines)
{
    int count = 0;

    for (const auto &s : lines) {
        for (auto c : s)
            if (c == '"' || c == '\\')
                count += 2;
            else
                count++;
        count += 2;
    }

    return count;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto [total_code, total_mem] = part1(lines);
    fmt::print("{}\n", total_code - total_mem);
    fmt::print("{}\n", part2(lines) - total_code);
}

}
