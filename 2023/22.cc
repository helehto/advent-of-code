#include "common.h"
#include "dense_map.h"
#include "dense_set.h"
#include <algorithm>
#include <ranges>

namespace aoc_2023_22 {

struct Brick {
    uint16_t x0;
    uint16_t y0;
    uint16_t z0;
    uint16_t x1;
    uint16_t y1;
    uint16_t z1;
    std::vector<Brick *> resting_on;
    std::vector<Brick *> supporting;
};

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<Brick> bricks;
    bricks.reserve(lines.size());

    uint16_t max_z = 0;
    for (std::string_view line : lines) {
        auto [x0, y0, z0, x1, y1, z1] = find_numbers_n<int, 6>(line);
        bricks.emplace_back(x0, y0, z0, x1, y1, z1);
        max_z = std::max<uint16_t>(max_z, z1);
    }
    std::ranges::sort(bricks, {}, λa(a.z0));

    std::vector<std::vector<Brick *>> bricks_by_z(max_z + 1);
    for (auto &b : bricks) {
        for (uint16_t z = b.z0; z <= b.z1; z++)
            bricks_by_z[z].push_back(&b);
    }

    for (auto &b : bricks) {
        while (b.z0 > 1) {
            std::vector<Brick *> supports;

            for (auto *other : bricks_by_z[b.z0 - 1]) {
                if ((b.x1 >= other->x0 && other->x1 >= b.x0) &&
                    (b.y1 >= other->y0 && other->y1 >= b.y0))
                    supports.push_back(other);
            }

            if (!supports.empty()) {
                b.resting_on = std::move(supports);
                for (auto *r : b.resting_on)
                    r->supporting.push_back(&b);
                break;
            }

            b.z0--;
            b.z1--;
            bricks_by_z[b.z0].push_back(&b);

            auto &zbricks = bricks_by_z[b.z1 + 1];
            if (auto it = std::ranges::find(zbricks, &b); it != zbricks.end())
                zbricks.erase(it);
        }
    }

    dense_set<uint16_t> unsafe_bricks;
    for (size_t i = 0; i < bricks.size(); ++i) {
        auto &b = bricks[i];
        for (auto *s : b.supporting) {
            if (s->resting_on.size() == 1)
                unsafe_bricks.insert(i);
        }
    }
    fmt::print("{}\n", bricks.size() - unsafe_bricks.size());

    // toposort
    std::vector<uint16_t> topo_sorted;
    {
        std::vector<std::vector<Brick *>> supporting(bricks.size());

        for (size_t i = 0; i < bricks.size(); i++) {
            supporting[i] = bricks[i].supporting;
            if (supporting[i].empty())
                topo_sorted.push_back(i);
        }

        for (size_t i = 0; i < topo_sorted.size(); i++) {
            auto &b = bricks[topo_sorted[i]];

            for (auto *s : b.resting_on) {
                auto j = s - bricks.data();
                std::erase_if(supporting[j], λx(&b == x));
                if (supporting[j].empty())
                    topo_sorted.push_back(j);
            }
        }
    }

    int64_t sum = 0;
    {
        std::vector<uint16_t> in_degrees(bricks.size());
        for (size_t i = 0; i < bricks.size(); i++)
            in_degrees[i] = bricks[i].resting_on.size();

        std::vector<uint16_t> indeg;
        indeg.reserve(in_degrees.size());

        std::vector<uint16_t> queue;

        for (uint16_t victim : topo_sorted) {
            if (!unsafe_bricks.count(victim))
                continue;

            indeg = in_degrees;
            queue = {victim};
            int n = 0;

            for (size_t i = 0; i < queue.size(); i++) {
                auto u = queue[i];
                for (auto *s : bricks[u].supporting) {
                    auto v = s - bricks.data();
                    if (--indeg[v] == 0) {
                        queue.push_back(v);
                        n++;
                    }
                }
            }

            sum += n;
        }
    }

    fmt::print("{}\n", sum);
}

}
