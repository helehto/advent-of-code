#include "common.h"
#include "inplace_vector.h"

namespace aoc_2017_17 {

static int part1(const int n)
{
    inplace_vector<uint16_t, 2019> value{0};
    inplace_vector<uint16_t, 2019> prev{0};
    inplace_vector<uint16_t, 2019> next{0};
    size_t i = 0;

    for (size_t k = 1; k <= 2017; ++k) {
        auto steps = n % value.size();
        if (steps < value.size()) {
            for (size_t j = 0; j < steps; ++j)
                i = next[i];
        } else {
            for (size_t j = 0; j < value.size() - steps; ++j)
                i = prev[i];
        }

        uint16_t j = value.size();
        value.push_back(k);
        prev.push_back(i);
        next.push_back(next[i]);
        prev[next[i]] = j;
        next[i] = j;
        i = j;
    }
    return value[next[i]];
}

static int part2(const int stride)
{
    int index = 0;
    int size = 1;

    // Since we are only interested in the element after 0, and a buffer
    // containing only 0 is the initial state of the spinlock, we don't
    // actually need to simulate the full spinlock here as with part 1; only
    // the size and current index are relevant.
    //
    // In addition, instead of simulating a single step at a time, the main
    // loop below proceeds in larger chunks which grow as the spinlock grows.
    //
    // The general idea is this:
    //
    //   (1) Since the spinlock is a circular buffer, it can be represented
    //   as an array such that the value 0 is always at index 0 without loss of
    //   generality.
    //
    //   (2) Using the representation in (1), any insertion of a value which
    //   ends up immediately after 0 must have wrapped around the array one or
    //   more times in the preceding steps.
    //
    //   (3) As the size of the spinlock grows, the wrap-arounds in (2) occur
    //   less and less frequently, since the stride (the puzzle input) is
    //   constant. With a spinlock of size N, the next wrap-around will occur
    //   after approximately N/stride insertions. Call this a "block".
    //
    // By (2) and (3), we can skip ahead one full block at a time and jump
    // directly next to the next wrap-around for each iteration without missing
    // any values inserted after 0. This makes the solution run in O(log N)
    // time in the maximum size of the spinlock, rather than O(N), which is
    // significant here as N is large (50 million).

    // Initial phase: while size < stride, any insertion is guaranteed to cause
    // one or more wrap-arounds. The notion of a block doesn't help much here,
    // so handle it specially.
    for (; size < stride; size++)
        index = (index + stride) % size + 1;
    size--;

    // The spinlock is now large enough that an insertion can only cause either
    // no or a single wrap-around (never multiple). Start advancing by a full
    // block at a time.
    int result = INT_MIN;
    while (true) {
        DEBUG_ASSERT(index < size); // Loop invariant

        // Number of insertions until the next block:
        const auto block_len = (size - index + stride) / stride;
        size += block_len;
        if (size >= 50'000'000)
            break;

        // Update the current index. Subtracting by `size` here is sufficient
        // to preserve the invariant on `index` since we are only dealing with
        // a single wrap-around by construction.
        index += block_len * (stride + 1) - size;
        if (index == 1)
            result = size;
    }

    return result;
}

void run(std::string_view buf)
{
    const auto [n] = find_numbers_n<size_t, 1>(buf);
    fmt::print("{}\n", part1(n));
    fmt::print("{}\n", part2(n));
}

}
