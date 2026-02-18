#include "common.h"
#include "thread_pool.h"
#include <hwy/highway.h>

namespace aoc_2017_15 {

using D = hn::FixedTag<uint64_t, 4>; // TODO: Make this scalable
constexpr D d;

/// Compute x mod (2³¹ - 1) for x < 2^62.
constexpr uint32_t mod_2p31m1_bounded(uint64_t x)
{
    // Compute the remainder with bit twiddling. This exploits the fact that
    // the `x` is at most 61 bits here and will only need at most 2 iterations
    // here to be completely reduced.
    x = (x & 0x7fffffff) + (x >> 31);
    x = (x & 0x7fffffff) + (x >> 31);
    return x;
}

/// Compute x mod (2³¹ - 1) for packed 64-bit unsigned integers less than 2^62.
static hn::Vec<D> mod_2p31m1_bounded_epu64(hn::Vec<D> x)
{
    // This uses the same trick as the scalar version above.
    const hn::Vec<D> mask = hn::Set(d, 0x7fffffff);
    x = (x & mask) + hn::ShiftRight<31>(x);
    x = (x & mask) + hn::ShiftRight<31>(x);
    return x;
}

/// Compute (a * b^n) mod (2³¹ - 1). For this problem, it allows for skipping
/// ahead a generator by an arbitrary amount of steps.
constexpr uint32_t modexp_2p31m1_bounded(uint64_t a, uint64_t b, uint64_t n)
{
    uint64_t exp = 1;
    for (; n; n >>= 1) {
        if (n & 1)
            exp = mod_2p31m1_bounded(exp * b);
        b = mod_2p31m1_bounded(b * b);
    }
    return mod_2p31m1_bounded(a * exp);
}

// Advance the state of a generator by multiplying each 64-bit integer by
// `factor` and reducing modulo 0x7fffffff.
static hn::Vec<D> advance(hn::Vec<D> n, uint64_t factor)
{
    constexpr hn::RepartitionToNarrow<D> d_half;
    hn::Vec<D> product = hn::MulEven(hn::BitCast(d_half, n), hn::Set(d_half, factor));
    return mod_2p31m1_bounded_epu64(product);
}

static int part1(uint32_t a, uint32_t b, size_t limit = 40'000'000)
{
    std::atomic<int> result = 0;

    // Constant factors for skipping ahead in the output stream by the number
    // of SIMD lanes used at a time for the two generators.
    const uint32_t a_skip_factor = modexp_2p31m1_bounded(1, 16807, hn::Lanes(d));
    const uint32_t b_skip_factor = modexp_2p31m1_bounded(1, 48271, hn::Lanes(d));

    ThreadPool::get().for_each_index(0, limit, [&](size_t start, size_t end) noexcept {
        HWY_ALIGN_MAX std::array<uint64_t, hn::MaxLanes(d)> a_init, b_init;
        for (size_t i = 0; i < std::size(a_init); ++i) {
            a_init[i] = modexp_2p31m1_bounded(a, 16807, start + i);
            b_init[i] = modexp_2p31m1_bounded(b, 48271, start + i);
        }

        const hn::Vec<D> mask = hn::Set(d, 0xffff);
        hn::Vec<D> count = hn::Zero(d);
        hn::Vec<D> va = hn::Load(d, a_init.data());
        hn::Vec<D> vb = hn::Load(d, b_init.data());

        // Check and advance as many states as we have SIMD lanes at a time.
        size_t i = start;
        for (; i + hn::Lanes(d) <= end; i += hn::Lanes(d)) {
            const hn::Mask<D> eq = hn::Eq(va & mask, vb & mask);
            count = hn::MaskedAddOr(count, eq, count, hn::Set(d, 1));
            va = advance(va, a_skip_factor);
            vb = advance(vb, b_skip_factor);
        }

        if (i != end) {
            const hn::Mask<D> eq = hn::Eq(va & mask, vb & mask);
            count -= hn::VecFromMask(hn::And(eq, hn::FirstN(d, end - i)));
        }

        result.fetch_add(hn::ReduceSum(d, count), std::memory_order_relaxed);
    });

    return result.load();
}

static int part2(uint32_t a, uint32_t b)
{
    ThreadPool &pool = ThreadPool::get();
    constexpr size_t limit = 5'000'000;
    const size_t n_threads = pool.num_threads();
    ASSERT(limit % n_threads == 0);

    // +20% to handle extra elements
    const int thread_buffer_size = ((limit * 12 / 10) / n_threads + 7) & ~8;

    const auto buffer_size = (thread_buffer_size + 3) * n_threads;
    auto a_buffer = std::make_unique_for_overwrite<uint64_t[]>(buffer_size);
    auto b_buffer = std::make_unique_for_overwrite<uint64_t[]>(buffer_size);

    std::vector<std::span<uint64_t>> a_spans(n_threads);
    for (size_t i = 0; i < n_threads; ++i)
        a_spans[i] = std::span(a_buffer.get() + i * (thread_buffer_size + 3),
                               thread_buffer_size + 3);

    std::vector<std::span<uint64_t>> b_spans(n_threads);
    for (size_t i = 0; i < n_threads; ++i)
        b_spans[i] = std::span(b_buffer.get() + i * (thread_buffer_size + 3),
                               thread_buffer_size + 3);

    auto search = [&](std::span<uint64_t> &buffer, uint64_t init, uint64_t k,
                      uint64_t mask, uint64_t begin, uint64_t steps) noexcept {
        ASSERT(steps % hn::Lanes(d) == 0);

        const uint64_t skip = modexp_2p31m1_bounded(1, k, hn::Lanes(d));

        HWY_ALIGN_MAX std::array<uint64_t, hn::MaxLanes(d)> init_state;
        for (size_t i = 0; i < std::size(init_state); ++i)
            init_state[i] = modexp_2p31m1_bounded(init, k, begin + i);
        hn::Vec<D> state = hn::Load(d, init_state.data());

        size_t n = 0;
        for (size_t i = 0; i < steps; i += hn::Lanes(d)) {
            // NB: Use Compress() + StoreU() here instead of CompressStore();
            // vpcompressq with a memory operand as a destination is abysmally
            // slow on Zen 4. uops.info claims 60 μops (wtf) compared to just 2
            // with a register operand.
            const hn::Mask<D> zero_mask = hn::Eq(state & hn::Set(d, mask), hn::Zero(d));
            hn::StoreU(hn::Compress(state, zero_mask), d, &buffer[n]);
            n += hn::CountTrue(d, zero_mask);
            state = advance(state, skip);
        }

        buffer = buffer.first(n);
    };

    pool.for_each_thread([=, &a_spans, &b_spans](size_t thread_id) noexcept {
        const auto chunk_size_4 = 4 * limit / n_threads * 102 / 100;
        const auto chunk_size_8 = 8 * limit / n_threads * 102 / 100;
        search(a_spans[thread_id], a, 16807, 3, thread_id * chunk_size_4, chunk_size_4);
        search(b_spans[thread_id], b, 48271, 7, thread_id * chunk_size_8, chunk_size_8);
    });

    size_t counted = 0;
    int result = 0;
    for (size_t ai = 0, bi = 0; ai < a_spans.size() && bi < b_spans.size();) {
        std::span<uint64_t> &a = a_spans[ai];
        std::span<uint64_t> &b = b_spans[bi];

        auto n = std::min({a.size(), b.size(), limit - counted});
        for (size_t i = 0; i < n; ++i)
            result += (a[i] & 0xffff) == (b[i] & 0xffff);
        counted += n;
        if (counted == limit)
            break;

        a = a.subspan(n);
        b = b.subspan(n);
        if (a.empty())
            ++ai;
        if (b.empty())
            ++bi;
    }

    return result;
}

void run(std::string_view buf)
{
    auto [a, b] = find_numbers_n<int, 2>(buf);
    fmt::print("{}\n", part1(a, b));
    fmt::print("{}\n", part2(a, b));
}

}
