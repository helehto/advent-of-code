#include "common.h"

namespace aoc_2018_8 {

template <int Part>
static std::pair<std::span<const uint8_t>, int> solve(std::span<const uint8_t> tree)
{
    int n_children = tree[0];
    int n_metadata = tree[1];
    tree = tree.subspan(2);

    int child_values[10] = {0};
    for (int i = 0; i < n_children; i++)
        std::tie(tree, child_values[i]) = solve<Part>(tree);

    int value = 0;
    if (Part == 1 || n_children == 0) {
        for (int x : tree.first(n_metadata))
            value += x;
    }

    if constexpr (Part == 1) {
        for (int i = 0; i < n_children; i++)
            value += child_values[i];
    } else if (n_children != 0) {
        for (int x : tree.first(n_metadata))
            value += child_values[x - 1];
    }

    return {tree.subspan(n_metadata), value};
}

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto tree = find_numbers<uint8_t>(lines[0]);
    fmt::print("{}\n", solve<1>(tree).second);
    fmt::print("{}\n", solve<2>(tree).second);
}

}
