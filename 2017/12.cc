#include "common.h"

namespace aoc_2017_12 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<uint16_t> offsets(lines.size() + 1);
    std::vector<uint16_t> edges;
    {
        size_t offset = 0;
        for (size_t i = 0; std::string_view line : lines) {
            auto count = std::ranges::count(line, ',');
            offsets[i++] = offset;
            offset += count + 1;
        }
        offsets.back() = offset;
        edges.resize(offset);
    }

    auto edges_of = [&](size_t i) {
        return std::span(&edges[offsets[i]], offsets[i + 1] - offsets[i]);
    };

    small_vector<uint16_t> nums;
    for (size_t i = 0; std::string_view line : lines) {
        find_numbers(line, nums);
        std::copy(nums.begin() + 1, nums.end(), &edges[offsets[i++]]);
    }

    small_vector<uint16_t, 128> queue;
    std::vector<uint8_t> visited(lines.size(), false);

    auto flood = [&](uint16_t from) {
        for (queue = {from}; !queue.empty();) {
            auto u = queue.back();
            queue.pop_back();
            if (!std::exchange(visited[u], true))
                for (auto v : edges_of(u))
                    queue.push_back(v);
        }
    };

    flood(0);
    fmt::print("{}\n", std::ranges::count(visited, true));

    size_t last_index = 0;
    for (int groups = 1;; ++groups) {
        auto it = std::find(visited.begin() + last_index, visited.end(), false);
        if (it == visited.end()) {
            fmt::print("{}\n", groups);
            break;
        }
        last_index = std::distance(visited.begin(), it);
        flood(last_index);
    }
}

}
