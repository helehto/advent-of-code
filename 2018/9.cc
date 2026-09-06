#include <algorithm>
#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/string.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace aoc_2018_9 {

static uint64_t play(uint32_t *buffer, uint32_t n_players, uint32_t n_marbles)
{
    std::vector<uint64_t> scores(n_players);

    // TODO: Both `head` and `tail` grow monotonically (`head` 16/23 of the
    // time in the core loop, `tail` 22/23 of the time). This could be a ring
    // buffer to avoid having to allocate an over-sized array.
    uint32_t *head = buffer;
    uint32_t *tail = buffer;

    *tail++ = 0;

    size_t i = 1;

    auto append_marble = [&](size_t k) {
        *tail++ = *head++;
        *tail++ = k;
    };
    auto insert_marble = [&](size_t k, size_t insert_offset) {
        *tail++ = k;
        std::rotate(tail - 1 - insert_offset, tail - 1, tail);
    };
    auto remove_marble = [&](size_t k) {
        scores[k % n_players] += k + tail[-8];
        std::rotate(tail - 8, tail - 7, tail);
        tail--;
    };

    while (i < 23)
        append_marble(i++);

    while (i + 23 <= n_marbles) {
        remove_marble(i++);
        insert_marble(i++, 5);
        insert_marble(i++, 4);
        insert_marble(i++, 3);
        insert_marble(i++, 2);
        insert_marble(i++, 1);
        insert_marble(i++, 0);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
        append_marble(i++);
    }

    for (; i <= n_marbles; ++i) {
        if (i % 23 == 0)
            remove_marble(i);
        else if (i % 23 <= 6)
            insert_marble(i, 6 - i % 23);
        else
            append_marble(i);
    }

    return std::ranges::max(scores);
}

void run(std::string_view buf, aoc::Answer &answer)
{
    auto [n_players, n_marbles] = find_numbers_n<uint32_t, 2>(buf);
    ASSERT(UINT64_C(100) * n_marbles <= UINT32_MAX);
    auto marbles = std::make_unique_for_overwrite<uint32_t[]>(200 * n_marbles);
    answer.add(play(marbles.get(), n_players, n_marbles));
    answer.add(play(marbles.get(), n_players, 100 * n_marbles));
}
AOC_REGISTER_SOLVER(2018, 9, run);

}
