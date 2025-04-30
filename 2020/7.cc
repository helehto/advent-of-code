#include "common.h"
#include "dense_map.h"
#include "dense_set.h"

namespace aoc_2020_7 {

void run(std::string_view buf)
{
    std::vector<std::string_view> bags;
    using Edge = std::pair<std::string_view, int>;
    dense_map<std::string_view, small_vector<Edge>> predecessors;
    dense_map<std::string_view, small_vector<Edge>> successors;
    for (std::string_view line : split_lines(buf)) {
        const size_t i = line.find(" bags contain ");
        ASSERT(i != std::string_view::npos);
        split(line.substr(i + 14), bags, ',');
        for (std::string_view s : bags) {
            int weight = -1;
            s = strip(s);
            auto r = std::from_chars(s.data(), s.data() + s.size(), weight);
            if (r.ec != std::errc())
                break;
            std::string_view bag2(r.ptr, s.data() + s.find(" bag") - r.ptr);
            predecessors[strip(bag2)].emplace_back(strip(line.substr(0, i)), weight);
            successors[strip(line.substr(0, i))].emplace_back(strip(bag2), weight);
        }
    }

    // Part 1:
    {
        small_vector<std::string_view, 512> queue{"shiny gold"};
        dense_set<std::string_view> visited;
        while (!queue.empty()) {
            std::string_view v = queue.back();
            queue.pop_back();
            visited.insert(v);

            if (auto it = predecessors.find(v); it != end(predecessors)) {
                for (auto &[u, _] : it->second)
                    queue.push_back(u);
            }
        }
        fmt::print("{}\n", visited.size() - 1);
    }

    // Part 2:
    {
        small_vector<std::pair<std::string_view, int>, 512> queue{{"shiny gold", 1}};
        int sum_weight = 0;
        while (!queue.empty()) {
            auto [u, count] = queue.back();
            queue.pop_back();
            sum_weight += count;

            if (auto it = successors.find(u); it != end(successors)) {
                for (auto &[v, weight] : it->second) {
                    queue.push_back({v, count * weight});
                }
            }
        }
        fmt::print("{}\n", sum_weight - 1);
    }
}

}
