#include "common.h"
#include <algorithm>
#include <fmt/core.h>
#include <string>
#include <vector>

using namespace std;

void run_2022_1(FILE *f)
{
    std::string s;
    std::vector<int> calories{0};

    while (getline(f, s)) {
        if (s.empty())
            calories.push_back(0);
        else
            calories.back() += std::stoi(s);
    }

    std::nth_element(begin(calories), begin(calories) + 2, end(calories));
    if (calories[0] > calories[1])
        std::swap(calories[0], calories[1]);

    fmt::print("{}\n", calories[0]);
    fmt::print("{}\n", calories[0] + calories[1] + calories[2]);
}
