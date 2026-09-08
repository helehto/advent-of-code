#include <algorithm>
#include <aoc/base.h>
#include <aoc/inplace_vector.h>
#include <aoc/macros.h>
#include <aoc/math.h>
#include <aoc/md5.h>
#include <aoc/small_vector.h>
#include <aoc/thread_pool.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <hwy/highway.h>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aoc_2016_17 {

namespace hn = hwy::HWY_NAMESPACE;

enum {
    DOOR_U_OPEN = 1 << 0,
    DOOR_D_OPEN = 1 << 1,
    DOOR_L_OPEN = 1 << 2,
    DOOR_R_OPEN = 1 << 3,
};

constexpr uint32_t doors_from_hash(uint32_t u)
{
    uint32_t result = 0;

    if ((u & 0x00f0) >= 0x00b0)
        result |= DOOR_U_OPEN;
    if ((u & 0x000f) >= 0x000b)
        result |= DOOR_D_OPEN;
    if ((u & 0xf000) >= 0xb000)
        result |= DOOR_L_OPEN;
    if ((u & 0x0f00) >= 0x0b00)
        result |= DOOR_R_OPEN;

    return result;
}

static md5::Vec4T md5_full(std::string_view s)
{
    std::array<uint32_t, md5::max_lanes> lengths;
    lengths.fill(s.size() + 1);

    md5::Vec4T r = md5::initial_state();

    for (; s.size() >= 64; s.remove_prefix(64)) {
        auto m = md5::SequentialBlocksN<1>::splat(s.substr(0, 64));
        r = md5::hash_block(m, r);
    }

    md5::SequentialBlocksN<1> m{};
    std::optional<size_t> x80_offset;

    for (size_t i = 0; i < 4; ++i) {
        memcpy(m.data[i], s.data(), s.size());
        m.data[i][s.size()] = "UDLR"[i];
    }

    if (s.size() >= 55) {
        // This messy logic is due to adding U/D/L/R to the string, which means
        // that we might need to push the 0x80 byte into a separate block.
        if (s.size() < 63) {
            for (size_t i = 0; i < 4; ++i)
                m.data[i][s.size() + 1] = 0x80;
        } else {
            x80_offset = 0;
        }
        r = md5::hash_block(m, r);
        m = {};
    } else {
        x80_offset = s.size() + 1;
    }

    prepare_final_blocks(m, x80_offset, lengths);
    return md5::hash_block(m, r);
}

struct State {
    int dist;
    Vec2i p;
    uint32_t door_mask;
    std::string str;
};

struct WorkQueue {
    std::mutex mutex;
    small_vector<State, 64> buffer;

    bool pop(State &result)
    {
        std::unique_lock lock(mutex);
        if (buffer.empty())
            return false;
        result = std::move(buffer.back());
        buffer.pop_back();
        return true;
    }

    void push(const State &state)
    {
        std::unique_lock lock(mutex);
        buffer.push_back(state);
    }
};

static std::array<uint32_t, md5::max_lanes> to_array(const md5::VecT v)
{
    std::array<uint32_t, md5::max_lanes> result;
    hn::StoreU(v, md5::D(), result.data());
    return result;
}

static inplace_vector<State, 4>
get_neighbors(int d, Vec2i p, const std::string &str, uint32_t door_mask)
{
    auto h = to_array(hn::Get4<0>(md5_full(str)));
    inplace_vector<State, 4> result;

    if ((door_mask & DOOR_U_OPEN) && p.y > 0)
        result.emplace_back(d + 1, p + Vec2i{0, -1}, doors_from_hash(h[0]), str + 'U');
    if ((door_mask & DOOR_D_OPEN) && p.y < 3)
        result.emplace_back(d + 1, p + Vec2i{0, +1}, doors_from_hash(h[1]), str + 'D');
    if ((door_mask & DOOR_L_OPEN) && p.x > 0)
        result.emplace_back(d + 1, p + Vec2i{-1, 0}, doors_from_hash(h[2]), str + 'L');
    if ((door_mask & DOOR_R_OPEN) && p.x < 3)
        result.emplace_back(d + 1, p + Vec2i{+1, 0}, doors_from_hash(h[3]), str + 'R');

    return result;
}

void run(std::string_view buf, aoc::Answer &answer)
{
    ASSERT(buf.size() < 32);

    ThreadPool &pool = ThreadPool::get();

    std::vector<WorkQueue> all_queues(pool.num_threads());

    // Compute the first 32 bits of the MD5 hash of the input string. This is
    // ugly, but then again the MD5 implementation is not exactly focused on
    // convenience of computing single hashes...
    const uint32_t initial_door_hash = [&] {
        std::array<uint32_t, md5::max_lanes> lengths{static_cast<uint32_t>(buf.size())};
        auto messages = md5::SequentialBlocksN<1>::splat(buf);
        prepare_final_blocks(messages, lengths);
        return md5::hash_block(
            interleave(messages), md5::initial_state(),
            [](md5::Vec4T h) { return ExtractLane(hn::Get4<0>(h), 0); });
    }();

    // Push the initial state as the root task.
    all_queues[0].push(State{
        .dist = 0,
        .p = Vec2i{0, 0},
        .door_mask = doors_from_hash(initial_door_hash),
        .str = std::string(buf),
    });

    std::atomic_size_t n_thieves = 0;
    std::mutex solution_mutex;
    std::optional<std::string> shortest;
    size_t longest = 0;

    pool.for_each_thread([&](size_t thread_id) noexcept {
        WorkQueue &queue = all_queues[thread_id];
        State state;

        small_vector<size_t, 32> victim_order;
        for (size_t i = 0; i < pool.num_threads(); ++i)
            if (i != thread_id)
                victim_order.push_back(i);

        // Randomize the order in which each thief tries to steal work from the
        // other threads, to avoid a "convoy" of thieves hammering the queues of a
        // sequence of threads in a deterministic way.
        std::ranges::shuffle(victim_order, std::minstd_rand(thread_id));

        while (queue.pop(state)) {
        restart_with_new_work:
            const auto &[d, p, mask, str] = state;
            if (p == Vec2i{3, 3}) {
                std::unique_lock lock(solution_mutex);
                longest = std::max(longest, str.size() - buf.size());
                if (!shortest || str.size() < shortest->size())
                    shortest = std::move(str);
                continue;
            }

            if (auto neighbors = get_neighbors(d, p, str, mask); !neighbors.empty()) {
                state = std::move(neighbors[0]);
                for (size_t i = 1; i < neighbors.size(); ++i)
                    queue.push(std::move(neighbors[i]));
                goto restart_with_new_work;
            }
        }

        // Out of items. If there are only thieves left, we are done; otherwise,
        // try to steal something from a worker thread.
        n_thieves.fetch_add(1, std::memory_order_relaxed);
        while (n_thieves.load(std::memory_order_relaxed) != pool.num_threads()) {
            for (const size_t i : victim_order) {
                if (all_queues[i].pop(state)) {
                    n_thieves.fetch_sub(1, std::memory_order_relaxed);
                    goto restart_with_new_work;
                }
            }
        }
    });

    ASSERT_MSG(shortest.has_value(), "No path found!?");
    answer.add(shortest->substr(buf.size()));
    answer.add(longest);
}
AOC_REGISTER_SOLVER(2016, 17, run);

}
