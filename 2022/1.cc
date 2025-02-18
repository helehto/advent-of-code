#include "common.h"
#include <algorithm>

namespace aoc_2022_1 {

using namespace std;

void run(std::string_view buf)
{
    std::vector<int> calories{0};

    for (std::string_view s : split_lines(buf)) {
        if (s.empty()) {
            calories.push_back(0);
        } else {
            int value = 0;
            std::from_chars(s.data(), s.data() + s.size(), value);
            calories.back() += value;
        }
    }

    std::nth_element(begin(calories), begin(calories) + 2, end(calories));
    if (calories[0] > calories[1])
        std::swap(calories[0], calories[1]);

    fmt::print("{}\n", calories[0]);
    fmt::print("{}\n", calories[0] + calories[1] + calories[2]);
}

}
