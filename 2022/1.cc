#include "common.h"
#include <algorithm>

using namespace std;

void run_2022_1(FILE *f)
{
    std::string s;
    std::vector<int> calories{0};

    while (getline(f, s)) {
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
