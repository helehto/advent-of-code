#include "common.h"
#include "dense_map.h"
#include <map>

namespace aoc_2019_18 {

struct alignas(8) State {
    std::array<uint8_t, 4> pos{};
    uint32_t keys = 0;

    bool operator==(const State &other) const
    {
        return std::bit_cast<uint64_t>(*this) == std::bit_cast<uint64_t>(other);
    }

    State move_robot(int r, int new_key) const
    {
        State result = *this;
        result.pos[r] = new_key;
        result.keys |= uint32_t(1) << new_key;
        return result;
    }
};

}

template <>
struct std::hash<aoc_2019_18::State> {
    size_t operator()(const aoc_2019_18::State &state) const noexcept
    {
        return _mm_crc32_u64(std::bit_cast<uint32_t>(state.pos), state.keys);
    }
};

namespace aoc_2019_18 {

template <typename T>
struct MonotonicBucketQueue {
    std::vector<std::vector<T>> buckets;
    uint32_t curr = 0;
    uint32_t mask;

    MonotonicBucketQueue() = default;

    MonotonicBucketQueue(uint32_t max_weight)
        : mask(std::bit_ceil(max_weight + 1) - 1)
    {
        buckets.resize(mask + 1);
    }

    uint32_t d() const { return curr; }

    template <typename... Args>
    void emplace(const uint32_t new_d, Args &&...args)
    {
        DEBUG_ASSERT(new_d >= curr);
        DEBUG_ASSERT(new_d <= curr + mask);
        buckets[new_d & mask].emplace_back(std::forward<Args>(args)...);
    }

    std::optional<T> pop()
    {
        for (size_t end = curr + mask + 1; curr < end; ++curr) {
            if (std::vector<T> &bucket = buckets[curr & mask]; !bucket.empty()) {
                T state(std::move(bucket.back()));
                bucket.pop_back();
                return state;
            }
        }
        return std::nullopt;
    }

    void reset()
    {
        for (std::vector<T> &bucket : buckets)
            bucket.clear();
        curr = 0;
    }

    void reset(size_t new_max_weight)
    {
        for (std::vector<T> &bucket : buckets)
            bucket.clear();
        curr = 0;
        mask = std::bit_ceil(new_max_weight + 1) - 1;
        buckets.resize(mask + 1);
    }
};

struct KeyInfo {
    uint32_t d;
    uint32_t door_mask;
};

template <typename Sink>
static void flood(const Matrix<char> &grid,
                  const Vec2i start,
                  MonotonicBucketQueue<std::tuple<Vec2i, uint32_t>> &queue,
                  Matrix<bool> &visited,
                  Sink &&sink)
{
    std::ranges::fill(visited.all(), false);
    queue.reset();
    queue.emplace(0, start, 0);

    while (auto state = queue.pop()) {
        auto [u, doors] = *state;
        visited(u) = true;
        const char c = grid(u);

        if (c >= 'A' && c <= 'Z')
            doors |= 1 << (c - 'A');
        else if (c >= 'a' && c <= 'z')
            sink(c - 'a', queue.d(), doors);

        for (auto v : neighbors4(grid, u))
            if (grid(v) != '#' && !visited(v))
                queue.emplace(queue.d() + 1, v, doors);
    }
}

/// Precompute the shortest path between all keys and which doors that are
/// between them.
static std::pair<Matrix<KeyInfo>, size_t>
precompute_key_distance_matrix(const Matrix<char> &grid, std::span<const Vec2i> keys)
{
    size_t max_key_distance = 0;
    Matrix<KeyInfo> result(keys.size(), keys.size(), KeyInfo{UINT32_MAX, UINT32_MAX});
    MonotonicBucketQueue<std::tuple<Vec2i, uint32_t>> queue(1);
    Matrix<bool> visited(grid.rows, grid.cols);

    for (size_t i = 0; i < keys.size(); i++) {
        flood(grid, keys[i], queue, visited, [&](size_t j, uint32_t d, uint32_t doors) {
            result(i, j) = KeyInfo{d, doors};
            result(j, i) = KeyInfo{d, doors};
            max_key_distance = std::max<size_t>(max_key_distance, d);
        });
    }

    return std::pair(std::move(result), max_key_distance);
}

void run(std::string_view buf)
{
    auto grid = Matrix<char>::from_lines(split_lines(buf));

    Vec2i start{};
    int num_keys = 0;
    for (auto p : grid.ndindex<int>()) {
        if (grid(p) >= 'a' && grid(p) <= 'z')
            num_keys++;
        else if (grid(p) == '@')
            start = p;
    }

    std::vector<Vec2i> keys(num_keys);
    for (auto p : grid.ndindex<int>())
        if (grid(p) >= 'a' && grid(p) <= 'z')
            keys[grid(p) - 'a'] = p;

    MonotonicBucketQueue<State> bq;
    dense_map<State, uint32_t> dist;
    dist.reserve(100'000);

    auto search = [&](std::span<const Vec2i> start, int num_robots) {
        const auto [key_info, max_key_distance] =
            precompute_key_distance_matrix(grid, keys);

        bq.reset(max_key_distance);
        dist.clear();

        const State init_state{.pos = {0xff, 0xff, 0xff, 0xff}, .keys = 0};
        bq.emplace(0, init_state);
        dist[init_state] = 0;

        auto expand = [&](const uint32_t d, const State prev, const State state) {
            if (const auto it = dist.emplace(state, UINT32_MAX).first; d < it->second) {
                it->second = d;
                bq.emplace(d, state);
            }
        };

        std::vector<std::vector<std::tuple<int, int, uint32_t>>> reachable_from_start(
            num_keys);
        {
            MonotonicBucketQueue<std::tuple<Vec2i, uint32_t>> queue(1);
            Matrix<bool> visited(grid.rows, grid.cols);
            for (int r = 0; r < num_robots; r++) {
                flood(grid, start[r], queue, visited,
                      [&](size_t j, uint32_t d, uint32_t doors) {
                          reachable_from_start[r].emplace_back(j, d, doors);
                      });
            }
        }

        while (std::optional<State> state = bq.pop()) {
            const auto [pos, keys] = *state;

            if (std::popcount(keys) == num_keys)
                return bq.d();

            for (int r = 0; r < num_robots; ++r) {
                if (pos[r] == 0xff) {
                    for (auto &[k, d, doors] : reachable_from_start[r]) {
                        if ((~keys & doors) == 0)
                            expand(bq.d() + d, *state, state->move_robot(r, k));
                    }
                } else {
                    const uint32_t all_keys_mask = (uint32_t(1) << num_keys) - 1;
                    for (uint32_t km = all_keys_mask & ~keys; km; km &= km - 1) {
                        const uint32_t k = std::countr_zero(km);
                        const auto &[d, doors] = key_info(pos[r], k);
                        if ((~keys & doors) == 0)
                            expand(bq.d() + d, *state, state->move_robot(r, k));
                    }
                }
            }
        }

        ASSERT_MSG(false, "No solution found!?");
    };

    const Vec2i start1[] = {start};
    fmt::print("{}\n", search(start1, 1));

    grid(start) = '#';
    grid(start + Vec2i{-1, 0}) = '#';
    grid(start + Vec2i{+1, 0}) = '#';
    grid(start + Vec2i{0, -1}) = '#';
    grid(start + Vec2i{0, +1}) = '#';

    const Vec2i start2[] = {
        start + Vec2i{-1, -1},
        start + Vec2i{-1, +1},
        start + Vec2i{+1, +1},
        start + Vec2i{+1, -1},
    };
    fmt::print("{}\n", search(start2, 4));
}

}
