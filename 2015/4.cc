#include "common.h"
#include <charconv>
#include <csignal>
#include <cstring>
#include <fmt/core.h>
#include <future>
#include <thread>
#include <x86intrin.h>

static __m256i mm256_rol(__m256i a, int n)
{
    return _mm256_or_si256(_mm256_slli_epi32(a, n), _mm256_srli_epi32(a, 32 - n));
}

static __m256i mm256_not(__m256i v)
{
    return _mm256_xor_si256(v, _mm256_set1_epi32(-1));
}

// Hash eight single-block messages simultaneously. The lengths are given in
// `lengths`, and the return values is a vector containing the first 32-bit
// words of the eight resulting hashes.
static __m256i md5_block_avx2(uint8_t *messages, uint32_t *lengths)
{
    // Padding and appending message length. (There is no scatter instruction
    // in AVX2, so this has to be a scalar loop.)
    for (int i = 0; i < 8; i++) {
        // The buffer is zero-initialized and the length of each message is
        // non-decreasing, so this is all that is needed to properly pad it for
        // each iteration.
        messages[64 * i + lengths[i]] = 0x80;

        // The message is never going to be more than 65536 bits.
        messages[64 * i + 56] = (lengths[i] << 3) & 0xff;
        messages[64 * i + 57] = (lengths[i] >> 5) & 0xff;
    }

    // The original input is eight 64-byte blocks laid out one after another.
    // The actual MD5 code below expects memory to contain interleaved 4-byte
    // words from each block, so reshuffle the original input into that
    // format.
    alignas(32) uint32_t M[16 * 8];
    {
        auto *W = reinterpret_cast<const uint32_t *>(messages);
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 8; j++) {
                M[8 * i + j] = W[16 * j + i];
            }
        }
    }

    __m256i A = _mm256_set1_epi32(0x67452301);
    __m256i B = _mm256_set1_epi32(0xefcdab89);
    __m256i C = _mm256_set1_epi32(0x98badcfe);
    __m256i D = _mm256_set1_epi32(0x10325476);

#define F(b, c, d) _mm256_or_si256(_mm256_and_si256(b, c), _mm256_andnot_si256(b, d))
#define G(b, c, d) _mm256_or_si256(_mm256_and_si256(d, b), _mm256_andnot_si256(d, c))
#define H(b, c, d) _mm256_xor_si256(b, _mm256_xor_si256(c, d))
#define I(b, c, d) _mm256_xor_si256(c, _mm256_or_si256(b, mm256_not(d)))

#define QUARTER_ROUND(f, a, b, c, d, j, k, shift)                                        \
    do {                                                                                 \
        a = _mm256_add_epi32(a, f(b, c, d));                                             \
        a = _mm256_add_epi32(a, _mm256_set1_epi32(K[k]));                                \
        a = _mm256_add_epi32(a, _mm256_load_si256((const __m256i *)(&M[8 * (j)])));      \
        a = mm256_rol(a, shift);                                                         \
        a = _mm256_add_epi32(a, b);                                                      \
    } while (0)

    static constexpr uint32_t K[] = {
       0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
       0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
       0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
       0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
       0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
       0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
       0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
       0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
       0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
       0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
       0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
       0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
       0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
       0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
       0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
       0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
    };

    // Quarter-round 1 (F):
    for (int i = 0; i < 16; i += 4) {
        QUARTER_ROUND(F, A, B, C, D, i + 0, i + 0, 7);
        QUARTER_ROUND(F, D, A, B, C, i + 1, i + 1, 12);
        QUARTER_ROUND(F, C, D, A, B, i + 2, i + 2, 17);
        QUARTER_ROUND(F, B, C, D, A, i + 3, i + 3, 22);
    }

    // Quarter-round 2 (G):
    for (int i = 16; i < 32; i += 4) {
        QUARTER_ROUND(G, A, B, C, D, (5 * (i + 0) + 1) & 15, i + 0, 5);
        QUARTER_ROUND(G, D, A, B, C, (5 * (i + 1) + 1) & 15, i + 1, 9);
        QUARTER_ROUND(G, C, D, A, B, (5 * (i + 2) + 1) & 15, i + 2, 14);
        QUARTER_ROUND(G, B, C, D, A, (5 * (i + 3) + 1) & 15, i + 3, 20);
    }

    // Quarter-round 3 (H):
    for (int i = 32; i < 48; i += 4) {
        QUARTER_ROUND(H, A, B, C, D, (3 * (i + 0) + 5) & 15, i + 0, 4);
        QUARTER_ROUND(H, D, A, B, C, (3 * (i + 1) + 5) & 15, i + 1, 11);
        QUARTER_ROUND(H, C, D, A, B, (3 * (i + 2) + 5) & 15, i + 2, 16);
        QUARTER_ROUND(H, B, C, D, A, (3 * (i + 3) + 5) & 15, i + 3, 23);
    }

    // Quarter-round 4 (I):
    for (int i = 48; i < 64; i += 4) {
        QUARTER_ROUND(I, A, B, C, D, (7 * (i + 0)) & 15, i + 0, 6);
        QUARTER_ROUND(I, D, A, B, C, (7 * (i + 1)) & 15, i + 1, 10);
        QUARTER_ROUND(I, C, D, A, B, (7 * (i + 2)) & 15, i + 2, 15);
        QUARTER_ROUND(I, B, C, D, A, (7 * (i + 3)) & 15, i + 3, 21);
    }

    return _mm256_add_epi32(A, _mm256_set1_epi32(0x67452301));
}

static std::pair<int, int> hash(char *buffer, int text_offset, int n, int stride)
{
    const auto mask5 = _mm256_set1_epi32(0xf0ffff);
    const auto mask6 = _mm256_set1_epi32(0xffffff);
    const auto vzero = _mm256_set1_epi32(0);

    // Assemble the eight inputs and their lengths.
    char *p = buffer + text_offset;
    char *q = buffer + 64;
    uint32_t lengths[8];
    for (int j = 0; j < 8; j++) {
        auto r = std::to_chars(p + 64 * j, q + 64 * j, n + j * stride);
        lengths[j] = r.ptr - (p + 64 * j) + text_offset;
    }

    // Run MD5 on the eight inputs.
    const __m256i hashes = md5_block_avx2((uint8_t *)buffer, lengths);

    // Do we have 6 leading hex digits anywhere?
    const __m256i eq6 = _mm256_cmpeq_epi32(_mm256_and_si256(mask6, hashes), vzero);
    const int eqmask6 = _mm256_movemask_ps(_mm256_castsi256_ps(eq6));

    // Do we have 5 leading hex digits anywhere?
    const __m256i eq5 = _mm256_cmpeq_epi32(_mm256_and_si256(mask5, hashes), vzero);
    const int eqmask5 = _mm256_movemask_ps(_mm256_castsi256_ps(eq5));

    return std::pair(eqmask5, eqmask6);
}

static std::pair<int, int>
hash_search(std::string_view s, int n, int stride, std::atomic_int &limit)
{
    int part1 = INT_MAX;
    int part2 = INT_MAX;

    char input[512] = {};
    for (int i = 0; i < 8; i++)
        memcpy(input + 64 * i, s.data(), s.size());

    for (size_t i = 0; n < limit.load(); i++, n += 8 * stride) {
        auto [eqmask5, eqmask6] = hash(input, s.size(), n, stride);

        if (eqmask5)
            part1 = std::min(part1, n + stride * __builtin_ctz(eqmask5));

        if (eqmask6) {
            auto part2 = n + stride * __builtin_ctz(eqmask6);
            if (limit.load() >= part2)
                limit.store(part2);
            return std::pair(part1, part2);
        }
    }

    return {part1, part2};
}

void run_2015_4(FILE *f)
{
    std::string s;
    getline(f, s);

    const auto num_threads = std::thread::hardware_concurrency();
    std::vector<std::future<std::pair<int, int>>> futures;

    std::atomic_int limit = INT_MAX;
    for (size_t i = 0; i < num_threads; i++)
        futures.push_back(std::async(std::launch::async, hash_search, s, i, num_threads,
                                     std::ref(limit)));

    std::vector<std::pair<int, int>> results;
    for (auto &f : futures)
        results.push_back(f.get());

    int part1 = INT_MAX;
    int part2 = INT_MAX;
    for (auto &[p1, p2] : results) {
        part1 = std::min(part1, p1);
        part2 = std::min(part2, p2);
    }

    fmt::print("{}\n", part1);
    fmt::print("{}\n", part2);
}
