#include "common.h"
#include "dense_map.h"

namespace aoc_2019_12 {

struct System {
    std::array<std::array<int, 3>, 4> p;
    std::array<std::array<int, 3>, 4> v;

    void step1d(size_t dim)
    {
        for (size_t i = 0; i < 4; i++) {
            for (size_t j = i + 1; j < 4; ++j) {
                const int lt = (p[i][dim] - p[j][dim]) >> 31;
                const int gt = (p[j][dim] - p[i][dim]) >> 31;
                v[i][dim] += gt - lt;
                v[j][dim] -= gt - lt;
            }
        }

        for (size_t i = 0; i < 4; ++i)
            p[i][dim] += v[i][dim];
    }

    int total_energy() const
    {
        int total = 0;
        for (size_t i = 0; i < 4; i++) {
            total += (std::abs(p[i][0]) + std::abs(p[i][1]) + std::abs(p[i][2])) *
                     (std::abs(v[i][0]) + std::abs(v[i][1]) + std::abs(v[i][2]));
        }
        return total;
    }
};

uint64_t cycle_length_1d(const System initial_state, size_t dim)
{
    System system = initial_state;
    size_t i = 0;
    do {
        system.step1d(dim);
        i++;
    } while (system.p[0][dim] != initial_state.p[0][dim] ||
             system.p[1][dim] != initial_state.p[1][dim] ||
             system.p[2][dim] != initial_state.p[2][dim] ||
             system.p[3][dim] != initial_state.p[3][dim] ||
             system.v[0][dim] != initial_state.v[0][dim] ||
             system.v[1][dim] != initial_state.v[1][dim] ||
             system.v[2][dim] != initial_state.v[2][dim] ||
             system.v[3][dim] != initial_state.v[3][dim]);
    return i;
}

void run(std::string_view buf)
{
    System initial_state{};
    for (size_t i = 0; std::string_view line : split_lines(buf)) {
        initial_state.p[i] = find_numbers_n<int, 3>(line);
        i++;
    }

    System system = initial_state;
    for (size_t i = 0; i < 1000; ++i) {
        system.step1d(0);
        system.step1d(1);
        system.step1d(2);
    }
    fmt::print("{}\n", system.total_energy());

    uint64_t len = 1;
    len = std::lcm(len, cycle_length_1d(initial_state, 0));
    len = std::lcm(len, cycle_length_1d(initial_state, 1));
    len = std::lcm(len, cycle_length_1d(initial_state, 2));
    fmt::print("{}\n", len);
}

}
