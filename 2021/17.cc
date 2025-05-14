#include "common.h"
#include "dense_set.h"
#include "inplace_vector.h"

namespace aoc_2021_17 {

/// Horizontal max of the 16-bit integers in `v`.
static int16_t hmax_epi16(const __m256i v)
{
    // Max of 32-bit blocks:
    const __m256i vlo = _mm256_unpacklo_epi16(v, v);
    const __m256i vhi = _mm256_unpackhi_epi16(v, v);
    const __m256i m32 = _mm256_max_epi16(vlo, vhi);
    const __m256i m32t = _mm256_shuffle_epi32(m32, 0b10'11'00'01);

    // Max of 64-bit blocks:
    const __m256i m64 = _mm256_max_epi16(m32, m32t);
    const __m256d m64d = _mm256_castsi256_pd(m64);
    const __m256i m64t = _mm256_castpd_si256(_mm256_shuffle_pd(m64d, m64d, 0b1100));

    // Max within 128-bit lane:
    const __m256i m128 = _mm256_max_epi16(m64, m64t);

    // Max of both 128-bit lanes:
    return std::max(_mm256_extract_epi16(m128, 0), _mm256_extract_epi16(m128, 8));
}

/// Run the simulation for the 16 velocities given by `vx` and `vy` for which
/// the corresponding element in active_mask is set (-1), with the target area
/// given by the rectangle (x0, y0), (x1, y1) inclusive.
static int simulate16(__m256i vx,
                      __m256i vy,
                      __m256i active_mask,
                      const int16_t x0,
                      const int16_t x1,
                      const int16_t y0,
                      const int16_t y1,
                      dense_set<Vec2i16> &valid_velocities)
{
    const __m256i orig_vx = vx;
    const __m256i orig_vy = vy;

    __m256i x = _mm256_setzero_si256();
    __m256i y = _mm256_setzero_si256();
    __m256i reached_mask = _mm256_setzero_si256();

    // Target area bounds:
    const __m256i targetl = _mm256_set1_epi16(x0 - 1);
    const __m256i targetr = _mm256_set1_epi16(x1 + 1);
    const __m256i targetd = _mm256_set1_epi16(y0 - 1);
    const __m256i targetu = _mm256_set1_epi16(y1 + 1);

    __m256i ymax = _mm256_set1_epi16(INT16_MIN);
    while (true) {
        // Figure out which probes are below the target area. We assume the
        // target area to be below y=0, so at this point they are moving
        // downwards below it and will *never* intersect it (again):
        const __m256i below_target_mask = _mm256_cmpgt_epi16(targetd, y);

        // Figure out which probes are in the target area:
        const __m256i pgtx0 = _mm256_cmpgt_epi16(x, targetl);
        const __m256i pltx1 = _mm256_cmpgt_epi16(targetr, x);
        const __m256i pgty0 = _mm256_cmpgt_epi16(y, targetd);
        const __m256i plty1 = _mm256_cmpgt_epi16(targetu, y);
        const __m256i in_target_x = _mm256_and_si256(pgtx0, pltx1);
        const __m256i in_target_y = _mm256_and_si256(pgty0, plty1);
        const __m256i in_target_area = _mm256_and_si256(in_target_x, in_target_y);

        // Update the mask signifying which probes that have reached the target
        // area for this simulation.
        reached_mask = _mm256_or_si256(in_target_area, reached_mask);

        // Deactivate the probes that are inside or below the target area.
        const __m256i disable = _mm256_or_si256(below_target_mask, in_target_area);
        active_mask = _mm256_andnot_si256(disable, active_mask);

        // If no probe is active, we are done.
        if (_mm256_testz_si256(active_mask, active_mask))
            break;

        // Simulate one time step.
        x = _mm256_add_epi16(x, vx);
        y = _mm256_add_epi16(y, vy);
        vx = _mm256_max_epi16(_mm256_sub_epi16(vx, _mm256_set1_epi16(1)),
                              _mm256_setzero_si256());
        vy = _mm256_sub_epi16(vy, _mm256_set1_epi16(1));
        ymax = _mm256_max_epi16(ymax, y);
    }

    // Discard the max Y coordinate for the probes that did not reach the
    // target area.
    ymax = _mm256_blendv_epi8(_mm256_set1_epi16(INT16_MIN), ymax, reached_mask);

    // Interleave the 16 separate X/Y values to an array of 2D vectors:
    std::array<Vec2i16, 16> v;
    _mm256_storeu_si256((__m256i *)&v[0], _mm256_unpacklo_epi16(orig_vx, orig_vy));
    _mm256_storeu_si256((__m256i *)&v[8], _mm256_unpackhi_epi16(orig_vx, orig_vy));

    const uint32_t reached_mask_u32 = _mm256_movemask_epi8(reached_mask);
    inplace_vector<Vec2i16, 16> result;
    for (auto m = reached_mask_u32; m; m &= m - 1, m &= m - 1)
        valid_velocities.insert(v[std::countr_zero(m) / 2]);

    return hmax_epi16(ymax);
}

/// Determine if a probe with the initial X velocity `v` can ever be inside the
/// target area.
constexpr bool vx_can_reach_target_area(int v, const int x0, const int x1)
{
    for (int x = 0;; x += v, v--)
        if (x + v >= x0)
            return x + v <= x1;
}

void run(std::string_view buf)
{
    auto [x0, x1, y0, y1] = find_numbers_n<int16_t, 4>(buf);

    dense_set<Vec2i16> s;
    s.reserve(4096);

    const __m256i iota16 =
        _mm256_setr_epi16(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

    constexpr uint16_t mask_window[] = {
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    };

    // The lower bound is the X velocity below which probes never reach the
    // target area before the velocity drops to zero.
    int ymax = y1;
    for (int vx = std::floor(std::sqrt(8 * x0) / 2); vx < x0; ++vx) {
        // Skip full simulation of the velocities for which the target area is
        // skipped completely in the X axis:
        if (!vx_can_reach_target_area(vx, x0, x1))
            continue;

        const __m256i vxx = _mm256_set1_epi16(vx);
        for (int vy = y0; vy <= -y0; vy += 16) {
            const __m256i vyy = _mm256_add_epi16(_mm256_set1_epi16(vy), iota16);
            const size_t n = -y0 - vy + 1;
            const __m256i active_mask =
                _mm256_loadu_si256((const __m256i *)&mask_window[16 - std::min(n, 16zu)]);
            ymax = std::max(ymax, simulate16(vxx, vyy, active_mask, x0, x1, y0, y1, s));
        }
    }

    fmt::print("{}\n", ymax);

    // Velocities that match the target area will cause the probe to enter the
    // target area within one time step, by definition; count those separately:
    fmt::print("{}\n", s.size() + (y1 - y0 + 1) * (x1 - x0 + 1));
}

}
