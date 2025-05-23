#include "common.h"

namespace aoc_2023_11 {

static void expand_dimension(std::span<std::array<int, 2>> galaxies, int factor, int dim)
{
    small_vector<int> empty;
    for (size_t i = 1; i < galaxies.size(); i++) {
        for (int j = galaxies[i - 1][dim] + 1; j < galaxies[i][dim]; j++)
            if (empty.empty() || empty.back() != j)
                empty.push_back(j);
    }

    for (size_t i = galaxies.size(); i--;) {
        while (!empty.empty() && empty.back() > galaxies[i][dim])
            empty.pop_back();
        galaxies[i][dim] += (factor - 1) * empty.size();
    }
}

static int64_t solve(small_vector<std::array<int, 2>> galaxies, int factor)
{
    // Expand by y first since the input is naturally sorted by it (we read the
    // input line-by-line).
    expand_dimension(galaxies, factor, 1);
    std::ranges::sort(galaxies, λab(a[0] < b[0]));
    expand_dimension(galaxies, factor, 0);

    int64_t sum = 0;
    for (size_t i = 0; i < galaxies.size(); i++) {
        for (size_t j = i + 1; j < galaxies.size(); j++) {
            sum += abs(galaxies[i][0] - galaxies[j][0]) +
                   abs(galaxies[i][1] - galaxies[j][1]);
        }
    }

    return sum;
}

void run(std::string_view buf)
{
    small_vector<std::array<int, 2>> galaxies;

    for (size_t i = 0; std::string_view s : split_lines(buf)) {
        for (size_t j = 0; j < s.size(); j++) {
            if (s[j] == '#')
                galaxies.push_back({static_cast<int>(j), static_cast<int>(i)});
        }
        i++;
    }

    fmt::print("{}\n", solve(galaxies, 2));
    fmt::print("{}\n", solve(galaxies, 1'000'000));
}

}
