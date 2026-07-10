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
    std::vector<uint32_t> atom_offsets;
    atom_offsets.reserve(input.size() / 2 + 1);
    std::vector<std::string_view> molecule;
    molecule.reserve(input.size() / 2 + 1);
    for (size_t i = 0; i < input.size();) {
        atom_offsets.push_back(i);
        if (i + 1 < input.size() && islower(input[i + 1])) {
            molecule.push_back(input.substr(i, 2));
            i += 2;
        } else {
            molecule.push_back(input.substr(i, 1));
            i++;
        }
    }
    atom_offsets.push_back(input.size());

    dense_set<std::string> seen;
    seen.reserve(5 * atom_offsets.size());
    std::string tmp;
    tmp.reserve(2 * atom_offsets.size());

    for (size_t i = 0; i + 1 < atom_offsets.size(); ++i) {
        auto it = recipes.find(molecule[i]);
        if (it == recipes.end())
            continue;

        for (std::string_view replacement : it->second) {
            tmp.clear();
            tmp += input.substr(0, atom_offsets[i]);
            tmp += replacement;
            tmp += input.substr(atom_offsets[i + 1], input.size() - atom_offsets[i + 1]);
            seen.insert(tmp);
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
