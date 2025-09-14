#include "common.h"
#include "md5.h"
#include "thread_pool.h"
#include <mutex>

namespace aoc_2016_5 {

struct alignas(8) PasswordChar {
    /// Which integer index that produced the hash that yielded this character.
    uint32_t index = UINT32_MAX;
    /// The character.
    uint32_t character = UINT32_MAX;
};
static_assert(sizeof(PasswordChar) == 8);

struct State {
    std::mutex mutex;
    std::atomic<uint32_t> part1_limit{UINT32_MAX};
    std::atomic<uint32_t> part2_mask{0};
    std::array<PasswordChar, 8> password1;
    std::array<PasswordChar, 8> password2;

    void add_part1_character(uint32_t index, uint32_t c)
    {
        std::unique_lock lk(mutex);

        // Taking the mutex issues an acquire fence, so this can be relaxed:
        if (index > part1_limit.load(std::memory_order_relaxed))
            return;

        auto it = std::ranges::find_if(password1, λa(a.index >= index));
        if (it == password1.end())
            return;

        std::move_backward(it, password1.end() - 1, password1.end());
        it->index = index;
        it->character = c;

        // Releasing the mutex issues a release fence, so this can be relaxed:
        part1_limit.store(password1.back().index, std::memory_order_relaxed);
    }

    void add_part2_character(uint32_t index, size_t char_index, uint32_t c)
    {
        if (char_index >= 8)
            return;

        std::unique_lock lk(mutex);
        if (index >= password2[char_index].index)
            return;

        password2[char_index].index = index;
        password2[char_index].character = c;

        // Releasing the mutex issues a release fence, so this can be relaxed:
        part2_mask.fetch_or(1 << char_index, std::memory_order_relaxed);
    }

    bool done(uint32_t index) const
    {
        // Using a mutex in add_part1_character() and add_part2_character()
        // above is fine since those only get called for hashes with five
        // leading hex zeroes, i.e. one in 2^20 = 1048576 hashes or so; in that
        // case contention is very rare.
        //
        // This function, however, is called for *every* iteration of the outer
        // loop in search() by *every* thread to determine when to terminate.
        // Locking the mutex here would lead to massive lock contention.
        //
        // Use an acquire fence instead, synchronizing with the implicit
        // release fence issued when the lock is released in the functions
        // above.
        std::atomic_thread_fence(std::memory_order_acquire);

        // These loads can be relaxed due to the fence above.
        bool p1_done = index >= part1_limit.load(std::memory_order_relaxed);
        bool p2_done = part2_mask.load(std::memory_order_relaxed) == 0xff;

        return p1_done && p2_done;
    }
};

static void search(State &state, std::string_view prefix, size_t start, size_t stride)
{
    md5::State md5(prefix);

    for (size_t n = 8 * start; !state.done(n); n += 8 * stride) {
        const __m256i hashes = md5.run(n).a;
        const uint32_t mask5 = md5::leading_zero_mask<5>(hashes);

        if (mask5 == 0)
            continue;

        alignas(32) std::array<uint32_t, 8> hashes_u32;
        _mm256_store_si256(reinterpret_cast<__m256i *>(&hashes_u32), hashes);

        for (uint32_t m = mask5; m; m &= m - 1) {
            const auto bit = std::countr_zero(m);
            const auto h1 = (hashes_u32[bit] >> 16) & 0xf;
            const auto h2 = (hashes_u32[bit] >> 28) & 0xf;
            state.add_part1_character(n + bit, "0123456789abcdef"[h1]);
            state.add_part2_character(n + bit, h1, "0123456789abcdef"[h2]);
        }
    }
}

void run(std::string_view buf)
{
    State state;

    ThreadPool::get().for_each_thread([&state, buf](size_t thread_id) noexcept {
        search(state, buf, thread_id, ThreadPool::get().num_threads());
    });

    for (size_t i = 0; i < 8; ++i)
        putc(state.password1[i].character, stdout);
    putc('\n', stdout);
    for (size_t i = 0; i < 8; ++i)
        putc(state.password2[i].character, stdout);
    putc('\n', stdout);
}

}
