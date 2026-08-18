#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/string.h>
#include <span>
#include <string_view>
#include <utility>

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

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);
    auto [total_code, total_mem] = part1(lines);
    answer.add(total_code - total_mem);
    answer.add(part2(lines) - total_code);
}
AOC_REGISTER_SOLVER(2015, 8, run);

}
