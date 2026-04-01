#include "common.h"

namespace aoc_2020_15 {

void run(std::string_view buf)
{
    small_vector<uint32_t> init;
    find_numbers(buf, init);

    std::vector<uint32_t> prev(30'000'000);

    // `seen_bitset` tracks which numbers that have been seen before (i.e.
    // which indices in `prev` are valid).
    //
    // Why not use a sentinel value for this? `prev` is 120MB, far too large to
    // fit even in L3. This, combined with the random access pattern, means the
    // majority of steps to compute the next number involves an expensive load
    // from main memory.
    //
    // As the bottleneck is a loop-carried dependency which involves a lookup
    // in `prev`, storing it there would cause a long stall even for numbers
    // that have not been seen before. The next number is 0 by definition, but
    // we would still have to wait for the load to complete to know that.
    //
    // Compressing this information into a bitset and storing it separately
    // instead makes it ~3.5 MB. This fits partially in L2 and fully in L3, and
    // means that we can identify whether we are looking at a completely new
    // number, check if the next number is 0 and break the dependency chain if
    // so with only the latency of an L3 hit at worst.
    //
    // Even if we have seen the number before and _do_ need to load from `prev`
    // to compute the next number, that load can be issued speculatively and
    // run in parallel if the branch is correctly predicted, so we will likely
    // not be worse off anyway.
    std::vector<uint64_t> seen_bitset((prev.size() + 63) / 64, 0);

    // 0 is the most common individual value in the sequence (~10%). Track it
    // in a separate variable here, and handle it in a separate branch in
    // step() below, to coax the compiler into keeping it in a register, which
    // avoids a bunch of memory ops.
    uint32_t zero = 0;

    for (size_t i = 0; i < init.size(); i++) {
        if (init[i] == 0) {
            zero = i + 1;
            seen_bitset[zero / 64] |= UINT64_C(1) << (zero % 64);
        } else {
            prev[init[i]] = i + 1;
            seen_bitset[init[i] / 64] |= UINT64_C(1) << (init[i] % 64);
        }
    }

    auto test_and_mark_as_seen = [&](size_t n) {
        const size_t index = n / 64;
        const uint64_t mask = UINT64_C(1) << (n % 64);
        if (seen_bitset[index] & mask)
            return true;
        seen_bitset[index] |= mask;
        return false;
    };

    auto step = [&](uint32_t turn, uint32_t last) -> uint32_t {
        if (last == 0) {
            const uint32_t delta = turn - zero;
            zero = turn;
            return delta;
        } else if (!test_and_mark_as_seen(last)) {
            prev[last] = turn;
            return 0;
        } else {
            const uint32_t delta = turn - prev[last];
            prev[last] = turn;
            return delta;
        }
    };

    uint32_t last = init.back();
    uint32_t turn = init.size();
    for (; turn < 2020; turn++)
        last = step(turn, last);
    fmt::print("{}\n", last);

    for (; turn < 30'000'000; turn++)
        last = step(turn, last);
    fmt::print("{}\n", last);
}

}
