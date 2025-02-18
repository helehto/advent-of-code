#include "common.h"
#include "dense_map.h"
#include <algorithm>
#include <ranges>

namespace aoc_2018_4 {

struct Schedule {
    int16_t total;
    int16_t sleepiest_minute;
    std::array<int16_t, 60> per_minute;
};

static dense_map<int, Schedule> parse_schedules(std::span<std::string_view> lines)
{
    dense_map<int, Schedule> schedules;

    int current_guard = 0;
    int asleep_at = 0;
    for (auto line : lines) {
        int minute = 10 * (line[15] - '0') + line[16] - '0';

        if (line.ends_with("asleep")) {
            asleep_at = minute;
        } else if (line.ends_with("up")) {
            auto &s = schedules[current_guard];
            for (int i = asleep_at; i < minute; i++) {
                s.per_minute[i]++;
                if (s.per_minute[i] > s.per_minute[s.sleepiest_minute])
                    s.sleepiest_minute = i;
            }
            s.total += minute - asleep_at;
            asleep_at = -1;
        } else {
            std::from_chars(line.begin() + 26, line.end(), current_guard);
        }
    }

    return schedules;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    std::ranges::sort(lines);
    auto schedules = parse_schedules(lines);

    // Part 1:
    {
        auto &[id, sched] = *std::ranges::max_element(
            schedules, {}, [&](auto x) { return x.second.total; });
        fmt::print("{}\n", id * sched.sleepiest_minute);
    }

    // Part 2:
    {
        auto &[id, sched] = *std::ranges::max_element(schedules, {}, [&](auto x) {
            auto &s = x.second;
            return s.per_minute[s.sleepiest_minute];
        });
        fmt::print("{}\n", id * sched.sleepiest_minute);
    }
}

}
