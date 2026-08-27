#include <aoc/base.h>
#include <aoc/macros.h>
#include <aoc/string.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <hwy/highway.h>
#include <memory>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

namespace aoc_2023_24 {

namespace hn = hwy::HWY_NAMESPACE;
using D = hn::ScalableTag<double>;
constexpr D d;

struct Hailstones {
    size_t n;
    size_t npadded;
    const double *x;
    const double *y;
    const double *z;
    const double *vx;
    const double *vy;
    const double *vz;
};

static int64_t part1(const Hailstones &h)
{
    const double *HWY_RESTRICT x = h.x;
    const double *HWY_RESTRICT y = h.y;
    const double *HWY_RESTRICT vx = h.vx;
    const double *HWY_RESTRICT vy = h.vy;

    using U = hn::Rebind<int64_t, D>;
    constexpr U u;

    int64_t count = 0;

    const hn::Vec<D> area_min = hn::Set(d, 200000000000000.0);
    const hn::Vec<D> area_max = hn::Set(d, 400000000000000.0);
    const hn::Vec<U> sign_bit = hn::Set(u, 0x8000000000000000);

    for (size_t i = 0; i < h.n; ++i) {
        const hn::Vec<D> xi = hn::Set(d, x[i]);
        const hn::Vec<D> yi = hn::Set(d, y[i]);
        const hn::Vec<D> vxi = hn::Set(d, vx[i]);
        const hn::Vec<D> vyi = hn::Set(d, vy[i]);

        for (size_t j = i + 1; j + hn::Lanes(d) <= h.npadded; j += hn::Lanes(d)) {
            const hn::Vec<D> xj = hn::LoadU(d, &x[j]);
            const hn::Vec<D> yj = hn::LoadU(d, &y[j]);
            const hn::Vec<D> vxj = hn::LoadU(d, &vx[j]);
            const hn::Vec<D> vyj = hn::LoadU(d, &vy[j]);

            const hn::Vec<D> dx = xj - xi;
            const hn::Vec<D> dy = yj - yi;
            const hn::Vec<D> cross0 = hn::NegMulAdd(vxj, dy, vyj * dx);
            const hn::Vec<D> cross1 = hn::NegMulAdd(vxi, dy, vyi * dx);

            const hn::Vec<D> det = hn::NegMulAdd(vxj, vyi, vxi * vyj);
            const hn::Vec<D> s = cross0 / det;
            const hn::Vec<D> c0 = hn::MulAdd(s, vxi, xi);
            const hn::Vec<D> c1 = hn::MulAdd(s, vyi, yi);

            const hn::Vec<U> sdet = hn::BitCast(u, det) & sign_bit;
            const hn::Vec<U> scross0 = hn::BitCast(u, cross0) & sign_bit;
            const hn::Vec<U> scross1 = hn::BitCast(u, cross1) & sign_bit;

            const hn::Mask<D> valid_area =
                hn::And(hn::And(hn::Ge(c0, area_min), hn::Le(c0, area_max)),
                        hn::And(hn::Ge(c1, area_min), hn::Le(c1, area_max)));

            const hn::Mask<U> valid_intersection =
                hn::And(hn::Eq(sdet, scross0), hn::Eq(sdet, scross1));

            const hn::Mask<U> valid =
                hn::And(valid_intersection, hn::RebindMask(u, valid_area));

            count += hn::CountTrue(u, valid);
        }
    }

    return count;
}

static auto generate_lhs_rhs(const Hailstones &h, size_t a, size_t b)
{
    auto &[n, _, x, y, z, vx, vy, vz] = h;
    ASSERT(a < n && b < n);

    std::array<double, 3> dp{
        x[a] - x[b],
        y[a] - y[b],
        z[a] - z[b],
    };
    std::array<double, 3> dv{
        vx[a] - vx[b],
        vy[a] - vy[b],
        vz[a] - vz[b],
    };

    std::array<std::array<double, 6>, 3> lhs{{
        {0, -dv[2], dv[1], 0, -dp[2], dp[1]},
        {dv[2], 0, -dv[0], dp[2], 0, -dp[0]},
        {-dv[1], dv[0], 0, -dp[1], dp[0], 0},
    }};

    std::array<double, 3> rhs{
        y[b] * vz[b] - z[b] * vy[b] - (y[a] * vz[a] - z[a] * vy[a]),
        z[b] * vx[b] - x[b] * vz[b] - (z[a] * vx[a] - x[a] * vz[a]),
        x[b] * vy[b] - y[b] * vx[b] - (x[a] * vy[a] - y[a] * vx[a]),
    };

    return std::pair(lhs, rhs);
}

static int64_t part2(const Hailstones &hailstones)
{
    // Gaussian elimination with full pivoting. (Partial pivoting ends up with
    // the wrong answer due to the enormous inputs.)
    const auto [a01, b01] = generate_lhs_rhs(hailstones, 0, 1);
    const auto [a02, b02] = generate_lhs_rhs(hailstones, 0, 2);

    constexpr size_t rows = 6;
    constexpr size_t cols = 6;

    // Augmented matrix (last column is the right-hand side of equations).
    std::array<std::array<double, cols + 1>, rows> A;
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < cols; ++j)
            A[i][j] = a01[i][j];
        A[i][cols] = b01[i];
    }
    for (size_t i = 3; i < 6; ++i) {
        for (size_t j = 0; j < cols; ++j)
            A[i][j] = a02[i - 3][j];
        A[i][cols] = b02[i - 3];
    }

    // Variable ordering (changing with column swaps).
    std::array<int, cols> perm;
    for (size_t i = 0; i < cols; ++i)
        perm[i] = i;

    // Forward pass:
    for (size_t k = 0; k < rows; ++k) {
        size_t maxi = k;
        size_t maxj = k;
        for (size_t i = k; i < rows; ++i)
            for (size_t j = k; j < cols; ++j)
                if (std::abs(A[i][j]) > std::abs(A[maxi][maxj]))
                    std::tie(maxi, maxj) = std::pair(i, j);

        ASSERT(maxi < rows && maxj < cols);
        ASSERT(std::abs(A[maxi][maxj]) > 1e-9);

        std::swap(A[k], A[maxi]);
        for (size_t i = 0; i < rows; ++i)
            std::swap(A[i][k], A[i][maxj]);
        std::swap(perm[k], perm[maxj]);

        for (size_t i = k + 1; i < rows; ++i) {
            const double factor = A[i][k] / A[k][k];
            for (size_t j = k; j <= cols; ++j)
                A[i][j] -= factor * A[k][j];
        }
    }

    // Back substitution:
    for (size_t k = rows; k-- > 0;) {
        for (size_t i = 0; i < k; ++i) {
            const double factor = A[i][k] / A[k][k];
            for (size_t j = k; j <= cols; ++j)
                A[i][j] -= factor * A[k][j];
        }
    }

    for (size_t k = 0; k < rows; ++k) {
        const double factor = A[k][k];
        for (size_t j = k; j <= cols; ++j)
            A[k][j] /= factor;
    }

    const double a = A[std::ranges::find(perm, 0) - perm.begin()][cols];
    const double b = A[std::ranges::find(perm, 1) - perm.begin()][cols];
    const double c = A[std::ranges::find(perm, 2) - perm.begin()][cols];
    return round(a) + round(b) + round(c);
}

void run(std::string_view buf, aoc::Answer &answer)
{
    auto nums = find_numbers<int64_t>(buf);
    ASSERT(nums.size() % 6 == 0);
    const size_t n = nums.size() / 6;
    const size_t npadded = n + hn::Lanes(d);
    auto buffer = std::make_unique<double[]>(6 * npadded);
    std::span<double> x(buffer.get() + 0 * npadded, npadded);
    std::span<double> y(buffer.get() + 1 * npadded, npadded);
    std::span<double> z(buffer.get() + 2 * npadded, npadded);
    std::span<double> vx(buffer.get() + 3 * npadded, npadded);
    std::span<double> vy(buffer.get() + 4 * npadded, npadded);
    std::span<double> vz(buffer.get() + 5 * npadded, npadded);

    for (size_t i = 0; i < n; ++i) {
        x[i] = nums[i * 6 + 0];
        y[i] = nums[i * 6 + 1];
        z[i] = nums[i * 6 + 2];
        vx[i] = nums[i * 6 + 3];
        vy[i] = nums[i * 6 + 4];
        vz[i] = nums[i * 6 + 5];
    }

    Hailstones hailstones{n, npadded, &x[0], &y[0], &z[0], &vx[0], &vy[0], &vz[0]};
    answer.add(part1(hailstones));
    answer.add(part2(hailstones));
}
AOC_REGISTER_SOLVER(2023, 24, run);

}
