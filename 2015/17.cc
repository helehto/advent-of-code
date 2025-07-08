#include "common.h"

namespace aoc_2015_17 {

static void search(const small_vector_base<int> &containers,
                   small_vector_base<int> &result,
                   size_t used = 0,
                   int left = 150,
                   size_t i = 0)
{
    if (left == 0) [[unlikely]] {
        result[used]++;
        return;
    }
    if (i < containers.size()) {
        // Take this one if it wouldn't make us exceed 150 liters, and proceed
        // to the next container:
        if (left >= containers[i])
            search(containers, result, used + 1, left - containers[i], i + 1);

        // Don't take this one; proceed to the next container:
        search(containers, result, used, left, i + 1);
    }
}

void run(std::string_view buf)
{
    small_vector<int> containers;
    find_numbers<int>(buf, containers);
    ASSERT(containers.size() < 64);

    // Sort the containers by decreasing capacity to be able to prune unviable
    // branches in search() quicker -- greedily taking larger containers first
    // will reach or exceed 150 liters of eggnog faster, after which there is
    // no point in looking any deeper.
    std::ranges::sort(containers, λab(a > b));

    small_vector<int> result(containers.size());
    search(containers, result);
    fmt::print("{}\n", std::ranges::fold_left(result, 0, λab(a + b)));
    fmt::print("{}\n", *std::ranges::find_if(result, λa(a != 0)));
}

}
