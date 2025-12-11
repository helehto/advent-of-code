#include "common.h"
#include "dense_map.h"

namespace aoc_2025_11 {

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    dense_map<std::string_view, size_t> dict;
    dict.reserve(2 * lines.size());

    auto name2index = [&](std::string_view s) -> size_t {
        auto [it, inserted] = dict.emplace(s, dict.size());
        return it->second;
    };

    std::vector<std::string_view> tokens;
    tokens.reserve(32);
    for (std::string_view line : lines) {
        split(line, tokens, ' ');
        name2index(tokens[0].substr(0, tokens[0].size() - 1));
        for (auto v : std::span(tokens).subspan(1))
            name2index(v);
    }

    const size_t v_you = dict.at("you");
    const size_t v_svr = dict.at("svr");
    const size_t v_out = dict.at("out");
    const size_t v_fft = dict.at("fft");
    const size_t v_dac = dict.at("dac");

    std::vector<small_vector<uint16_t>> out_edges(dict.size() + 1);
    std::vector<small_vector<uint16_t>> in_edges(dict.size() + 1);
    for (std::string_view line : lines) {
        split(line, tokens, ' ');
        auto u = name2index(tokens[0].substr(0, tokens[0].size() - 1));
        for (auto sv : std::span(tokens).subspan(1)) {
            auto v = name2index(sv);
            in_edges[v].push_back(u);
            out_edges[u].push_back(v);
        }
    }

    // dfs to prune unreachable nodes
    std::vector<bool> reachable(dict.size(), false);
    {
        small_vector<uint16_t> queue{static_cast<uint16_t>(dict.at("svr"))};
        while (!queue.empty()) {
            size_t u = queue.back();
            queue.pop_back();
            if (!reachable[u]) {
                reachable[u] = true;
                queue.append_range(out_edges[u]);
            }
        }
        ASSERT(reachable[v_you]);

        for (size_t u = 0; u < in_edges.size(); ++u) {
            if (!reachable[u]) {
                for (auto &vs : out_edges)
                    erase(vs, u);
                for (auto &vs : in_edges)
                    erase(vs, u);
            }
        }
    }

    auto topo_sort = [&](uint16_t from) -> std::vector<uint16_t> {
        std::vector<uint16_t> result;
        result.reserve(in_edges.size());

        std::vector<uint16_t> n_edges(in_edges.size());
        for (size_t u = 0; u < in_edges.size(); ++u)
            n_edges[u] = in_edges[u].size();

        small_vector<uint16_t> queue = {from};
        while (!queue.empty()) {
            auto u = queue.back();
            queue.pop_back();
            result.push_back(u);
            for (size_t v : out_edges[u])
                if (!--n_edges[v])
                    queue.push_back(v);
        }
        return result;
    };

    auto topo_svr = topo_sort(v_svr);
    std::vector<int64_t> n_paths;
    auto count_paths = [&](size_t from, size_t to) {
        n_paths.assign(dict.size(), 0);
        n_paths[from]++;
        for (auto u : topo_svr)
            for (size_t v : out_edges[u])
                n_paths[v] += n_paths[u];
        return n_paths[to];
    };

    // Part 1
    fmt::print("{}\n", count_paths(v_you, v_out));

    // Part 2
    auto dac_index = std::ranges::find(topo_svr, v_dac) - topo_svr.begin();
    auto fft_index = std::ranges::find(topo_svr, v_fft) - topo_svr.begin();
    const size_t a = dac_index < fft_index ? v_dac : v_fft;
    const size_t b = dac_index < fft_index ? v_fft : v_dac;
    fmt::print("{}\n", count_paths(v_svr, a) * count_paths(a, b) * count_paths(b, v_out));
}

}
