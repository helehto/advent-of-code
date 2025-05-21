#include "common.h"
#include "dense_map.h"
#include "dense_set.h"

namespace aoc_2015_19 {

static size_t part1(std::span<const std::string_view> lines)
{
    dense_map<std::string_view, small_vector<std::string_view>> recipes;

    for (size_t i = 0; !lines[i].empty(); ++i) {
        std::string_view line = lines[i];
        auto arrow = line.find("=>");
        auto a = line.substr(0, arrow - 1);
        auto b = line.substr(arrow + 3);
        recipes[a].push_back(b);
    }

    std::string_view input = lines.back();
    std::vector<std::string_view> molecule;
    for (size_t i = 0; i < input.size();) {
        if (i + 1 < input.size() && islower(input[i + 1])) {
            molecule.push_back(input.substr(i, 2));
            i += 2;
        } else {
            molecule.push_back(input.substr(i, 1));
            i++;
        }
    }

    dense_set<std::string> seen;
    seen.reserve(5 * molecule.size());

    for (size_t i = 0; i < molecule.size(); ++i) {
        auto it = recipes.find(molecule[i]);
        if (it == recipes.end())
            continue;
        for (std::string_view replacement : it->second) {
            std::vector<std::string_view> tmp = molecule;
            tmp[i] = replacement;
            auto result = fmt::format("{}", fmt::join(tmp, ""));
            seen.insert(std::move(result));
        }
    }

    return seen.size();
}

static int part2(std::string_view input)
{
    int rn = 0;
    int ar = 0;
    int y = 0;
    int atoms = 0;
    for (size_t i = 0; i < input.size(); ++atoms) {
        if (i + 1 < input.size() && islower(input[i + 1])) {
            if (input.substr(i, 2) == "Rn")
                rn++;
            if (input.substr(i, 2) == "Ar")
                ar++;
            i += 2;
        } else {
            if (input.substr(i, 1) == "Y")
                y++;
            i++;
        }
    }

    return atoms - rn - ar - 2 * y - 1;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    fmt::print("{}\n", part1(lines));
    fmt::print("{}\n", part2(lines.back()));
}

}
