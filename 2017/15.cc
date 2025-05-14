#include "common.h"
#include "inplace_vector.h"
#include <future>
#include <thread>

namespace aoc_2017_15 {

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
static __m256i mod_2p31m1_bounded_epu64(__m256i x)
{
    // This uses the same trick as the scalar version above.
    const __m256i mask = _mm256_set1_epi64x(0x7fffffff);
    x = _mm256_add_epi64(_mm256_and_si256(x, mask), _mm256_srli_epi64(x, 31));
    x = _mm256_add_epi64(_mm256_and_si256(x, mask), _mm256_srli_epi64(x, 31));
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
// `factor_4x64` and reducing modulo 0x7fffffff.
static __m256i advance_4x64(__m256i n_4x64, __m256i factor_4x64)
{
    return mod_2p31m1_bounded_epu64(_mm256_mul_epu32(n_4x64, factor_4x64));
}

static int part1(uint32_t a, uint32_t b)
{
    constexpr size_t limit = 40'000'000;
    constexpr int max_threads = 32;

    const auto n_threads = std::bit_floor(
        std::min<unsigned int>(std::thread::hardware_concurrency(), max_threads));

    ASSERT(limit % n_threads == 0);
    const auto stride = limit / n_threads;
    ASSERT(stride % 4 == 0);

    int result = 0;

    // Constant factors for skipping ahead in the output stream by 4 iterations
    // at a time for the two generators.
    constexpr uint32_t a_skip4 = modexp_2p31m1_bounded(1, 16807, 4);
    constexpr uint32_t b_skip4 = modexp_2p31m1_bounded(1, 48271, 4);

    auto search = [=, &result](const uint32_t start) noexcept {
        std::array<uint64_t, 4> a_init, b_init;
        for (size_t i = 0; i < 4; ++i) {
            a_init[i] = modexp_2p31m1_bounded(a, 16807, start + i);
            b_init[i] = modexp_2p31m1_bounded(b, 48271, start + i);
        }
        __m256i a_4x64 = _mm256_loadu_si256(reinterpret_cast<__m256i *>(&a_init));
        __m256i b_4x64 = _mm256_loadu_si256(reinterpret_cast<__m256i *>(&b_init));

        const __m256i mask_4x64 = _mm256_set1_epi64x(0xffff);
        __m256i count_4x64 = _mm256_setzero_si256();

        // Check and advance by 4 elements at a time. Each thread is assumed to
        // process a multiple of 4 elements, so we don't need a fallback scalar
        // loop below.
        for (size_t i = 0; i < stride; i += 4) {
            a_4x64 = advance_4x64(a_4x64, _mm256_set1_epi64x(a_skip4));
            b_4x64 = advance_4x64(b_4x64, _mm256_set1_epi64x(b_skip4));
            const __m256i a_lo16_4x64 = _mm256_and_si256(a_4x64, mask_4x64);
            const __m256i b_lo16_4x64 = _mm256_and_si256(b_4x64, mask_4x64);
            const __m256i eq_4x64 = _mm256_cmpeq_epi64(a_lo16_4x64, b_lo16_4x64);
            count_4x64 = _mm256_sub_epi64(count_4x64, eq_4x64);
        }

        std::array<uint64_t, 4> count;
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(&count), count_4x64);
        result += count[0] + count[1] + count[2] + count[3];
    };

    {
        inplace_vector<std::jthread, max_threads> threads;
        for (size_t i = 0; i < n_threads; ++i)
            threads.unchecked_emplace_back(search, i * stride);
    }

    return result;
}

/// Store the states in `n_4x64` for which the bits in `mask` are zero into an
/// output buffer, and advance the state. Returns the number of elements stored
/// into `buffer`.
static size_t step_compress(__m256i *n_4x64,
                            uint64_t *buffer,
                            const uint64_t factor,
                            const uint32_t mask)
{
    const auto mask_4x64 = _mm256_set1_epi64x(mask);
    const auto zero_4x64 = _mm256_setzero_si256();

    // Figure out which elements that fulfil the given mask.
    const auto low_bits_4x64 = _mm256_and_si256(*n_4x64, mask_4x64);
    const auto ctrl_mask_4x64 = _mm256_cmpeq_epi64(low_bits_4x64, zero_4x64);

    // Note: using vmovmskps instead of vmovmskpd here is intended despite the
    // 64-bit elements, to better fit the compressed store logic below.
    const uint64_t ctrl_mask = _mm256_movemask_ps(_mm256_castsi256_ps(ctrl_mask_4x64));

    // Pack those elements into the output buffer. This is a slight variation
    // on a technique for doing this with AVX2 + BMI2 (for pext); credits to
    // Peter Cordes:
    //
    //     https://stackoverflow.com/questions/36932240/avx2-what-is-the-most-efficient-way-to-pack-left-based-on-a-mask
    size_t result = std::popcount(ctrl_mask) / 2;
    {
        const auto expanded_mask = _pdep_u64(ctrl_mask, 0x0101010101010101) * 0xff;
        const auto wanted_indices = _pext_u64(0x0706050403020100, expanded_mask);
        const auto bytevec = _mm_cvtsi64_si128(wanted_indices);
        const auto shufmask = _mm256_cvtepu8_epi32(bytevec);
        const auto shuffled = _mm256_permutevar8x32_epi32(*n_4x64, shufmask);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer), shuffled);
    }

    // Advance the generator state for each element.
    *n_4x64 = advance_4x64(*n_4x64, _mm256_set1_epi64x(factor));

    return result;
}

static int part2(uint32_t a, uint32_t b)
{
    constexpr size_t limit = 5'000'000;

    auto a_values = std::make_unique_for_overwrite<uint64_t[]>(limit + 3);
    auto b_values = std::make_unique_for_overwrite<uint64_t[]>(limit + 3);

    auto search = [](uint64_t init, uint64_t k, uint64_t mask,
                     uint64_t *buffer) noexcept {
        const uint32_t skip = modexp_2p31m1_bounded(1, k, 4);

        __m256i state_4x64 = _mm256_setr_epi64x(
            modexp_2p31m1_bounded(init, k, 0), modexp_2p31m1_bounded(init, k, 1),
            modexp_2p31m1_bounded(init, k, 2), modexp_2p31m1_bounded(init, k, 3));

        for (size_t n = 0; n < limit;)
            n += step_compress(&state_4x64, &buffer[n], skip, mask);
    };

    // Compute the admissible values from the two generators in parallel.
    {
        std::jthread t1(search, a, 16807, 3, a_values.get());
        std::jthread t2(search, b, 48271, 7, b_values.get());
    }

    int result = 0;
    for (size_t i = 0; i < limit; ++i)
        result += (a_values[i] & 0xffff) == (b_values[i] & 0xffff);

    return result;
}

void run(std::string_view buf)
{
    auto [a, b] = find_numbers_n<int, 2>(buf);
    fmt::print("{}\n", part1(a, b));
    fmt::print("{}\n", part2(a, b));
}

}
