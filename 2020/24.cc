#include "common.h"
#include "dense_set.h"

namespace aoc_2020_24 {

constexpr Vec2i walk(std::string_view s)
{
    const char *p = s.data();
    Vec2i pos{};

    while (true) {
        if (const char c = *p++; c == 'n') {
            if (*p == 'w') {
                pos += Vec2i{0, -1};
                ++p;
            } else if (*p == 'e') {
                pos += Vec2i{1, -1};
                ++p;
            }
        } else if (c == 's') {
            if (*p == 'w') {
                pos += Vec2i{-1, 1};
                ++p;
            } else if (*p == 'e') {
                pos += Vec2i{0, 1};
                ++p;
            }
        } else if (c == 'w') {
            pos += Vec2i{-1, 0};
        } else if (c == 'e') {
            pos += Vec2i{1, 0};
        } else {
            return pos;
        }
    }
}

static void step_avx2(const int8_t *__restrict__ row_above,
                      const int8_t *__restrict__ row_input,
                      const int8_t *__restrict__ row_below,
                      int8_t *dest)
{
    const __m256i a0 = _mm256_loadu_si256((const __m256i *)row_above);
    const __m256i a1 = _mm256_loadu_si256((const __m256i *)(row_above + 1));
    const __m256i b0 = _mm256_loadu_si256((const __m256i *)(row_input - 1));
    const __m256i b1 = _mm256_loadu_si256((const __m256i *)(row_input + 1));
    const __m256i c0 = _mm256_loadu_si256((const __m256i *)(row_below - 1));
    const __m256i c1 = _mm256_loadu_si256((const __m256i *)row_below);

    const __m256i va = _mm256_add_epi8(a0, a1);
    const __m256i vb = _mm256_add_epi8(b0, b1);
    const __m256i vc = _mm256_add_epi8(c0, c1);

    const __m256i neighbors = _mm256_add_epi8(va, _mm256_add_epi8(vb, vc));
    const __m256i has1 = _mm256_cmpeq_epi8(neighbors, _mm256_set1_epi8(-1));
    const __m256i has2 = _mm256_cmpeq_epi8(neighbors, _mm256_set1_epi8(-2));
    const __m256i has1or2 = _mm256_or_si256(has1, has2);

    const __m256i black = _mm256_loadu_si256((const __m256i *)row_input);
    const __m256i result = _mm256_blendv_epi8(has2, has1or2, black);

    _mm256_storeu_si256((__m256i *)dest, result);
}

static void step(Matrix<int8_t> &out,
                 Matrix<int8_t> &g,
                 size_t &min_x,
                 size_t &max_x,
                 size_t &min_y,
                 size_t &max_y)
{
    for (size_t y = min_y - 1; y <= max_y + 1; y += 4) {
        for (size_t x = min_x - 1; x <= max_x + 1; x += 32) {
            step_avx2(&g(y - 1, x), &g(y + 0, x), &g(y + 1, x), &out(y + 0, x));
            step_avx2(&g(y + 0, x), &g(y + 1, x), &g(y + 2, x), &out(y + 1, x));
            step_avx2(&g(y + 1, x), &g(y + 2, x), &g(y + 3, x), &out(y + 2, x));
            step_avx2(&g(y + 2, x), &g(y + 3, x), &g(y + 4, x), &out(y + 3, x));
        }
    }

    size_t next_min_x = min_x;
    size_t next_min_y = min_y;
    size_t next_max_x = max_x;
    size_t next_max_y = max_y;

    // Update new min/max coordinates.
    for (size_t y = min_y - 1; y <= max_y + 1; ++y) {
        if (out(y, min_x - 1)) {
            --next_min_x;
            break;
        }
    }
    for (size_t y = min_y - 1; y <= max_y + 1; ++y) {
        if (out(y, max_x + 1)) {
            ++next_max_x;
            break;
        }
    }
    for (size_t x = min_x - 1; x <= max_x + 1; ++x) {
        if (out(min_y - 1, x)) {
            --next_min_y;
            break;
        }
    }
    for (size_t x = min_x - 1; x <= max_x + 1; ++x) {
        if (out(max_y + 1, x)) {
            ++next_max_y;
            break;
        }
    }

    std::swap(out, g);
    min_x = next_min_x;
    max_x = next_max_x;
    min_y = next_min_y;
    max_y = next_max_y;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    dense_set<Vec2i> black;
    black.reserve(lines.size());
    for (std::string_view s : lines)
        if (auto [it, inserted] = black.emplace(walk(s)); !inserted)
            black.erase(it);
    fmt::print("{}\n", black.size());

    auto min = INT_MAX, max = INT_MAX;
    for (const auto p : black) {
        min = std::min({min, p.x, p.y});
        max = std::min({max, p.x, p.y});
    }

    const size_t max_size = max - min + 200;
    Matrix<int8_t> g(max_size + 32, max_size + 32, 0);
    size_t min_x = SIZE_MAX;
    size_t min_y = SIZE_MAX;
    size_t max_x = 0;
    size_t max_y = 0;
    for (auto p : black) {
        const int offset = (max_size - (max - min)) / 2;
        auto q = p + Vec2i(offset, offset);
        g(q) = -1;
        min_x = std::min<size_t>(q.x, min_x);
        max_x = std::max<size_t>(q.y, max_x);
        min_y = std::min<size_t>(q.y, min_y);
        max_y = std::max<size_t>(q.y, max_y);
    }

    Matrix<int8_t> next(g.rows, g.cols, 0);
    for (int i = 0; i < 100; ++i)
        step(next, g, min_x, max_x, min_y, max_y);
    fmt::print("{}\n", std::ranges::count(g.all(), -1));
}

}
