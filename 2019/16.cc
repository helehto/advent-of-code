#include "common.h"
#include "thread_pool.h"

namespace aoc_2019_16 {

/// Return a representative of the binomial coefficient (m choose n) mod 10.
constexpr uint8_t binomial_mod_10_repr(int m, int n)
{
    // We compute the binomial coefficients modulo 10 by using Lucas' theorem
    // separately for mod 2 and mod 5, and then combine the results with the
    // Chinese remainder theorem.

    /// Compute a representative of (m choose n) mod 5.
    auto mod5_repr = [&](int64_t m, int64_t) {
        // Table of binomial coefficients for m,n < 5.
        static constexpr uint16_t mod5_table[5][5] = {
            // clang-format off
            {1, 0, 0, 0, 0},
            {1, 1, 0, 0, 0},
            {1, 2, 1, 0, 0},
            {1, 3, 3, 1, 0},
            {1, 4, 6, 4, 1},
            // clang-format on
        };

        // Map from numbers 0..124 to a triple of base 5 digits. This reduces
        // the number of expensive divisions/modulo operations needed.
        static constexpr auto mod5_digits3 = [] consteval {
            std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 125> result;
            for (uint8_t i = 0; i < 5; ++i)
                for (uint8_t j = 0; j < 5; ++j)
                    for (uint8_t k = 0; k < 5; ++k)
                        result[i * 25 + j * 5 + k] = {i, j, k};
            return result;
        }();

        // Use Lucas' theorem to compute (m choose n) mod 5 by decomposing m
        // and n into their base-5 digits.
        uint32_t result = 1;
        while (m && n) {
            auto [ma, mb, mc] = mod5_digits3[m % 125];
            auto [na, nb, nc] = mod5_digits3[n % 125];
            result *= mod5_table[ma][na];
            result *= mod5_table[mb][nb];
            result *= mod5_table[mc][nc];
            m /= 125;
            n /= 125;
        }

        return result;
    };

    const uint32_t a = (~m & n) == 0 ? 1 : 0;
    const uint32_t b = mod5_repr(m, n);
    constexpr uint32_t n1 = 5 * modinv(5, 2);
    constexpr uint32_t n2 = 2 * modinv(2, 5);

    // Use the Chinese remainder theorem hre to combine the results mod 2 and
    // mod 5. Taking this expression mod 10 would give the actual value, but
    // since we only need a representative, we skip the final reduction.
    return a * n1 + b * n2;
}

void run(std::string_view input)
{
    std::string buf(input);

    std::vector<uint8_t> v(input.size());
    for (size_t i = 0; i < buf.size(); ++i)
        v[i] = buf[i] - '0';

    const size_t n = v.size();

    // Part 1.
    {
        std::vector<uint8_t> next;
        for (size_t _ = 0; _ < 100; ++_) {
            next.resize(n);

            size_t i = 0;

            for (; i < (n + 1) / 2; ++i) {
                int val = 0;
                size_t j = i;
                while (j < n) {
                    for (size_t jj = std::min(j + (i + 1), n); j < jj; ++j)
                        val += v[j];
                    j += i + 1;
                    for (size_t jj = std::min(j + (i + 1), n); j < jj; ++j)
                        val -= v[j];
                    j += i + 1;
                }

                next[i] = std::abs(val) % 10;
            }

            // The second half of the output vector is just a suffix sum of the
            // second half of the input vector.
            next[n - 1] = v[n - 1];
            for (size_t k = n - 2; k >= i; --k)
                next[k] = (next[k + 1] + v[k]) % 10;

            std::swap(v, next);
        }

        for (size_t i = 0; i < 8; ++i)
            fputc(v[i] + '0', stdout);
        fputc('\n', stdout);
    }

    // For part 2, ignoring the reduction mod 10 for each step for a moment:
    //
    // With that simplification, the FFT transformation is completely linear
    // and can be written as a matrix A. Performing n phases is then equivalent
    // to left-multiplying the input vector by Aⁿ.
    //
    // The way the matrix is constructed leads to two observations:
    //
    // (1) A is (non-strictly) upper triangular, so Aⁿ will also be. In
    // particular, by representing A as a block matrix [A, B; 0, C], the power
    // Aⁿ is equal to [Aⁿ, X; 0, Cⁿ], where X is some horrendous expression.
    // Dividing the input vector into two blocks [v; w], the output vector is
    // then [Aⁿ v + X w; Cⁿ w]; the second block depends only on Cⁿ and w!
    //
    // (2) Shifting and elongating the pattern for each row means that C is an
    // upper-triangular matrix of all ones. This has two consequences:
    //
    //   - Cⁿ has a well-known form in terms of binomial coefficients:
    //     Cⁿ(i,j) = binomial(j-i+k-1, k-1) for j >= i, and 0 otherwise.
    //
    //   - As C and w both consist of only non-negative elements, every element
    //     in the second half of the output vector is non-decreasing with each
    //     phase, so they will always be non-negative.
    //
    // Combining these facts, we obtain an explicit formula for each element in
    // the second half of the output vector after n phases, where k is the size
    // of the vector:
    //
    //    output[i] = sum(binomial(j-i+n-1, n-1) * input[j], j=i..k-1).
    //
    // Since the second half of the vector in each intermediate step (w, C w,
    // C² w, ..., Cⁿ w) is non-negative, the modulo 10 operation can be
    // deferred to the very end, so the problem reduces to computing the above
    // formula modulo 10.
    {
        for (size_t i = 0; i < buf.size(); ++i)
            v[i] = buf[i] - '0';

        constexpr size_t expansion_factor = 10'000;
        const size_t expanded_len = expansion_factor * buf.size();

        size_t offset = 0;
        std::from_chars(&buf[0], &buf[7], offset);

        // By the comment above, we only support computing arbitrary offsets in
        // the second half of the output signal.
        ASSERT(offset >= expanded_len / 2);

        // Each relevant row in the matrix is only a shifted version of the row
        // above, so it suffices to compute one set of binomial coefficients.
        // (Note that these are representatives that are correct modulo 10, not
        // the actual values mod 10.)
        const size_t n_coeffs = expanded_len - offset;
        auto coeffs = std::make_unique_for_overwrite<uint8_t[]>(n_coeffs);

        ThreadPool &pool = ThreadPool::get();
        pool.for_each_index(0, n_coeffs, [&](size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i)
                coeffs[i] = binomial_mod_10_repr(i + 100 - 1, 100 - 1);
        });

        for (size_t i = offset; i < offset + 8; ++i) {
            int val = 0;
            for (size_t j = i, k = i % n; j < ((i + n - 1) / n) * n; ++j, ++k)
                val += coeffs[j - i] * v[k];
            for (size_t block = i / n + 1; block < expansion_factor; ++block)
                for (size_t k = 0; k < n; ++k)
                    val += coeffs[block * n + k - i] * v[k];

            fputc('0' + (val % 10), stdout);
        }
        fputc('\n', stdout);
    }
}
}
