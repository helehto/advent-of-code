#include "common.h"
#include "inplace_vector.h"

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

void run(std::string_view buf)
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

        fmt::print("{}\n{}\n", mu + lambda, lambda);
    }
}

}
