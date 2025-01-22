#include "common.h"
#include "dense_map.h"

namespace aoc_2019_14 {

struct Recipe {
    int n;
    std::vector<std::pair<int, int>> ingredients;
};

static auto parse_quantity_and_name(std::string_view s)
{
    if (s[0] == ' ')
        s.remove_prefix(1);

    int n = 0;
    auto r = std::from_chars(s.begin(), s.end(), n);
    std::string_view name(r.ptr + 1, s.data() + s.size());
    return std::pair(n, name);
}

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    std::vector<std::string_view> tmp;

    std::vector<Recipe> recipes;
    recipes.resize(lines.size() + 1);

    dense_map<std::string_view, int> product_to_line;
    product_to_line.reserve(lines.size() + 1);
    for (size_t i = 0; std::string_view line : lines) {
        std::string_view product = line.substr(line.rfind(' ') + 1);
        product_to_line.emplace(product, i);
        i++;
    }
    product_to_line.emplace("ORE", product_to_line.size());

    size_t ore_index = product_to_line.size() - 1;
    size_t fuel_index = -1;
    for (size_t i = 0; std::string_view line : lines) {
        const auto arrow = line.find("=>");
        auto a = line.substr(0, arrow - 1);
        auto b = line.substr(arrow + 3);

        split(a, tmp, ',');
        std::vector<std::pair<int, int>> ingredients;
        recipes[i].ingredients.reserve(tmp.size());
        for (std::string_view v : tmp) {
            auto [q, name] = parse_quantity_and_name(v);
            recipes[i].ingredients.emplace_back(q, product_to_line.at(name));
        }

        auto [n_product, product] = parse_quantity_and_name(b);
        recipes[i].n = n_product;

        if (product == "FUEL")
            fuel_index = i;
        i++;
    }
    ASSERT(fuel_index != static_cast<size_t>(-1));

    std::vector<int> in_edges(product_to_line.size(), 0);
    in_edges[fuel_index] = 0;
    for (const auto &recipe : recipes)
        for (const auto &[n, dep] : recipe.ingredients)
            in_edges[dep]++;

    // Topological sort:
    std::vector<size_t> topo;
    topo.reserve(product_to_line.size());
    while (true) {
        auto it = std::ranges::find(in_edges, 0);
        const size_t index = std::distance(in_edges.begin(), it);
        if (index == ore_index)
            break;
        topo.push_back(index);
        in_edges[index] = INT_MAX;
        for (auto [n, dep] : recipes[index].ingredients)
            --in_edges[dep];
    }

    auto fuel_to_ore =
        [&, need = std::vector<int64_t>(product_to_line.size(), 0)](int64_t n) mutable {
            need.clear();
            need.resize(product_to_line.size());
            need[fuel_index] = n;
            for (int product : topo) {
                const Recipe &recipe = recipes[product];
                const int64_t crafts_needed = (need[product] + recipe.n - 1) / recipe.n;
                for (auto [items_per_craft, dep] : recipe.ingredients)
                    need[dep] += crafts_needed * items_per_craft;
            }
            return need[ore_index];
        };

    fmt::print("{}\n", fuel_to_ore(1));

    // Find an upper bound:
    constexpr int64_t ore_target = 1'000'000'000'000;
    int64_t ore_lo = 0;
    int64_t ore_hi = 0;
    int64_t fuel_hi = 20;
    int64_t fuel_lo = fuel_hi;
    while (true) {
        ore_hi = fuel_to_ore(fuel_hi);
        if (ore_hi > ore_target)
            break;
        fuel_lo = std::exchange(fuel_hi, fuel_hi * 16);
        ore_lo = ore_hi;
    }

    // Interpolation search:
    while (fuel_lo + 1 < fuel_hi) {
        const double t = static_cast<double>(ore_target - ore_lo) / (ore_hi - ore_lo);
        const int64_t fuel_p = std::lerp(fuel_lo, fuel_hi, t);
        const int64_t ore_p = fuel_to_ore(fuel_p);
        if (ore_p > ore_target) {
            ore_hi = ore_p;
            fuel_hi = fuel_p;
        } else if (ore_p < ore_target) {
            ore_lo = ore_p;
            fuel_lo = fuel_p;
        }
    }

    fmt::print("{}\n", fuel_lo);
}

}
