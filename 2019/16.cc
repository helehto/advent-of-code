#include "common.h"

namespace aoc_2019_16 {

constexpr int B = 2048;

static void prefix_sum_block(int16_t *p)
{
    __m256i x = _mm256_loadu_si256(reinterpret_cast<__m256i *>(p));
    x = _mm256_add_epi16(x, _mm256_slli_si256(x, 2));
    x = _mm256_add_epi16(x, _mm256_slli_si256(x, 4));
    x = _mm256_add_epi16(x, _mm256_slli_si256(x, 8));
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x);
}

static __m128i prefix_sum_accumulate(int16_t *p, __m128i s)
{
    const __m128i d = _mm_set1_epi16(p[7]);
    __m128i x = _mm_loadu_si128(reinterpret_cast<__m128i *>(p));
    x = _mm_add_epi16(s, x);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(p), x);
    return _mm_add_epi16(s, d);
}

static void prefix_sum_mod10(int16_t *a, size_t n)
{
    __m128i s = _mm_setzero_si128();
    for (size_t i = 0; i < n; i += B) {
        int16_t *p = &a[i];

        for (int i = 0; i < B; i += 16)
            prefix_sum_block(&p[i]);

        for (int i = 0; i < B; i += 8)
            s = prefix_sum_accumulate(&p[i], s);

        for (int i = 0; i < B; i++)
            p[i] %= 10;

        // Reduce the running sum modulo 10 for each block to avoid overflow.
        s = _mm_set1_epi16(_mm_extract_epi16(s, 0) % 10);
    }
}

void run(std::string_view input)
{
    std::string buf(input);

    std::vector<int16_t> v;
    v.reserve(buf.size());
    for (char c : buf)
        v.push_back(c - '0');

    constexpr size_t expansion_factor = 10'000;
    size_t expanded_len = expansion_factor * buf.size();

    size_t offset = 0;
    std::from_chars(&buf[0], &buf[7], offset);
    ASSERT(offset >= expanded_len / 2);

    std::vector<int16_t> next;
    for (size_t _ = 0; _ < 100; ++_) {
        for (size_t i = 0; i < v.size(); ++i) {
            int val = 0;
            size_t j = i;
            while (j < v.size()) {
                for (size_t jj = std::min(j + (i + 1), v.size()); j < jj; ++j)
                    val += v[j];
                j += i + 1;
                for (size_t jj = std::min(j + (i + 1), v.size()); j < jj; ++j)
                    val -= v[j];
                j += i + 1;
            }

            next.push_back(std::abs(val) % 10);
        }

        std::swap(v, next);
        next.clear();
    }

    for (size_t i = 0; i < 8; ++i)
        fputc(v[i] + '0', stdout);
    fputc('\n', stdout);

    v.clear();
    v.reserve(expanded_len);
    std::ranges::reverse(buf);
    for (char &c : buf)
        c -= '0';
    for (size_t i = 0; i < expansion_factor && v.size() < offset + B; ++i)
        v.insert(v.end(), buf.begin(), buf.end());

    offset = expanded_len - offset;

    for (size_t i = 0; i < 100; ++i)
        prefix_sum_mod10(v.data(), offset);
    for (size_t i = offset - 1; i >= offset - 8; --i)
        fputc((v[i] % 10) + '0', stdout);
    fputc('\n', stdout);
}

}
