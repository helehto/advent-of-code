#include "common.h"
#include <cassert>
#include <climits>
#include <fmt/core.h>
#include <span>

namespace {

struct Map {
    std::vector<uint8_t> array;
    int xmin = 0;
    int xmax = 0;
    int ymin = 0;
    int ymax = 0;
    int k = 0;

    uint8_t &at(Point<int> p)
    {
        return array[(xmax - xmin) * p.y + p.x - k];
    }

    void increment(Point<int> p)
    {
        if (p.x < xmin + 1 || p.x >= xmax - 1 || p.y < ymin + 1 || p.y >= ymax - 1)
            resize_to(p);
        at(p)++;
    }

    __attribute__((cold)) void resize_to(Point<int> p)
    {
        Map new_map;
        new_map.xmin = std::min(p.x - 16, xmin - 16);
        new_map.xmax = std::max(p.x + 16, xmax + 16);
        new_map.ymin = std::min(p.y - 16, ymin - 16);
        new_map.ymax = std::max(p.y + 16, ymax + 16);
        new_map.array.resize((new_map.ymax - new_map.ymin) *
                             (new_map.xmax - new_map.xmin));
        new_map.k = 
            (new_map.xmax - new_map.xmin) * new_map.ymin + new_map.xmin;

        // This is pretty slow, but it's not going to run very often.
        for (int y = ymin; y < ymax; y++) {
            for (int x = xmin; x < xmax; x++) {
                new_map.at({x, y}) = at({x, y});
            }
        }

        *this = std::move(new_map);
    }

    uint64_t neighborhood_mask(Point<int> p)
    {
        const uint8_t *r0 = &at({p.x - 1, p.y - 1});
        const uint8_t *r1 = &at({p.x - 1, p.y});
        const uint8_t *r2 = &at({p.x - 1, p.y + 1});
        return (uint64_t)r0[0] << 0 | (uint64_t)r0[1] << 8 | (uint64_t)r0[2] << 16 |
               (uint64_t)r1[0] << 24 | (uint64_t)r1[2] << 32 |
               (uint64_t)r2[0] << 40 | (uint64_t)r2[1] << 48 | (uint64_t)r2[2] << 56;
    }
};

} // namespace

constexpr uint64_t gen_mask(int x1, int x2, int x3)
{
    return UINT64_C(0xff) << (8*x1) | UINT64_C(0xff) << (8*x2) | UINT64_C(0xff) << (8*x3);
}

void run_2022_23(FILE *f)
{
    std::string s;
    std::vector<Point<int>> elves;
    int i = 0;
    while (getline(f, s)) {
        for (size_t j = 0; j < s.size(); j++) {
            if (s[j] == '#')
                elves.push_back(Point{static_cast<int>(j), i});
        }
        i++;
    }

    struct Dir {
        uint64_t mask;
        int8_t dx;
        int8_t dy;
    };

    std::array<Dir, 4> dirs = {{
        {gen_mask(0,1,2), 0, -1},
        {gen_mask(5,6,7), 0, 1},
        {gen_mask(0,3,5), -1, 0},
        {gen_mask(2,4,7), 1, 0},
    }};

    Map map;
    for (auto &elf : elves)
        map.increment(elf);

    Map new_map;
    Map proposal_map;
    std::vector<Point<int>> new_elves;
    std::vector<Point<int>> proposals(elves.size());

    for (int round = 1;; round++) {
        for (auto &p : new_elves) {
            new_map.resize_to(p);
            proposal_map.resize_to(p);
        }

        for (size_t i = 0; i < elves.size(); i++) {
            auto p = elves[i];
            proposals[i] = p;

            auto mask = map.neighborhood_mask(p);
            if (mask == 0)
                continue;

            assert(map.neighborhood_mask(p) != 0);

            for (const auto &d : dirs) {
                if ((mask & d.mask) == 0) {
                    Point<int> dest{p.x + d.dx, p.y + d.dy};
                    proposal_map.increment(dest);
                    proposals[i] = dest;
                    break;
                }
            }
        }

        for (size_t i = 0; i < elves.size(); i++) {
            Point<int> dest =
                proposal_map.at(proposals[i]) > 1 ? elves[i] : proposals[i];
            new_elves.push_back(dest);
            new_map.increment(dest);
        }

        auto f = [&](auto e) { return new_map.at(e); };
        if (std::all_of(begin(elves), end(elves), f)) {
            fmt::print("{}\n", round);
            break;
        }

        std::swap(map,new_map);
        memset(proposal_map.array.data(), 0, proposal_map.array.size());
        memset(new_map.array.data(), 0, new_map.array.size());
        std::swap(elves, new_elves);
        new_elves.clear();

        if (round == 10) {
            auto [xmin, xmax] = std::minmax_element(
                begin(elves), end(elves), [&](auto &a, auto &b) { return a.x < b.x; });
            auto [ymin, ymax] = std::minmax_element(
                begin(elves), end(elves), [&](auto &a, auto &b) { return a.y < b.y; });
            fmt::print("{}\n", ((xmax->x - xmin->x + 1) * (ymax->y - ymin->y + 1) -
                                elves.size()));
        }

        std::rotate(std::begin(dirs), std::begin(dirs) + 1, std::end(dirs));
    }
}
