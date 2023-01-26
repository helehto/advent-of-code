#include "common.h"
#include <charconv>
#include <csignal>
#include <cstring>
#include <fmt/core.h>
#include <future>
#include <thread>

#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"

static uint32_t rol(uint32_t a, int n)
{
    return (a << n) | (a >> (32 - n));
}

static uint32_t md5_block(unsigned char *s, uint32_t len)
{
    static constexpr int S[] = {
        7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21,
    };

    static constexpr uint32_t K[] = {
        /* K[ 0.. 3] */ 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        /* K[ 4.. 7] */ 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        /* K[ 8..11] */ 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        /* K[12..15] */ 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        /* K[16..19] */ 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        /* K[20..23] */ 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        /* K[24..27] */ 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        /* K[28..31] */ 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        /* K[32..35] */ 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        /* K[36..39] */ 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        /* K[40..43] */ 0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        /* K[44..47] */ 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        /* K[48..51] */ 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        /* K[52..55] */ 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        /* K[56..59] */ 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        /* K[60..63] */ 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
    };

    static constexpr unsigned char PADDING[64] = {0x80};
    memcpy(s + len, PADDING, 56 - len);

    // The message is never going to be more than 65 kbits.
    s[56] = (len << 3) & 0xff;
    s[57] = (len >> 5) & 0xff;

    uint32_t A = 0x67452301;
    uint32_t B = 0xefcdab89;
    uint32_t C = 0x98badcfe;
    uint32_t D = 0x10325476;

    const auto *M = reinterpret_cast<uint32_t *>(s);

#define FF(a, b, c, d, i) a = rol(a + ((b & c) | (~b & d)) + K[i] + M[i], S[i & 3]) + b
#define GG(a, b, c, d, i) a = rol(a + ((d & b) | (~d & c)) + K[i] + M[(5*i+1)&15], S[(i & 3)+4]) + b
#define HH(a, b, c, d, i) a = rol(a + (b ^ c ^ d) + K[i] + M[(3*i+5)&15], S[(i & 3)+8]) + b
#define II(a, b, c, d, i) a = rol(a + (c ^ (b | ~d)) + K[i] + M[(7*i)&15], S[(i & 3)+12]) + b

    // Quarter-round 1 (F):
    FF(A, B, C, D, 0);  FF(D, A, B, C, 1);  FF(C, D, A, B, 2);  FF(B, C, D, A, 3);
    FF(A, B, C, D, 4);  FF(D, A, B, C, 5);  FF(C, D, A, B, 6);  FF(B, C, D, A, 7);
    FF(A, B, C, D, 8);  FF(D, A, B, C, 9);  FF(C, D, A, B, 10); FF(B, C, D, A, 11);
    FF(A, B, C, D, 12); FF(D, A, B, C, 13); FF(C, D, A, B, 14); FF(B, C, D, A, 15);

    // Quarter-round 2 (G):
    GG(A, B, C, D, 16); GG(D, A, B, C, 17); GG(C, D, A, B, 18); GG(B, C, D, A, 19);
    GG(A, B, C, D, 20); GG(D, A, B, C, 21); GG(C, D, A, B, 22); GG(B, C, D, A, 23);
    GG(A, B, C, D, 24); GG(D, A, B, C, 25); GG(C, D, A, B, 26); GG(B, C, D, A, 27);
    GG(A, B, C, D, 28); GG(D, A, B, C, 29); GG(C, D, A, B, 30); GG(B, C, D, A, 31);

    // Quarter-round 3 (H):
    HH(A, B, C, D, 32); HH(D, A, B, C, 33); HH(C, D, A, B, 34); HH(B, C, D, A, 35);
    HH(A, B, C, D, 36); HH(D, A, B, C, 37); HH(C, D, A, B, 38); HH(B, C, D, A, 39);
    HH(A, B, C, D, 40); HH(D, A, B, C, 41); HH(C, D, A, B, 42); HH(B, C, D, A, 43);
    HH(A, B, C, D, 44); HH(D, A, B, C, 45); HH(C, D, A, B, 46); HH(B, C, D, A, 47);

    // Quarter-round 4 (I):
    II(A, B, C, D, 48); II(D, A, B, C, 49); II(C, D, A, B, 50); II(B, C, D, A, 51);
    II(A, B, C, D, 52); II(D, A, B, C, 53); II(C, D, A, B, 54); II(B, C, D, A, 55);
    II(A, B, C, D, 56); II(D, A, B, C, 57); II(C, D, A, B, 58); II(B, C, D, A, 59);
    II(A, B, C, D, 60); II(D, A, B, C, 61); II(C, D, A, B, 62); II(B, C, D, A, 63);

    return 0x67452301 + A;
}

static std::pair<int, int>
hash_search(std::string_view s, int n, int stride, std::atomic_int &limit)
{
    int part1 = INT_MAX;
    int part2 = INT_MAX;

    char input[64] = {};
    memcpy(input, s.data(), s.size());

    auto hash = [&](int n) {
        auto r = std::to_chars(std::begin(input) + s.size(), std::end(input), n);
        return md5_block((unsigned char *)input, r.ptr - (char*)input);
    };

    // Search for a solution until another thread finds a solution for part 2.
    for (int i = 0; (i & 16384) != 0 || n < limit.load(); i++, n += stride) {
        const uint32_t word = hash(n);
        if ((word & 0xf0ffff) == 0)
            part1 = std::min(part1, n);
        if ((word & 0xffffff) == 0) {
            // We are potentially the first to find a solution. Signal this
            // upper limit to the other threads.
            if (limit.load() >= n)
                limit.store(n);
            return std::pair(part1, n);
        }
    }

    // At this point, another thread has found a solution for part 2. Continue
    // searching until the upper limit established by that thread to ensure
    // that we find the minimum solution.
    for (const auto limit_ = limit.load(); n <= limit_; n += stride) {
        const uint32_t word = hash(n);
        if ((word & 0xf0ffff) == 0)
            part1 = std::min(part1, n);
        if ((word & 0xffffff) == 0) {
            part2 = n;
            break;
        }
    }

    return std::pair(part1, part2);
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
