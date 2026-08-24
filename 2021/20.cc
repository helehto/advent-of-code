#include <algorithm>
#include <aoc/base.h>
#include <aoc/math.h>
#include <aoc/string.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <hwy/aligned_allocator.h>
#include <hwy/highway.h>
#include <span>
#include <string_view>
#include <utility>

namespace aoc_2021_20 {

namespace hn = hwy::HWY_NAMESPACE;

using D = hn::ScalableTag<uint8_t>;
constexpr D d;

// 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, ... repeating.
HWY_ALIGN_MAX constexpr auto bitmask_lookup_array =
    std::to_array<uint8_t>({0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02,
                            0x04, 0x08, 0x10, 0x20, 0x40, 0x80});
static const hn::Vec<D> bitmask_lookup = hn::LoadDup128(d, bitmask_lookup_array.data());

// Variant for 128-bit SIMD. The table is split across 4 vectors, each pair
// representing one half of the table.
class Table128x4 {
    hn::Vec<D> pattern0_lo; // bits 0-127
    hn::Vec<D> pattern0_hi; // bits 128-255
    hn::Vec<D> pattern1_lo; // bits 256-383
    hn::Vec<D> pattern1_hi; // bits 384-511

    hn::Vec<D>
    lookup(const hn::Vec<D> lo, const hn::Vec<D> hi, const hn::Vec<D> index) const
    {
        const hn::Vec<D> byte_idx = hn::ShiftRight<3>(index);
        const hn::Vec<D> bitsets =
            hn::TwoTablesLookupLanes(d, lo, hi, hn::IndicesFromVec(d, byte_idx));
        const hn::Vec<D> bit_idx = hn::And(index, hn::Set(d, 7));
        const hn::Vec<D> bitmask = hn::TableLookupBytes(bitmask_lookup, bit_idx);
        return hn::VecFromMask(hn::TestBit(bitsets, bitmask));
    }

public:
    Table128x4(std::span<const uint8_t, 512 / 8> bits)
        : pattern0_lo(hn::LoadU(d, &bits[0]))
        , pattern0_hi(hn::LoadU(d, &bits[16]))
        , pattern1_lo(hn::LoadU(d, &bits[32]))
        , pattern1_hi(hn::LoadU(d, &bits[48]))
    {
    }

    hn::Vec<D> map(const hn::Vec<D> selector, const hn::Vec<D> index) const
    {
        const hn::Vec<D> result0 = lookup(pattern0_lo, pattern0_hi, index);
        const hn::Vec<D> result1 = lookup(pattern1_lo, pattern1_hi, index);
        return hn::IfVecThenElse(selector, result1, result0);
    }
};

// Variant for ≥256-bit SIMD. Each table half fits neatly in a single vector.
//
// Note that >256-bit vectors don't help much here, other than mapping more
// elements at a time. We are limited by the 8-bit lanes rather than the vector
// width; since the 3x3 neighborhood defines a 512-bit table that requires 9
// bits to index, we need to split the table into two halves anyway and blend
// the results.
class Table256x2 {
    hn::Vec<D> pattern0;
    hn::Vec<D> pattern1;

    hn::Vec<D> lookup(const hn::Vec<D> table, const hn::Vec<D> index) const
    {
        const hn::Vec<D> byte_idx = hn::ShiftRight<3>(index);
        const hn::Vec<D> bitsets =
            hn::TableLookupLanes(table, hn::IndicesFromVec(d, byte_idx));
        const hn::Vec<D> bit_idx = hn::And(index, hn::Set(d, 7));
        const hn::Vec<D> bitmask = hn::TableLookupBytes(bitmask_lookup, bit_idx);
        return hn::VecFromMask(hn::TestBit(bitsets, bitmask));
    }

public:
    Table256x2(std::span<const uint8_t, 512 / 8> bits)
        : pattern0(hn::LoadN(d, &bits[0], 32))
        , pattern1(hn::LoadN(d, &bits[32], 32))
    {
    }

    hn::Vec<D> map(const hn::Vec<D> selector, const hn::Vec<D> index) const
    {
        const hn::Vec<D> result0 = lookup(pattern0, index);
        const hn::Vec<D> result1 = lookup(pattern1, index);
        return hn::IfVecThenElse(selector, result1, result0);
    }
};

static_assert(hn::MaxLanes(d) >= 16);
using LookupTable = std::conditional_t<hn::MaxLanes(d) == 16, Table128x4, Table256x2>;

static std::array<uint8_t, 512 / 8> parse_pattern(std::string_view pat)
{
    ASSERT(pat.size() == 512);
    std::array<uint8_t, 512 / 8> pattern_bits{};
    for (size_t i = 0; i < 512; ++i)
        pattern_bits[i / 8] |= (pat[i] == '#') ? (1 << (i % 8)) : 0;
    return pattern_bits;
}

void run(std::string_view buf, aoc::Answer &answer)
{
    const bool first_is_lit = buf[0] == '#';
    auto lines = split_lines(buf);
    LookupTable table(parse_pattern(lines[0]));

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

    auto storage = hwy::MakeUniqueAlignedArray<uint8_t>(2 * rows * cols);
    MatrixView<uint8_t> image(storage.get(), rows, cols);
    MatrixView<uint8_t> new_image(storage.get() + rows * cols, rows, cols);

    for (size_t y = y0; y <= y1; ++y)
        for (size_t x = x0; x <= x1; ++x)
            image(y, x) = (image_lines[y - y0][x - x0] == '#') ? 0xff : 0;

    const hn::Vec<D> k7 = hn::Set(d, 1 << 7);
    const hn::Vec<D> k6 = hn::Set(d, 1 << 6);
    const hn::Vec<D> k5 = hn::Set(d, 1 << 5);
    const hn::Vec<D> k4 = hn::Set(d, 1 << 4);
    const hn::Vec<D> k3 = hn::Set(d, 1 << 3);
    const hn::Vec<D> k2 = hn::Set(d, 1 << 2);
    const hn::Vec<D> k1 = hn::Set(d, 1 << 1);
    const hn::Vec<D> k0 = hn::Set(d, 1 << 0);

    auto make_index = [&](const hn::Vec<D> uc, const hn::Vec<D> ur, const hn::Vec<D> ml,
                          const hn::Vec<D> mc, const hn::Vec<D> mr, const hn::Vec<D> ll,
                          const hn::Vec<D> lc, const hn::Vec<D> lr) {
        const hn::Vec<D> u = hn::Xor(uc & k7, ur & k6);
        const hn::Vec<D> m = hn::Xor3(ml & k5, mc & k4, mr & k3);
        const hn::Vec<D> l = hn::Xor3(ll & k2, lc & k1, lr & k0);
        return hn::Xor3(u, m, l);
    };

    bool outside_is_lit = false;
    for (size_t n = 0; n < max_iterations; ++n, outside_is_lit = !outside_is_lit) {
        const uint8_t pad = first_is_lit && !outside_is_lit ? 0xff : 0;

        // Fill in the interior of the new image. Unrolled twice along y to
        // enable output rows to share loaded input rows.
        for (size_t y = y0 - 1; y <= y1 + 1; y += 2) {
            // NB: the matrix is sized so that we can deliberately over-read
            // past the current bounding box, meaning that we don't need a
            // scalar loop to handle the tail.
            for (size_t x = x0 - 1; x <= x1 + 1; x += hn::Lanes(d)) {
                const auto *HWY_RESTRICT p = &image(y, x);
                const hn::Vec<D> l0 = hn::LoadU(d, p - 1 * cols - 1);
                const hn::Vec<D> c0 = hn::LoadU(d, p - 1 * cols + 0);
                const hn::Vec<D> r0 = hn::LoadU(d, p - 1 * cols + 1);
                const hn::Vec<D> l1 = hn::LoadU(d, p + 0 * cols - 1);
                const hn::Vec<D> c1 = hn::LoadU(d, p + 0 * cols + 0);
                const hn::Vec<D> r1 = hn::LoadU(d, p + 0 * cols + 1);
                const hn::Vec<D> l2 = hn::LoadU(d, p + 1 * cols - 1);
                const hn::Vec<D> c2 = hn::LoadU(d, p + 1 * cols + 0);
                const hn::Vec<D> r2 = hn::LoadU(d, p + 1 * cols + 1);
                const hn::Vec<D> l3 = hn::LoadU(d, p + 2 * cols - 1);
                const hn::Vec<D> c3 = hn::LoadU(d, p + 2 * cols + 0);
                const hn::Vec<D> r3 = hn::LoadU(d, p + 2 * cols + 1);

                // The top-left bit (l0/11) selects which table half to use.
                // The rest are treated as a bit index into that.
                const hn::Vec<D> index0 = make_index(c0, r0, l1, c1, r1, l2, c2, r2);
                const hn::Vec<D> index1 = make_index(c1, r1, l2, c2, r2, l3, c3, r3);

                const hn::Vec<D> result0 = table.map(l0, index0);
                const hn::Vec<D> result1 = table.map(l1, index1);
                hn::StoreU(result0, d, &new_image(y, x));
                hn::StoreU(result1, d, &new_image(y + 1, x));
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
                const uint8_t *HWY_RESTRICT p = &image(y, x0);
                count += std::count(p, p + x1 - x0 + 1, 0xff);
            }
            answer.add(count);
        }
    }
}
AOC_REGISTER_SOLVER(2021, 20, run);
}
