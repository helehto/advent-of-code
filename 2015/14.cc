#include "common.h"
#include "dense_map.h"

namespace aoc_2015_14 {

struct Reindeer {
    int velocity;
    int duration;
    int rest_time;
};

static std::vector<Reindeer> parse_input(std::string_view buf)
{
    std::vector<Reindeer> reindeer;

    for (std::string_view s : split_lines(buf)) {
        auto [v, d, r] = find_numbers_n<int, 3>(s);
        reindeer.push_back({.velocity = v, .duration = d, .rest_time = r});
    }

    return reindeer;
}

static int fly_part1(const Reindeer &r, int t)
{
    int cycle_length = r.duration + r.rest_time;
    int full_cycles = t / cycle_length;
    int slack = std::min(r.duration, t % cycle_length) * r.velocity;
    return full_cycles * r.velocity * r.duration + slack;
}

static int part2(const std::vector<Reindeer> &reindeer, int time)
{
    std::vector<int> positions(reindeer.size());
    std::vector<int> scores(reindeer.size());

    for (int t = 0; t < time; t++) {
        for (size_t i = 0; i < reindeer.size(); i++) {
            auto &r = reindeer[i];
            if (t % (r.duration + r.rest_time) < r.duration)
                positions[i] += r.velocity;
        }

        auto m = *std::max_element(begin(positions), end(positions));
        for (size_t i = 0; i < reindeer.size(); i++) {
            if (positions[i] == m)
                scores[i]++;
        }
    }

    return *std::max_element(begin(scores), end(scores));
}

void run(std::string_view buf)
{
    const auto reindeer = parse_input(buf);

    int distance = 0;
    for (auto &r : reindeer)
        distance = std::max(distance, fly_part1(r, 2503));

    fmt::print("{}\n", distance);
    fmt::print("{}\n", part2(reindeer, 2503));
}

}
