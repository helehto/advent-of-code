#include "common.h"
#include <fmt/core.h>
#include <numeric>
#include <span>
#include <vector>

static int cost1(std::span<int> crabs, int goal)
{
    int cost = 0;

    for (int p : crabs)
        cost += abs(p - goal);

    return cost;
}

static int cost2(std::span<int> crabs, int goal)
{
    int cost = 0;

    for (int p : crabs) {
        int d = abs(p - goal);
        cost += d * (d + 1) / 2;
    }

    return cost;
}

int part1(std::span<int> crabs)
{
    // We want to minimize f(x, G) = sum(abs(x_i - G), i=0..n) with respect to
    // G, where x_i is the position of each individual crab and G is the goal
    // position.
    //
    // Taking the derivative with respect to G, we obtain
    //
    //     df/dG = sum(d/dx abs(x_i - G), i=1..n)
    //           = sum(-1) + sum(1)
    //
    // where the number of -1 and 1 terms depends on how many crabs are to
    // the left or right of the goal, respectively. Hence, the derivative is
    // zero when these sums are balanced, i.e. the median position.

    int median = crabs[crabs.size() / 2];
    return cost1(crabs, median);
}

int part2(std::span<int> crabs)
{
    // The cost function is now changed such that a distance of d has a
    // cost equal to the d'th triangular number. Similarly to part 1, We
    // want to minimize
    //
    //     f(x, G) = sum((x_i - G) * (x_i - G + 1) / 2)
    //             = 1/2 sum(x_i^2 - 2G x_i + x_i + G^2)
    //
    // with respect to G. Taking the derivative with respect to G:
    //
    //     df/dG = d/dG 1/2 sum(x_i^2 - 2G x_i + x_i + G^2)
    //           = 1/2 sum(-2x_i + 2G)
    //           = sum(-x_i + G)
    //
    // Then,
    //
    //     df/dG = 0 ==> sum(-x_i + G) = 0
    //               ==> G = sum(x_i) / n.
    //
    // Hence the cost is minimized by setting G to the mean of the crabs'
    // positions. Since the mean is not necessarily an integer, we test
    // rounding both ways to see which one is smaller.

    float goal = std::accumulate(begin(crabs), end(crabs), 0.f) / crabs.size();
    return std::min(cost2(crabs, ceilf(goal)), cost2(crabs, floorf(goal)));
}

int run_2021_7(FILE *f)
{
    std::string s;
    getline(f, s);
    auto crabs = find_numbers<int>(s);
    std::sort(begin(crabs), end(crabs));

    printf("%d\n", part1(crabs));
    printf("%d\n", part2(crabs));
    return 0;
}
