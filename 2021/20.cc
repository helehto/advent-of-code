#include "common.h"
#include <hwy/aligned_allocator.h>

namespace aoc_2021_20 {

void run(std::string_view buf, aoc::Answer &answer)
{
    HWY_ALIGN_MAX std::array<int8_t, 512> pattern;
    auto lines = split_lines(buf);
    for (size_t i = 0; buf[i] != '\n'; ++i)
        pattern[i] = (buf[i] == '#') ? -1 : 0;

    auto image_lines = std::span(lines).subspan(2);
    constexpr int max_iterations = 50;

    const size_t init_rows = image_lines.size();
    const size_t init_cols = image_lines[0].size();

    // Sized to fit all iterations (the bounding box grows by 1 pixel in each
    // direction per iteration) and to allow for reading the 3x3 kernel without
    // bounds checking in the final iteration.
    const size_t rows = init_rows + 2 * (max_iterations + 1);
    const size_t cols = (init_cols + 2 * (max_iterations + 1) + 2 * 64) & ~63; // +SIMD

    // Bounding box for the current image. NOTE: x1/y1 is inclusive!
    size_t x0 = max_iterations + 1;
    size_t x1 = x0 + init_cols - 1;
    size_t y0 = max_iterations + 1;
    size_t y1 = y0 + init_rows - 1;

    auto storage = hwy::MakeUniqueAlignedArray<int8_t>(2 * rows * cols);
    MatrixView<int8_t> image(storage.get(), rows, cols);
    MatrixView<int8_t> new_image(storage.get() + rows * cols, rows, cols);

    for (size_t y = y0; y <= y1; ++y)
        for (size_t x = x0; x <= x1; ++x)
            image(y, x) = (image_lines[y - y0][x - x0] == '#') ? -1 : 0;

    using D = hn::ScalableTag<int8_t>;
    constexpr D d;
    using D16 = hn::ScalableTag<int16_t>;
    constexpr D16 d16;

    bool outside_is_lit = false;
    for (size_t n = 0; n < max_iterations; ++n, outside_is_lit = !outside_is_lit) {
        const int8_t pad = pattern[0] && !outside_is_lit ? -1 : 0;

        // Fill in the interior of the new image.
        for (size_t y = y0 - 1; y <= y1 + 1; ++y) {
            // NB: the matrix is sized so that we can deliberately over-read
            // past the current bounding box, meaning that we don't need a
            // scalar loop to handle the tail.
            for (size_t x = x0 - 1; x <= x1 + 1; x += hn::Lanes(d16)) {
                const auto *HWY_RESTRICT p = &image(y, x);
                const hn::Vec<D> ul8 = hn::LoadU(d, p - cols - 1);
                const hn::Vec<D> uc8 = hn::LoadU(d, p - cols + 0);
                const hn::Vec<D> ur8 = hn::LoadU(d, p - cols + 1);
                const hn::Vec<D> cl8 = hn::LoadU(d, p - 1);
                const hn::Vec<D> cc8 = hn::LoadU(d, p + 0);
                const hn::Vec<D> cr8 = hn::LoadU(d, p + 1);
                const hn::Vec<D> ll8 = hn::LoadU(d, p + cols - 1);
                const hn::Vec<D> lc8 = hn::LoadU(d, p + cols + 0);
                const hn::Vec<D> lr8 = hn::LoadU(d, p + cols + 1);

                // Since the table is 512 bits we need 9 bits to encode the
                // index; sign-extend to 16 bits to handle bit 8 (upper left
                // pixel of the 3x3 neighborhood).
                const hn::Vec<D16> ul16 = hn::PromoteLowerTo(d16, ul8);
                const hn::Vec<D16> uc16 = hn::PromoteLowerTo(d16, uc8);
                const hn::Vec<D16> ur16 = hn::PromoteLowerTo(d16, ur8);
                const hn::Vec<D16> cl16 = hn::PromoteLowerTo(d16, cl8);
                const hn::Vec<D16> cc16 = hn::PromoteLowerTo(d16, cc8);
                const hn::Vec<D16> cr16 = hn::PromoteLowerTo(d16, cr8);
                const hn::Vec<D16> ll16 = hn::PromoteLowerTo(d16, ll8);
                const hn::Vec<D16> lc16 = hn::PromoteLowerTo(d16, lc8);
                const hn::Vec<D16> lr16 = hn::PromoteLowerTo(d16, lr8);

                const hn::Vec<D16> index = (ul16 & hn::Set(d16, UINT16_C(1) << 8)) |
                                           (uc16 & hn::Set(d16, UINT16_C(1) << 7)) |
                                           (ur16 & hn::Set(d16, UINT16_C(1) << 6)) |
                                           (cl16 & hn::Set(d16, UINT16_C(1) << 5)) |
                                           (cc16 & hn::Set(d16, UINT16_C(1) << 4)) |
                                           (cr16 & hn::Set(d16, UINT16_C(1) << 3)) |
                                           (ll16 & hn::Set(d16, UINT16_C(1) << 2)) |
                                           (lc16 & hn::Set(d16, UINT16_C(1) << 1)) |
                                           (lr16 & hn::Set(d16, UINT16_C(1) << 0));

                using DU16 = hn::RebindToUnsigned<D16>;
                HWY_ALIGN_MAX std::array<uint16_t, hn::MaxLanes(DU16())> indices;
                hn::Store(hn::BitCast(DU16(), index), DU16(), indices.data());

                int8_t *HWY_RESTRICT out = &new_image(y, x);
                for (size_t i = 0; i < hn::Lanes(d16); i += 8) {
                    out[i + 0] = pattern[indices[i + 0]];
                    out[i + 1] = pattern[indices[i + 1]];
                    out[i + 2] = pattern[indices[i + 2]];
                    out[i + 3] = pattern[indices[i + 3]];
                    out[i + 4] = pattern[indices[i + 4]];
                    out[i + 5] = pattern[indices[i + 5]];
                    out[i + 6] = pattern[indices[i + 6]];
                    out[i + 7] = pattern[indices[i + 7]];
                }
            }
        }

        y0--;
        y1++;
        x0--;
        x1++;

        if (n != max_iterations - 1) {
            // Fill in the border of the new image, corresponding to the state
            // of the infinite area outside of the bounding box, so the SIMD
            // code can handle the borders without any special casing.
            memset(&new_image(y0 - 2, x0 - 2), pad, x1 - x0 + 5);
            memset(&new_image(y0 - 1, x0 - 2), pad, x1 - x0 + 5);
            memset(&new_image(y1 + 1, x0 - 2), pad, x1 - x0 + 5);
            memset(&new_image(y1 + 2, x0 - 2), pad, x1 - x0 + 5);
            for (size_t y = y0 - 2; y <= y1 + 2; ++y) {
                new_image(y, x0 - 2) = pad;
                new_image(y, x0 - 1) = pad;
                new_image(y, x1 + 1) = pad;
                new_image(y, x1 + 2) = pad;
            }
        }

        std::swap(image, new_image);

        if (n == 1 || n == max_iterations - 1) {
            int count = 0;
            // Be careful only to count inside the bounding box: the SIMD loop
            // writes stray pixels past its end that we shouldn't count.
            for (size_t y = y0; y <= y1; ++y) {
                const int8_t *HWY_RESTRICT p = &image(y, x0);
                count += std::count(p, p + x1 - x0 + 1, -1);
            }
            answer.add(count);
        }
    }
}
AOC_REGISTER_SOLVER(2021, 20, run);

}
