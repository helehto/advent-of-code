#include "common.h"
#include "dense_map.h"
#include <algorithm>
#include <variant>

namespace aoc_2023_16 {

enum { N, E, S, W };

static Point<uint8_t> step(Point<uint8_t> p, int d)
{
    static int8_t table[] = {0, 1, 0, -1};
    return p.translate(table[d], table[(d - 1) & 3]);
}

static uint16_t encode_state(Point<uint8_t> p, int dir)
{
    return p.x | static_cast<uint16_t>(p.y) << 7 | static_cast<uint16_t>(dir) << 14;
}

static void sort_unique(std::vector<uint16_t> &v)
{
    std::ranges::sort(v);
    v.erase(std::unique(v.begin(), v.end()), v.end());
}

template <typename... Spans>
inline void merge_n(std::vector<uint16_t> &output, Spans&... spans)
{
    while ((!spans.empty() && ...)) {
        uint16_t elem = std::min({spans.front()...});
        (void)((elem == spans.front() && (spans = spans.subspan(1), true)) || ...);
        if (output.empty() || output.back() != elem)
            output.push_back(elem);
    }
}

/// Merge the two sorted spans `a` and `b` into a sorted vector with unique
/// elements.
static std::vector<uint16_t> merge2_unique(std::span<const uint16_t> a,
                                           std::span<const uint16_t> b)
{
    std::vector<uint16_t> result;
    result.reserve(a.size() + b.size());
    merge_n(result, a, b);
    merge_n(result, a);
    merge_n(result, b);
    return result;
}

/// Merge the three sorted spans `a`, `b` and `c` into a sorted vector with
/// unique elements.
static std::vector<uint16_t> merge3_unique(std::span<const uint16_t> a,
                                           std::span<const uint16_t> b,
                                           std::span<const uint16_t> c)
{
    std::vector<uint16_t> result;
    result.reserve(a.size() + b.size() + c.size());
    merge_n(result, a, b, c);
    merge_n(result, a, b);
    merge_n(result, a, c);
    merge_n(result, b, c);
    merge_n(result, a);
    merge_n(result, b);
    merge_n(result, c);
    return result;
}

// This monstrosity was produced by manually converting a recursive version to
// continuation passing style and then systematically inlining everything while
// replacing all resulting tail calls with (computed) gotos. The final product
// as seen below is iterative (with explicitly stored continuations), slightly
// faster, but much harder to understand.
//
// Look at commit 0becb0a2f5fad6cfb4bc3f955e2af852ad92e908 for the last sane
// (recursive) version of this function.
static size_t fire_laser(dense_map<uint16_t, std::vector<uint16_t>> &cache,
                         const Matrix<char> &grid,
                         Point<uint8_t> p,
                         uint8_t dir)
{
    // Continuation for returning the final size of the visited set for `p`.
    struct ReturnCont {};

    // Continuation for the first result (i.e. nodes visited) of a splitter.
    struct Split1 {
        size_t original_continuation;
        std::vector<uint16_t> *result;
        Point<uint8_t> p;
        uint8_t dir;
    };

    // Continuation for the second result (i.e. nodes visited) of a splitter.
    struct Split2 {
        size_t original_continuation;
        std::vector<uint16_t> *result;
        std::vector<uint16_t> *output_from_split1;
    };

    std::vector<std::variant<ReturnCont, Split1, Split2>> continuations;
    continuations.reserve(4 * std::max(grid.rows, grid.cols));
    continuations.push_back(ReturnCont{});

    size_t continuation_index = 0;

    // These need to be in the same order as in std::variant<>.
    static const void *const continuation_labels[] = {
        &&apply_final_continuation,
        &&apply_split1_continuation,
        &&apply_split2_continuation,
    };

    auto apply_continuation = [&] {
        auto &continuation = continuations[continuation_index];
        return continuation_labels[continuation.index()];
    };

restart:
    std::vector<uint16_t> *visited = nullptr;

    // This is the main loop of the function; given a point and a direction, it
    // walks the grid until it reaches a splitter, at which point continuation
    // passing style shenanigans begin.
    {
        auto [it, inserted] = cache.try_emplace(encode_state(p, dir));
        visited = &it->second;
        if (!inserted)
            goto *apply_continuation();

        p = step(p, dir);
        for (; grid.in_bounds(p); p = step(p, dir)) {
            visited->push_back(static_cast<uint16_t>(p.y) << 8 | p.x);

            if (auto it = cache.find(encode_state(p, dir)); it != cache.end()) {
                sort_unique(*visited);
                *visited = merge2_unique(*visited, it->second);
                goto *apply_continuation();
            }

            if (grid(p) == "-|"[dir & 1]) {
                continuations.push_back(Split1{continuation_index, visited, p, dir});
                sort_unique(*visited);
                dir = (dir + 1) & 3;
                continuation_index = continuations.size() - 1;
                goto restart;
            } else if (grid(p) == '/') {
                dir ^= 0b01;
            } else if (grid(p) == '\\') {
                dir ^= 0b11;
            }
        }

        sort_unique(*visited);
        goto *apply_continuation();
    }

apply_split1_continuation:;
    {
        // Continuation for the result of the first branch of a splitter; push
        // a new continuation for the second branch which contains the nodes
        // visited so far, and keep executing.
        const auto &k = *std::get_if<Split1>(&continuations[continuation_index]);
        continuations.push_back(Split2{k.original_continuation, k.result, visited});
        p = k.p;
        dir = (k.dir - 1) & 3;
        continuation_index = continuations.size() - 1;
        goto restart;
    }

apply_split2_continuation:;
    {
        // Continuation for the result of the second branch of a splitter.
        // Here, we need to find the union of the nodes visited between the
        // original point and the splitter, and the two corresponding branches
        // of the splitter; all of these are sorted vectors by construction, so
        // we can perform a fast three-way merge.
        const auto &k = *std::get_if<Split2>(&continuations[continuation_index]);
        *k.result = merge3_unique(*k.result, *k.output_from_split1, *visited);
        continuation_index = k.original_continuation;
        visited = k.result;
        goto *apply_continuation();
    }

apply_final_continuation:
    return visited->size();
}

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto grid = Matrix<char>::from_lines(lines);
    dense_map<uint16_t, std::vector<uint16_t>> cache(grid.size());

    size_t part2 = 0;
    auto fire_from_edge = [&](uint8_t x, uint8_t y, int dir) {
        Point<uint8_t> p = step({x, y}, (dir + 2) & 3);
        return fire_laser(cache, grid, p, dir);
    };
    fmt::print("{}\n", fire_from_edge(0, 0, E));

    for (uint8_t x = 0; x < grid.cols; x++) {
        part2 = std::max(part2, fire_from_edge(x, 0, S));
        part2 = std::max(part2, fire_from_edge(x, grid.rows - 1, N));
    }
    for (uint8_t y = 0; y < grid.rows; y++) {
        part2 = std::max(part2, fire_from_edge(0, y, E));
        part2 = std::max(part2, fire_from_edge(grid.cols - 1, y, W));
    }
    fmt::print("{}\n", part2);
}

}
