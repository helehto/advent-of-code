#include "common.h"

namespace aoc_2021_24 {

// Hard-coded constants in input:
constexpr int k1[] = {1, 1, 1, 26, 1, 1, 26, 1, 26, 1, 26, 26, 26, 26};
constexpr int k2[] = {15, 14, 11, -13, 14, 15, -7, 10, -12, 15, -16, -9, -8, -8};
constexpr int k3[] = {4, 16, 14, 3, 11, 13, 11, 7, 12, 15, 13, 1, 15, 4};

// This is a rough equivalent of the compiled function, annotated:
constexpr int f(std::span<const int> input)
{
    int z = 0;

    for (size_t i = 0; i < input.size(); ++i) {
        const int w = input[i];

        // Summary: Pop a base-26 digit from z and then append w+k3[i] as a
        // base-26 digit if z % 26 + k2[i] == w.
        //
        // We unconditionally push digits when k1[i] == 1, as x will always be
        // true, so not much can be done about that. However, we can affect
        // things when k1[i] == 26, so we must ensure that a number is *never*
        // pushed in that case; otherwise, there will be a remainder in z after
        // the last iteration of the loop, so the model number will be invalid.
        //
        // In other words, this means that we want z % 26 + k2[i] to be equal
        // to w. z % 26 is the last base-26 digit that was pushed that has not
        // been popped yet. For instance, in iteration i == 3, this will be the
        // value input[2] + k3[2]. Hence, input[3] must be equal to input[2] +
        // k3[2] + k2[3].
        //
        // This logic applies to all input values for which k1[i] is 26, so we
        // have:
        //
        //    input[3] = input[2] + k3[2] + k2[3]
        //    input[6] = input[5] + k3[5] + k2[6]
        //    input[8] = input[7] + k3[7] + k2[8]
        //    input[10] = input[9] + k3[9] + k2[10]
        //    input[11] = input[4] + k3[4] + k2[11]
        //    input[12] = input[1] + k3[1] + k2[12]
        //    input[13] = input[0] + k3[0] + k2[13]
        //
        // ...which perfectly balances out to leave z = 0 when all input has
        // been consumed.

        const bool x = z % 26 + k2[i] != w;
        z /= k1[i];

        if (x) {
            // Push w + k3[i] onto z as a base-26 digit.
            z = z * 26 + (w + k3[i]);
        }
    }

    return z;
}

constexpr std::array<int, 14> solve1()
{
    constexpr int ka = k3[0] + k2[13];
    constexpr int kb = k3[1] + k2[12];
    constexpr int kc = k3[2] + k2[3];
    constexpr int kd = k3[4] + k2[11];
    constexpr int ke = k3[5] + k2[6];
    constexpr int kf = k3[7] + k2[8];
    constexpr int kg = k3[9] + k2[10];

    auto a = std::min(9, 9 - ka);
    auto b = std::min(9, 9 - kb);
    auto c = std::min(9, 9 - kc);
    auto d = std::min(9, 9 - kd);
    auto e = std::min(9, 9 - ke);
    auto f = std::min(9, 9 - kf);
    auto g = std::min(9, 9 - kg);

    return {a, b, c, c + kc, d, e, e + ke, f, f + kf, g, g + kg, d + kd, b + kb, a + ka};
}

constexpr std::array<int, 14> solve2()
{
    constexpr int ka = k3[0] + k2[13];
    constexpr int kb = k3[1] + k2[12];
    constexpr int kc = k3[2] + k2[3];
    constexpr int kd = k3[4] + k2[11];
    constexpr int ke = k3[5] + k2[6];
    constexpr int kf = k3[7] + k2[8];
    constexpr int kg = k3[9] + k2[10];

    auto a = std::max(1, 1 - ka);
    auto b = std::max(1, 1 - kb);
    auto c = std::max(1, 1 - kc);
    auto d = std::max(1, 1 - kd);
    auto e = std::max(1, 1 - ke);
    auto f = std::max(1, 1 - kf);
    auto g = std::max(1, 1 - kg);

    return {a, b, c, c + kc, d, e, e + ke, f, f + kf, g, g + kg, d + kd, b + kb, a + ka};
}

void run(std::string_view, aoc::Answer &answer)
{
    answer.add_formatted("{}", fmt::join(solve1(), ""));
    answer.add_formatted("{}", fmt::join(solve2(), ""));
}
AOC_REGISTER_SOLVER(2021, 24, run);

}
