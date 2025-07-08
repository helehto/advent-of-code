#include "common.h"
#include "dense_map.h"

namespace aoc_2020_15 {

/// The cut-off index after which value are cached in a dense_map instead.
constexpr int32_t sparse_cutoff = 2'000'000;

static int32_t step(int32_t turn,
                    int32_t last,
                    std::vector<int32_t> &seen_dense,
                    dense_map<int32_t, int32_t> &seen_sparse)
{
    if (last < sparse_cutoff) {
        const int32_t t = std::exchange(seen_dense[last], turn - 1);
        return t >= 0 ? turn - t - 1 : 0;
    } else {
        if (auto [it, inserted] = seen_sparse.emplace(last, turn - 1); !inserted) {
            const int32_t t = std::exchange(it->second, turn - 1);
            return turn - t - 1;
        }
        return 0;
    }
}

void run(std::string_view buf)
{
    small_vector<int32_t> init;
    find_numbers(buf, init);

    std::vector<int32_t> seen_dense(sparse_cutoff, -1);
    for (size_t i = 0; i < init.size() - 1; i++)
        seen_dense[init[i]] = i + 1;

    dense_map<int32_t, int32_t> seen_sparse;
    seen_sparse.reserve(1'000'000);

    int32_t last = init.back();
    int32_t turn = init.size() + 1;
    for (; turn <= 2020; turn++)
        last = step(turn, last, seen_dense, seen_sparse);
    fmt::print("{}\n", last);

    for (; turn <= 30'000'000; turn++)
        last = step(turn, last, seen_dense, seen_sparse);
    fmt::print("{}\n", last);
}

}
