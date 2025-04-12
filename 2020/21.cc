#include "common.h"
#include "dense_map.h"

namespace aoc_2020_21 {

using namespace std::literals;

struct Recipe {
    std::array<uint64_t, 4> ingredient_mask;
    uint64_t allergen_mask;
};

struct Dictionary {
    dense_map<std::string_view, uint16_t> indices;
    std::vector<std::string_view> names;

    uint16_t insert(std::string_view key)
    {
        auto [it, inserted] = indices.emplace(key, indices.size());
        if (inserted)
            names.push_back(key);
        return it->second;
    }
};

struct Input {
    Dictionary ingredients;
    Dictionary allergens;
    std::vector<Recipe> recipes;
};

static Input parse_input(std::string_view buf)
{
    Input result;

    std::vector<std::string_view> tokens;
    tokens.reserve(256);

    auto lines = split_lines(buf);

    for (std::string_view line : lines) {
        auto lparen = line.find('(');
        ASSERT(lparen != std::string_view::npos);

        Recipe &recipe = result.recipes.emplace_back();

        auto ingredients = line.substr(0, lparen);
        split(strip(ingredients), tokens, ' ');
        for (std::string_view s : tokens) {
            auto id = result.ingredients.insert(s);
            ASSERT(id < 64 * recipe.ingredient_mask.size());
            recipe.ingredient_mask[id / 64] |= uint64_t(1) << (id & 63);
        }

        auto allergens = line.substr(lparen + "(contains "sv.size());
        allergens.remove_suffix(1);
        split(strip(allergens), tokens, ',');
        for (std::string_view s : tokens) {
            auto id = result.allergens.insert(strip(s));
            ASSERT(id < 64);
            recipe.allergen_mask |= uint64_t(1) << id;
        }
    }

    return result;
}

void run(std::string_view buf)
{
    Input input = parse_input(buf);
    const size_t n_ingredients = input.ingredients.indices.size();
    const size_t n_allergens = input.allergens.indices.size();

    const auto allergen_ingredients_mask = [&] {
        std::vector<std::array<uint64_t, 4>> result;

        for (size_t i = 0; i < n_allergens; ++i) {
            std::array<uint64_t, 4> mask;
            mask.fill(UINT64_MAX);

            for (const auto &[ingredient_mask, allergens] : input.recipes) {
                if (allergens & (uint64_t(1) << i)) {
                    for (size_t j = 0; j < ingredient_mask.size(); ++j)
                        mask[j] &= ingredient_mask[j];
                }
            }

            result.push_back(mask);
        }

        // For each allergen, resolve which ingredient that contain it. For
        // valid inputs, all elements in the output array should contain a
        // single bit after this step, corresponding to a single ingredient.
        bool changed;
        do {
            changed = false;
            for (size_t i = 0; i < n_allergens; ++i) {
                const std::array<uint64_t, 4> &mask = result[i];
                if (const int popcnt = std::popcount(mask[0]) + std::popcount(mask[1]) +
                                       std::popcount(mask[2]) + std::popcount(mask[3]);
                    popcnt != 1)
                    continue;

                for (size_t j = 0; j < n_allergens; ++j) {
                    if (i == j)
                        continue;

                    auto next = result[j];
                    for (size_t k = 0; k < next.size(); ++k)
                        next[k] &= ~mask[k];

                    if (next != result[j]) {
                        changed = true;
                        result[j] = next;
                    }
                }
            }
        } while (changed);

        return result;
    }();

    // After computing and reducing the allergen => ingredient masks above,
    // each mask now only contains a single element. Simplify this to a single
    // integer: the number of trailing zeros in the mask (i.e. the index of the
    // ingredient).
    std::vector<uint64_t> allergen_of_ingredient;
    allergen_of_ingredient.reserve(n_allergens);
    for (const std::array<uint64_t, 4> &m : allergen_ingredients_mask) {
        for (size_t i = 0; i < m.size(); ++i) {
            if (m[i]) {
                allergen_of_ingredient.push_back(64 * i + std::countr_zero(m[i]));
                break;
            }
        }
    }

    // Construct the reverse map: given an ingredient, what allergen does it
    // contain?
    std::vector<int64_t> ingredient_of_allergen(n_ingredients, -1);
    for (size_t i = 0; i < n_allergens; ++i)
        ingredient_of_allergen[allergen_of_ingredient[i]] = i;

    // Part 1: count the number of instances of harmless ingredients with no
    // allergens across all recipes.
    {
        int harmless_ingredients = 0;

        for (const Recipe &recipe : input.recipes) {
            for (size_t i = 0; i < recipe.ingredient_mask.size(); ++i) {
                uint64_t m = recipe.ingredient_mask[i];
                for (; m; m &= m - 1)
                    if (ingredient_of_allergen[64 * i + std::countr_zero(m)] < 0)
                        harmless_ingredients++;
            }
        }

        fmt::print("{}\n", harmless_ingredients);
    }

    // Part 2: compute the list of ingredients with allergens, sorted by the
    // name of the allergen.
    {
        std::vector<size_t> danger_list;
        danger_list.reserve(n_allergens);
        for (size_t i = 0; i < ingredient_of_allergen.size(); ++i)
            if (ingredient_of_allergen[i] >= 0)
                danger_list.push_back(i);

        std::ranges::sort(danger_list, {},
                          λx(input.allergens.names[ingredient_of_allergen[x]]));

        fmt::print("{}\n", fmt::join(danger_list | std::ranges::views::transform(
                                                       λx(input.ingredients.names[x])),
                                     ","));
    }
}

}
