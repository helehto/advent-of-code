#include "common.h"
#include <algorithm>

namespace aoc_2018_9 {

struct Marble {
    int value;
    Marble *prev;
    Marble *next;
};

static Marble *add_marble(Marble *curr, Marble *new_marble)
{
    auto new_prev = curr->next;
    auto new_next = curr->next->next;
    new_prev->next = new_marble;
    new_next->prev = new_marble;
    new_marble->prev = new_prev;
    new_marble->next = new_next;
    return new_marble;
}

static void remove_marble(Marble *m)
{
    auto *prev = m->prev;
    auto *next = m->next;
    prev->next = next;
    next->prev = prev;
}

static int64_t play(Marble *buffer, int n_players, int n_marbles)
{
    Marble *zero = buffer++;
    zero->value = 0;
    zero->prev = zero;
    zero->next = zero;

    Marble *curr = zero;
    std::vector<int64_t> scores(n_players);
    for (int i = 1; i <= n_marbles; i++) {
        if (i % 23 != 0) {
            Marble *new_marble = buffer++;
            new_marble->value = i;
            curr = add_marble(curr, new_marble);
        } else {
            for (int i = 0; i < 6; i++)
                curr = curr->prev;
            scores[i % n_players] += i + curr->prev->value;
            remove_marble(curr->prev);
        }
    }

    return std::ranges::max(scores);
}

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto input = find_numbers<int>(lines[0]);
    const int n_players = input[0];
    const int n_marbles = input[1];

    auto marbles = std::make_unique_for_overwrite<Marble[]>(100 * n_marbles + 1);
    fmt::print("{}\n", play(marbles.get(), n_players, n_marbles));
    fmt::print("{}\n", play(marbles.get(), n_players, 100 * n_marbles));
}

}
