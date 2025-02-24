#include "common.h"

namespace aoc_2019_22 {

/// Compute a*b mod m (without overflow).
constexpr static int64_t mulq2_mod(int64_t a, int64_t b, int64_t m)
{
    auto v = static_cast<__int128_t>(a) * b;
    return v % m;
}

/// Compute a*b*c mod m (without overflow).
constexpr static int64_t mulq3_mod(int64_t a, int64_t b, int64_t c, int64_t m)
{
    return mulq2_mod(mulq2_mod(a, b, m), c, m);
}

/// Compute a^b mod m.
constexpr static int64_t modexp(int64_t a, int64_t b, const int64_t m)
{
    int64_t result = 1;
    for (; b; b >>= 1) {
        if (b & 1)
            result = mulq2_mod(result, a, m);
        a = mulq2_mod(a, a, m);
    }
    return result;
}

/// Given the input, determine the function f(x) = ax + b mod m that generates
/// the deck after shuffling, given x = 0..N. Returns `a` and `b` as pair.
static std::pair<int64_t, int64_t> get_coefficients(std::span<std::string_view> lines,
                                                    int64_t m)
{
    int64_t a = 1;
    int64_t b = 0;
    int k = 0;

    for (std::string_view line : lines) {
        if (line.starts_with("deal with")) {
            std::from_chars(line.data() + 20, line.data() + line.size(), k);
            a = mulq2_mod(a, modinv(k, m), m);
        } else if (line.starts_with("deal into")) {
            a = m - a;
            b = (a + b) % m;
        } else if (line.starts_with("cut")) {
            std::from_chars(line.data() + 4, line.data() + line.size(), k);
            ASSERT(k != 0);
            b = (a * k + b) % m;
        }
    }

    return {a, b};
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    // Part 1: After computing `a` and `b` in f(x) = (ax + b) mod m, we are
    // looking for `x` such that (ax + b) mod m = 2019. Rearranging, we get
    // x = (a^-1 * (2019 - b)) mod m.
    {
        constexpr int64_t m = 10007;
        auto [a, b] = get_coefficients(lines, m);
        fmt::print("{}\n", (modinv(a, m) * (2019 - b)) % m);
    }

    // Part 2: If f(x) = (ax + b) mod m, we have
    //
    //     f^n(x) = (a^n x + b Sum[a^k, k=0..n-1]) mod m.
    //
    // Proof by induction - base case:
    //
    //     f(1) = (ax + b) mod m
    //          = (a^1 x + b Sum[a^k, k=0..0]) mod m.
    //
    // Inductive case:
    //
    //     f^(n+1)(x) = (af^n(x) + b) mod m
    //                = (a(a^n x + b Sum[a^k, k=0..n-1]) + b) mod m
    //                = (a^(n+1) x + b Sum[a^k, k=1..n] + b) mod m
    //                = (a^(n+1) x + b Sum[a^k, k=0..n]) mod m.
    //
    // The latter sum is a geometric sum, meaning that the (gigantic) sum and
    // hence f^n(x) can be simplified to:
    //
    //     f^n(x) = (a^n x + b Sum[a^k b, k=0..n-1]) mod m
    //            = (a^n x + b (1 - a^n) * (1 - a)^-1) mod m,
    //
    // where the modular inverse (1 - a)^-1 is guaranteed to exist since the
    // size of the deck of cards (119315717514047) is prime.

    {
        constexpr int64_t m = 119315717514047;
        constexpr int64_t n = 101741582076661;
        auto [a, b] = get_coefficients(lines, m);
        const int64_t v = modexp(a, n, m);
        fmt::print("{}\n", (v * 2020 + mulq3_mod(b, 1 - v, modinv(m + 1 - a, m), m)) % m);
    }
}

}
