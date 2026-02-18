#include "common.h"
#include <hwy/highway.h>

namespace aoc_2021_17 {

using D = hn::ScalableTag<int16_t>;
constexpr D d;

/// Run the simulation for the velocities given by the lanes of `vx` and `vy`
/// for which the corresponding element in active_mask is set with the target
/// area given by the rectangle (x0, y0), (x1, y1) inclusive.
static int simulate(hn::Vec<D> vx,
                    hn::Vec<D> vy,
                    hn::Mask<D> active_mask,
                    const int16_t x0,
                    const int16_t x1,
                    const int16_t y0,
                    const int16_t y1,
                    size_t &valid_velocities_count)
{
    hn::Vec<D> x = hn::Zero(d);
    hn::Vec<D> y = hn::Zero(d);
    hn::Mask<D> reached_mask = hn::MaskFalse(d);

    // Target area bounds:
    const hn::Vec<D> targetl = hn::Set(d, x0 - 1);
    const hn::Vec<D> targetr = hn::Set(d, x1 + 1);
    const hn::Vec<D> targetd = hn::Set(d, y0 - 1);
    const hn::Vec<D> targetu = hn::Set(d, y1 + 1);

    hn::Vec<D> ymax = hn::Set(d, INT16_MIN);
    while (true) {
        // Figure out which probes are below the target area. We assume the
        // target area to be below y=0, so at this point they are moving
        // downwards below it and will *never* intersect it (again):
        const hn::Mask<D> below_target_mask = hn::Gt(targetd, y);

        // Figure out which probes are in the target area:
        const hn::Mask<D> pgtx0 = hn::Gt(x, targetl);
        const hn::Mask<D> pltx1 = hn::Lt(x, targetr);
        const hn::Mask<D> pgty0 = hn::Gt(y, targetd);
        const hn::Mask<D> plty1 = hn::Lt(y, targetu);
        const hn::Mask<D> in_target_x = hn::And(pgtx0, pltx1);
        const hn::Mask<D> in_target_y = hn::And(pgty0, plty1);
        const hn::Mask<D> in_target_area = hn::And(in_target_x, in_target_y);

        // Update the mask signifying which probes that have reached the target
        // area for this simulation.
        reached_mask = hn::Or(in_target_area, reached_mask);

        // Deactivate the probes that are inside or below the target area.
        const hn::Mask<D> disable = hn::Or(below_target_mask, in_target_area);
        active_mask = hn::AndNot(disable, active_mask);

        // If no probe is active, we are done.
        if (hn::AllFalse(d, active_mask))
            break;

        // Simulate one time step.
        constexpr hn::RebindToUnsigned<D> du16;
        x += vx;
        y += vy;
        vx = hn::BitCast(d, hn::SaturatedSub(hn::BitCast(du16, vx), hn::Set(du16, 1)));
        vy -= hn::Set(d, 1);
        ymax = hn::Max(ymax, y);
    }
    valid_velocities_count += hn::CountTrue(d, reached_mask);
    return hn::MaskedReduceMax(d, reached_mask, ymax);
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

    size_t valid_velocities_count = 0;

    // The lower bound is the X velocity below which probes never reach the
    // target area before the velocity drops to zero.
    int ymax = y1;
    for (int vx = std::floor(std::sqrt(8 * x0) / 2); vx < x0; ++vx) {
        // Skip full simulation of the velocities for which the target area is
        // skipped completely in the X axis:
        if (!vx_can_reach_target_area(vx, x0, x1))
            continue;

        const hn::Vec<D> vxx = hn::Set(d, vx);
        for (int vy = y0; vy <= -y0; vy += hn::Lanes(d)) {
            const hn::Vec<D> vyy = hn::Set(d, vy) + hn::Iota(d, 0);
            const size_t n = -y0 - vy + 1;
            const hn::Mask<D> active_mask = hn::FirstN(d, n);
            ymax = std::max(ymax, simulate(vxx, vyy, active_mask, x0, x1, y0, y1,
                                           valid_velocities_count));
        }
    }

    fmt::print("{}\n", ymax);

    // Velocities that match the target area will cause the probe to enter the
    // target area within one time step, by definition; count those separately:
    fmt::print("{}\n", valid_velocities_count + (y1 - y0 + 1) * (x1 - x0 + 1));
}

}
