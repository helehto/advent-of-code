#include "common.h"
#include "dense_map.h"
#include "dense_set.h"

namespace aoc_2019_6 {

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);

    dense_map<std::string_view, int16_t> name_to_id;
    std::vector<std::vector<int16_t>> directed_edges(lines.size() + 1);
    std::vector<std::vector<int16_t>> undirected_edges(lines.size() + 1);

    name_to_id.reserve(lines.size() + 1);

    auto id_of = [&](std::string_view name) -> int16_t {
        auto [it, inserted] = name_to_id.emplace(name, name_to_id.size());
        return inserted ? name_to_id.size() - 1 : it->second;
    };

    for (std::string_view s : lines) {
        const size_t i = s.find(")");
        const int16_t a = id_of(s.substr(0, i));
        const int16_t b = id_of(s.substr(i + 1));
        directed_edges[a].push_back(b);
        undirected_edges[a].push_back(b);
        undirected_edges[b].push_back(a);
    }

    std::vector<std::pair<int16_t, int16_t>> queue;
    int part1 = 0;
    queue.reserve(lines.size());
    queue.emplace_back(name_to_id.at("COM"), 0);
    for (size_t i = 0; i < queue.size(); i++) {
        auto [u, depth] = queue[i];
        part1 += depth;
        for (int16_t v : directed_edges[u])
            queue.emplace_back(v, depth + 1);
    }
    fmt::print("{}\n", part1);

    const int16_t you = name_to_id.at("YOU");
    const int16_t san = name_to_id.at("SAN");
    queue = {{you, 0}};
    std::vector<bool> visited(name_to_id.size());
    for (size_t i = 0; i < queue.size(); i++) {
        auto [u, depth] = queue[i];
        visited[u] = true;
        if (u == san) {
            fmt::print("{}\n", depth - 2);
            break;
        }
        for (int16_t v : undirected_edges[u]) {
            if (!visited[v]) {
                queue.emplace_back(v, depth + 1);
            }
        }
    }
}

}
