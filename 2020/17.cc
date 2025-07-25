#include "common.h"
#include "dense_set.h"

namespace aoc_2020_17 {

// clang-format off
alignas(16) static constexpr uint8_t tail_masks[][16] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00},
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
};
// clang-format on

struct Grid4D {
    std::unique_ptr<uint8_t[]> storage;
    std::array<size_t, 4> extents;
    std::array<size_t, 3> strides;
    std::array<size_t, 4> bbox_min;
    std::array<size_t, 4> bbox_max;

    constexpr Grid4D() noexcept = default;

    constexpr Grid4D(std::array<size_t, 4> extents) noexcept
        : extents(extents)
        , strides{
              extents[1] * extents[2] * extents[3],
              extents[2] * extents[3],
              extents[3],
          }
    {
        storage = std::make_unique<uint8_t[]>(extents[0] * strides[0]);
    }

    constexpr uint8_t &operator[](size_t w, size_t z, size_t y, size_t x) const noexcept
    {
        return storage[strides[0] * w + strides[1] * z + strides[2] * y + x];
    }

    constexpr std::span<const uint8_t> all() const noexcept
    {
        return std::span(storage.get(), storage.get() + extents[0] * strides[0]);
    }

    constexpr size_t count_active() const noexcept
    {
        size_t result = 0;

        for (size_t w = bbox_min[0]; w <= bbox_max[0]; ++w) {
            for (size_t z = bbox_min[1]; z <= bbox_max[1]; ++z) {
                for (size_t y = bbox_min[2]; y <= bbox_max[2]; ++y) {
                    uint8_t *p = &(*this)[w, z, y, bbox_min[3]];
                    for (size_t i = 0; i <= bbox_max[3] - bbox_min[3]; ++i)
                        result += p[i];
                }
            }
        }

        return result;
    }
};

constexpr int count_neighbors3d(const Grid4D &grid, uint8_t *p)
{
    const ssize_t zstride = grid.strides[1];
    const ssize_t ystride = grid.strides[2];
    uint8_t result = 0;

    for (ssize_t dz = -1; dz <= 1; ++dz)
        for (ssize_t dy = -1; dy <= 1; ++dy)
            for (ssize_t dx = -1; dx <= 1; ++dx)
                result += p[dz * zstride + dy * ystride + dx];

    return result - *p;
}

static Grid4D step_grid_3d(const Grid4D &prev)
{
    constexpr int pad = 2;

    size_t zmin = SIZE_MAX, zmax = 0;
    size_t ymin = SIZE_MAX, ymax = 0;
    size_t xmin = SIZE_MAX, xmax = 0;

    for (size_t z = pad; z < prev.extents[1] - pad; ++z) {
        for (size_t y = pad; y < prev.extents[2] - pad; ++y) {
            for (size_t x = pad; x < prev.extents[3] - pad; ++x) {
                if (prev[0, z, y, x]) {
                    zmin = std::min(zmin, z);
                    zmax = std::max(zmax, z);
                    ymin = std::min(ymin, y);
                    ymax = std::max(ymax, y);
                    xmin = std::min(xmin, x);
                    xmax = std::max(xmax, x);
                }
            }
        }
    }

    Grid4D next({
        1,
        zmax - zmin + 2 * pad + 3,
        ymax - ymin + 2 * pad + 3,
        xmax - xmin + 2 * pad + 3,
    });

    for (size_t z = pad; z < next.extents[1] - pad; ++z) {
        for (size_t y = pad; y < next.extents[2] - pad; ++y) {
            uint8_t *p = &prev[0, z - pad + zmin - 1, y - pad + ymin - 1, xmin - 1];

            for (size_t x = pad; x < next.extents[3] - pad; ++x, ++p) {
                const int active_neighbors = count_neighbors3d(prev, p);
                next[0, z, y, x] = *p ? active_neighbors == 2 || active_neighbors == 3
                                      : active_neighbors == 3;
            }
        }
    }

    return next;
}

/// Count the number of neighbors to the elements `p .. p+15`.
static __m128i count_neighbors4d_16(const Grid4D &grid, const uint8_t *const p)
{
    __m128i result = _mm_setzero_si128();
    const ssize_t wstride = grid.strides[0];
    const ssize_t zstride = grid.strides[1];
    const ssize_t ystride = grid.strides[2];

    for (ssize_t dw = -1; dw <= 1; ++dw) {
        for (ssize_t dz = -1; dz <= 1; ++dz) {
            for (ssize_t dy = -1; dy <= 1; ++dy) {
                const uint8_t *row = &p[dw * wstride + dz * zstride + dy * ystride];
                result = _mm_add_epi8(result, _mm_loadu_si128((const __m128i *)&row[-1]));
                result = _mm_add_epi8(result, _mm_loadu_si128((const __m128i *)&row[0]));
                result = _mm_add_epi8(result, _mm_loadu_si128((const __m128i *)&row[1]));
            }
        }
    }

    return _mm_sub_epi8(result, _mm_loadu_si128((const __m128i *)p));
}

/// Compute the next state of the elements at `prev .. prev+15`, given a vector
/// of neighbor counts.
static __m128i step_chunk16(const __m128i neighbors, const uint8_t *prev)
{
    const __m128i vprev = _mm_loadu_si128((const __m128i *)prev);
    const __m128i prev_zero_mask = _mm_cmpeq_epi8(vprev, _mm_setzero_si128());
    const __m128i eq2 = _mm_abs_epi8(_mm_cmpeq_epi8(neighbors, _mm_set1_epi8(2)));
    const __m128i eq3 = _mm_abs_epi8(_mm_cmpeq_epi8(neighbors, _mm_set1_epi8(3)));
    const __m128i eq2or3 = _mm_or_si128(eq2, eq3);
    return _mm_blendv_epi8(eq2or3, eq3, prev_zero_mask);
}

/// Compute the next state of a full row of length `n` in `prev_grid`, starting
/// at `prevp`, and write the results to `nextp`.
[[gnu::noinline, gnu::flatten]] static void
step_row_4d(const Grid4D &prev_grid, uint8_t *nextp, size_t n, const uint8_t *prevp)
{
    for (;; n -= 16, nextp += 16, prevp += 16) {
        const __m128i neighbors = count_neighbors4d_16(prev_grid, prevp);
        const __m128i result = step_chunk16(neighbors, prevp);
        if (n >= 16) {
            _mm_storeu_si128((__m128i *)nextp, result);
        } else {
            const __m128i mask = _mm_loadu_si128((const __m128i *)tail_masks[n]);
            _mm_storeu_si128((__m128i *)nextp, _mm_and_si128(result, mask));
            break;
        }
    }
}

[[gnu::noinline]] static Grid4D step_grid_4d(const Grid4D &prev)
{
    constexpr int pad = 2;
    const size_t innerw = prev.bbox_max[0] - prev.bbox_min[0] + 3;
    const size_t innerz = prev.bbox_max[1] - prev.bbox_min[1] + 3;
    const size_t innery = prev.bbox_max[2] - prev.bbox_min[2] + 3;
    const size_t innerx = prev.bbox_max[3] - prev.bbox_min[3] + 3;

    Grid4D next({
        (innerw + 2 * pad + 16) & ~15,
        (innerz + 2 * pad + 16) & ~15,
        (innery + 2 * pad + 16) & ~15,
        (innerx + 2 * pad + 16) & ~15,
    });

    for (size_t w = 0; w < innerw; ++w) {
        for (size_t z = 0; z < innerz; ++z) {
            for (size_t y = 0; y < innery; ++y) {
                step_row_4d(prev, &next[w + pad, z + pad, y + pad, pad], innerx,
                            &prev[w + prev.bbox_min[0] - 1, z + prev.bbox_min[1] - 1,
                                  y + prev.bbox_min[2] - 1, 0 + prev.bbox_min[3] - 1]);
            }
        }
    }

    next.bbox_min = {SIZE_MAX, SIZE_MAX, SIZE_MAX, SIZE_MAX};
    next.bbox_max = {0, 0, 0, 0};
    for (size_t w = pad; w < innerw + pad; ++w) {
        for (size_t z = pad; z < innerz + pad; ++z) {
            for (size_t y = pad; y < innery + pad; ++y) {
                for (size_t x = pad; x < innerx + pad; ++x) {
                    if (next[w, z, y, x]) {
                        next.bbox_min[0] = std::min(next.bbox_min[0], w);
                        next.bbox_min[1] = std::min(next.bbox_min[1], z);
                        next.bbox_min[2] = std::min(next.bbox_min[2], y);
                        next.bbox_min[3] = std::min(next.bbox_min[3], x);
                        next.bbox_max[0] = std::max(next.bbox_max[0], w);
                        next.bbox_max[1] = std::max(next.bbox_max[1], z);
                        next.bbox_max[2] = std::max(next.bbox_max[2], y);
                        next.bbox_max[3] = std::max(next.bbox_max[3], x);
                    }
                }
            }
        }
    }

    return next;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    auto grid = Matrix<char>::from_lines(lines);

    std::array<size_t, 4> extents{1, 1, 0, 0};
    for (auto p : grid.ndindex()) {
        if (grid(p) == '#') {
            extents[2] = std::max(extents[2], p.y);
            extents[3] = std::max(extents[3], p.x);
        }
    }
    extents[2]++;
    extents[3]++;

    constexpr int pad = 2;

    {
        extents[1] += 2 * pad;
        extents[2] += 2 * pad;
        extents[3] += 2 * pad;

        Grid4D active(extents);
        for (auto p : grid.ndindex())
            if (grid(p) == '#')
                active[0, pad, p.y + pad, p.x + pad] = 1;

        for (size_t i = 0; i < 6; ++i)
            active = step_grid_3d(active);

        fmt::print("{}\n", std::ranges::count(active.all(), 1));
    }

    {
        constexpr int pad = 2;

        extents[0] += 2 * pad;

        Grid4D active(extents);

        active.bbox_min = {pad, pad, SIZE_MAX, SIZE_MAX};
        active.bbox_max = {pad, pad, 0, 0};

        for (auto p : grid.ndindex()) {
            if (grid(p) == '#') {
                active[pad, pad, p.y + pad, p.x + pad] = 1;
                active.bbox_min[2] = std::min(active.bbox_min[2], p.y + pad);
                active.bbox_min[3] = std::min(active.bbox_min[3], p.x + pad);
                active.bbox_max[2] = std::max(active.bbox_max[2], p.y + pad);
                active.bbox_max[3] = std::max(active.bbox_max[3], p.x + pad);
            }
        }

        for (size_t i = 0; i < 6; ++i)
            active = step_grid_4d(active);

        fmt::print("{}\n", active.count_active());
    }
}

}
