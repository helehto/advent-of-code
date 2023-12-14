#include "common.h"
#include <algorithm>

using namespace std::literals;

void run_2022_25(FILE *f)
{
    std::string s;
    int64_t n = 0;
    while (getline(f, s)) {
        int64_t exp = 1;
        for (size_t i = s.size(); i--; exp *= 5)
            n += ("=-012"sv.find(s[i]) - 2) * exp;
    }

    std::string result;
    while (n) {
        auto r = n % 5;
        result.push_back("012=-"[r]);
        n = n / 5 + (r >= 3);
    }

    std::reverse(begin(result), end(result));
    fmt::print("{}\n", result);
}
