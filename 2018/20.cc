#include "common.h"
#include "dense_map.h"

namespace aoc_2018_20 {

enum { N = 0b00, S = 0b01, W = 0b10, E = 0b11 };
constexpr Vec2i dir_offset[] = {{0, -1}, {0, +1}, {-1, 0}, {+1, 0}};
constexpr Vec2i start{0, 0};

static dense_map<Vec2i, uint8_t> explore(std::string_view regex)
{
    dense_map<Vec2i, uint8_t> result;
    result.reserve(10'000);

    small_vector<Vec2i> stack{start};
    Vec2i curr = stack.back();

    auto traverse = [&](int dir) {
        const Vec2i next = curr + dir_offset[dir];
        result[curr] |= 1 << dir;
        result[next] |= 1 << (dir ^ 1);
        curr = next;
    };

    for (const char c : regex) {
        switch (c) {
        case 'N':
            traverse(N);
            break;
        case 'S':
            traverse(S);
            break;
        case 'W':
            traverse(W);
            break;
        case 'E':
            traverse(E);
            break;
        case '(':
            stack.push_back(curr);
            break;
        case ')':
            curr = stack.back();
            stack.pop_back();
            break;
        case '|':
            curr = stack.back();
            break;
        default:
            ASSERT(false);
        }
    }

    return result;
}

void run(std::string_view buf)
{
    ASSERT(buf.front() == '^');
    ASSERT(buf.back() == '$');
    dense_map<Vec2i, uint8_t> open = explore(buf.substr(1, buf.size() - 2));

    dense_map<Vec2i, int> dist;
    dist.reserve(open.size());
    dist[start] = 0;

    small_vector<std::pair<int, Vec2i>> queue;
    queue.reserve(open.size());
    queue.emplace_back(0, Vec2i{0, 0});

    int long_paths = 0;

    for (size_t i = 0; i < queue.size(); ++i) {
        auto [d, u] = queue[i];
        long_paths += (d >= 1000);

        auto open_mask = open.at(u);
        for (int dir = N; dir <= E; ++dir) {
            if (open_mask & (1 << dir)) {
                auto v = u + dir_offset[dir];
                if (dist.try_emplace(v, d + 1).second)
                    queue.emplace_back(d + 1, v);
            }
        }
    }

    fmt::print("{}\n", queue.back().first);
    fmt::print("{}\n", long_paths);
}

}
