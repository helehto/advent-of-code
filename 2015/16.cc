#include "common.h"

namespace aoc_2015_16 {

enum {
    CHILDREN,
    CATS,
    SAMOYEDS,
    POMERANIANS,
    AKITAS,
    VIZSLAS,
    GOLDFISH,
    TREES,
    CARS,
    PERFUMES,
};

void run(std::string_view buf)
{
    constexpr std::array<int8_t, 10> tape{3, 7, 2, 3, 0, 0, 5, 3, 2, 1};

    auto check1 = [&](const std::array<int8_t, 10> &aunt) {
        for (size_t i = 0; i < aunt.size(); ++i) {
            if (aunt[i] >= 0 && aunt[i] != tape[i])
                return false;
        }
        return true;
    };

    auto check2 = [&](const std::array<int8_t, 10> &aunt) {
        for (size_t i : {CHILDREN, SAMOYEDS, AKITAS, VIZSLAS, CARS, PERFUMES})
            if (aunt[i] >= 0 && aunt[i] != tape[i])
                return false;
        for (size_t i : {CATS, TREES})
            if (aunt[i] >= 0 && aunt[i] <= tape[i])
                return false;
        for (size_t i : {POMERANIANS, GOLDFISH})
            if (aunt[i] >= 0 && aunt[i] >= tape[i])
                return false;
        return true;
    };

    std::vector<std::string_view> items;
    int aunt1 = -1;
    int aunt2 = -1;
    for (size_t i = 1; std::string_view line : split_lines(buf)) {
        auto items_str = line.substr(line.find(":") + 2);

        std::array<int8_t, 10> aunt;
        aunt.fill(-1);

        for (std::string_view item : split(items_str, items, ',')) {
            auto colon = item.find(":");
            auto name = strip(item.substr(0, colon));
            auto quantity = item[colon + 2] - '0';

            if (name == "children")
                aunt[CHILDREN] = quantity;
            else if (name == "cats")
                aunt[CATS] = quantity;
            else if (name == "samoyeds")
                aunt[SAMOYEDS] = quantity;
            else if (name == "pomeranians")
                aunt[POMERANIANS] = quantity;
            else if (name == "akitas")
                aunt[AKITAS] = quantity;
            else if (name == "vizslas")
                aunt[VIZSLAS] = quantity;
            else if (name == "goldfish")
                aunt[GOLDFISH] = quantity;
            else if (name == "trees")
                aunt[TREES] = quantity;
            else if (name == "cars")
                aunt[CARS] = quantity;
            else if (name == "perfumes")
                aunt[PERFUMES] = quantity;
            else
                ASSERT(false);
        }

        if (aunt1 < 0 && check1(aunt))
            aunt1 = i;
        if (aunt2 < 0 && check2(aunt))
            aunt2 = i;
        i++;

        if (aunt1 >= 0 && aunt2 >= 0)
            break;
    }

    fmt::print("{}\n{}\n", aunt1, aunt2);
}

}
