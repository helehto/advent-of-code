#include <algorithm>
#include <aoc/base.h>
#include <aoc/inplace_vector.h>
#include <aoc/string.h>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>

namespace aoc_2017_6 {

static void step(inplace_vector<int, 16> &v)
{
    auto it = std::ranges::max_element(v);
    size_t i = std::distance(v.begin(), it);
    size_t k = std::exchange(v[i], 0);
    while (true) {
        i++;
        if (i >= v.size())
            i = 0;
        if (!k--)
            break;
        v[i]++;
    }
}

void run(std::string_view buf, aoc::Answer &answer)
{
    auto blocksv = find_numbers<int>(buf);

    // Brent's cycle finding algorithm:
    {
        int power = 1, lambda = 1;
        inplace_vector<int, 16> tortoise(blocksv);
        inplace_vector<int, 16> hare(blocksv);
        step(hare);

        while (tortoise != hare) {
            if (power == lambda) {
                tortoise = hare;
                power *= 2;
                lambda = 0;
            }
            step(hare);
            ++lambda;
        }

        tortoise.assign_range(blocksv);
        hare.assign_range(blocksv);
        for (int i = 0; i < lambda; ++i)
            step(hare);

        int mu = 0;
        while (tortoise != hare) {
            step(tortoise);
            step(hare);
            ++mu;
        }

        answer.add(mu + lambda);
        answer.add(lambda);
    }
}
AOC_REGISTER_SOLVER(2017, 6, run);

}
