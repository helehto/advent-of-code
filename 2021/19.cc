#include "common.h"
#include "dense_set.h"

namespace aoc_2021_19 {

using Transformation = std::array<std::array<int16_t, 4>, 4>;
using Point = std::array<int16_t, 3>;

/// All 24 proper rotations in 3D, represented as 4x4 matrices in homogeneous
/// coordinates with no translation component.
constexpr auto all_rotations = [] {
    std::array<Transformation, 24> result{};

    constexpr std::pair<int, int> v[] = {{0, +1}, {1, +1}, {2, +1},
                                         {0, -1}, {1, -1}, {2, -1}};

    for (size_t k = 0; auto [i, si] : v) {
        for (auto [j, sj] : v) {
            if (i == j)
                continue;

            Transformation &R = result[k++];
            R[i][0] = si;
            R[j][1] = sj;
            R[3][3] = 1;

            // Cross product of first two vectors.
            R[0][2] = R[1][0] * R[2][1] - R[2][0] * R[1][1];
            R[1][2] = R[2][0] * R[0][1] - R[0][0] * R[2][1];
            R[2][2] = R[0][0] * R[1][1] - R[1][0] * R[0][1];
        }
    }

    return result;
}();

/// Apply a transformation to a point.
constexpr Point transform(const Transformation &A, const Point &p)
{
    return Point{
        static_cast<int16_t>(A[0][0] * p[0] + A[0][1] * p[1] + A[0][2] * p[2] + A[0][3]),
        static_cast<int16_t>(A[1][0] * p[0] + A[1][1] * p[1] + A[1][2] * p[2] + A[1][3]),
        static_cast<int16_t>(A[2][0] * p[0] + A[2][1] * p[1] + A[2][2] * p[2] + A[2][3]),
    };
}

/// Fingerprint for a pair of points, invariant under rotation and translation.
///
/// Because of these properties, this can be used to figure out whether a pair
/// of points is likely the same as another pair of points, even if the two
/// pairs are detected by different scanners (i.e. are specified in different
/// coordinate systems).
struct RTInvariantPairFingerprint {
    int16_t manhattan;
    int16_t dmin;
    int16_t dmax;
    uint8_t index1;
    uint8_t index2;

    constexpr RTInvariantPairFingerprint() = default;

    constexpr RTInvariantPairFingerprint(size_t index1,
                                         size_t index2,
                                         const Point &p,
                                         const Point &q)
        : index1(index1)
        , index2(index2)
    {
        const int dx = std::abs(p[0] - q[0]);
        const int dy = std::abs(p[1] - q[1]);
        const int dz = std::abs(p[2] - q[2]);
        manhattan = dx + dy + dz;
        dmin = std::min({dx, dy, dz});
        dmax = std::max({dx, dy, dz});
    }

    constexpr std::weak_ordering
    operator<=>(const RTInvariantPairFingerprint &other) const noexcept
    {
        return std::tuple(manhattan, dmin, dmax) <=>
               std::tuple(other.manhattan, other.dmin, other.dmax);
    }
};

/// Generate a set of rotation- and translation-invariant fingerprints for an
/// entire cluster.
static small_vector<RTInvariantPairFingerprint>
generate_fingerprints(std::span<const Point> points)
{
    small_vector<RTInvariantPairFingerprint> result;

    size_t k = 0;
    for (size_t i = 0; i < points.size(); ++i)
        for (size_t j = i + 1; j < points.size(); ++j, ++k)
            result.emplace_back(i, j, points[i], points[j]);

    std::ranges::sort(result, λab(a < b));
    return result;
}

/// Fingerprint for a set of points, invariant under translation.
struct TInvariantFingerprint {
    int16_t dx;
    int16_t dy;
    int16_t dz;
    float xy_angle_sum = 0;

    TInvariantFingerprint(std::span<const Point> points)
    {
        using namespace std::ranges;
        DEBUG_ASSERT(is_sorted(points));

        const auto [xmin, xmax] = minmax(points | views::transform(λa(a[0])));
        const auto [ymin, ymax] = minmax(points | views::transform(λa(a[1])));
        const auto [zmin, zmax] = minmax(points | views::transform(λa(a[2])));
        dx = xmax - xmin;
        dy = ymax - ymin;
        dz = zmax - zmin;

        const auto &p0 = points[0];
        for (size_t i = 1; i < points.size(); ++i) {
            const auto &p = points[i];
            xy_angle_sum += std::atan2f(p[1] - p0[1], p[0] - p0[0]);
        }
    }

    constexpr bool operator==(const TInvariantFingerprint &other) const noexcept
    {
        return dx == other.dx && dy == other.dy && dz == other.dz &&
               std::abs(xy_angle_sum - other.xy_angle_sum) < 1e-3f;
    }
};

/// Given the two clusters of points `c0` and `c1`, which are specified in
/// different coordinate systems, along with the fingerprints of the two sets,
/// return a set of points from each clusters that are likely the same
/// collection of points (but relative to different sensors).
static std::pair<small_vector<Point>, small_vector<Point>>
find_intersection_candidates(std::span<const Point> c0,
                             std::span<const Point> c1,
                             std::span<const RTInvariantPairFingerprint> fingerprints0,
                             std::span<const RTInvariantPairFingerprint> fingerprints1)
{
    small_vector<Point> ps, qs;

    /// The two fingerprint sets are sorted by construction; merge them in O(n)
    /// time, gathering points which have equal fingerprints.
    for (size_t i = 0, j = 0; i < fingerprints0.size() && j < fingerprints1.size();) {
        if (fingerprints0[i] < fingerprints1[j]) {
            ++i;
        } else if (fingerprints1[j] < fingerprints0[i]) {
            ++j;
        } else {
            ps.push_back(c0[fingerprints0[i].index1]);
            ps.push_back(c0[fingerprints0[i].index2]);
            qs.push_back(c1[fingerprints1[j].index1]);
            qs.push_back(c1[fingerprints1[j].index2]);
            ++i;
            ++j;
        }
    }

    // Each pair will contribute two points; sort and get rid of duplicates.
    std::ranges::sort(ps);
    std::ranges::sort(qs);
    ps.erase(std::ranges::unique(ps).begin(), ps.end());
    qs.erase(std::ranges::unique(qs).begin(), qs.end());

    return {ps, qs};
}

/// Given the two clusters of points `c0` and `c1`, which are specified in
/// different coordinate systems, along with the fingerprints of the two sets,
/// return a 4x4 homogeneous coordinate transformation that maps points in `c1`
/// to `c0`.
static std::optional<Transformation>
get_transform(std::span<const Point> c0,
              std::span<const Point> c1,
              std::span<const RTInvariantPairFingerprint> fingerprints0,
              std::span<const RTInvariantPairFingerprint> fingerprints1)
{
    const auto [ps, qs] =
        find_intersection_candidates(c0, c1, fingerprints0, fingerprints1);
    if (ps.size() < 12)
        return std::nullopt;

    const TInvariantFingerprint tifp(ps);

    small_vector<Point> transformed;
    for (Transformation A : all_rotations) {
        transformed.clear();
        for (const Point &v : qs)
            transformed.push_back(transform(A, v));

        std::ranges::sort(transformed);

        // If this is the correct rotation, the two intersecting sets will only
        // differ by translation, so they have the same translation-invariant
        // fingerprint by definition.
        if (tifp == TInvariantFingerprint(transformed)) {
            A[0][3] = ps[0][0] - transformed[0][0];
            A[1][3] = ps[0][1] - transformed[0][1];
            A[2][3] = ps[0][2] - transformed[0][2];
            return A;
        }
    }

    ASSERT_MSG(false, "Did not find transform from overlapping scanners!?");
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    // Clusters as given in the input (initially in the scanner-local
    // coordinate space).
    std::vector<small_vector<Point, 26>> clusters;
    clusters.reserve(lines.size() / 29);

    size_t i = 0;
    while (i < lines.size()) {
        ++i; // skip --- scanner X ---
        auto &points = clusters.emplace_back();
        for (; i < lines.size() && !lines[i].empty(); ++i)
            points.push_back(find_numbers_n<int16_t, 3>(lines[i]));
        ++i; // skip empty line
    }
    ASSERT(clusters.size() < 64);

    std::vector<small_vector<RTInvariantPairFingerprint>> fingerprints;
    for (size_t i = 0; i < clusters.size(); ++i)
        fingerprints.push_back(generate_fingerprints(clusters[i]));

    // Coordinates of all scanners in scanner 0's coordinate space.
    small_vector<Point> scanners(clusters.size());

    // The main loop: iteratively explore clusters that overlap with
    for (uint64_t visited = 0, pending = 1; pending != 0;) {
        const int i = std::countr_zero(pending);
        pending &= pending - 1;
        visited |= UINT64_C(1) << i;

        for (size_t j = 0; j < fingerprints.size(); j++) {
            if (((pending | visited) & (UINT64_C(1) << j)) != 0)
                continue;

            std::optional<Transformation> candidate =
                get_transform(clusters[i], clusters[j], fingerprints[i], fingerprints[j]);
            if (!candidate.has_value())
                continue; // no overlap

            const Transformation &A = *candidate;

            for (size_t k = 0; k < clusters[j].size(); ++k)
                clusters[j][k] = transform(A, clusters[j][k]);

            scanners[j] = {A[0][3], A[1][3], A[2][3]};
            pending |= UINT64_C(1) << j;
        }
    }

    dense_set<Point, CrcHasher> all_points;
    for (size_t i = 0; i < clusters.size(); ++i)
        all_points.insert(clusters[i].begin(), clusters[i].end());
    fmt::print("{}\n", all_points.size());

    int max_manhattan = INT_MIN;
    for (size_t i = 0; i < scanners.size(); ++i) {
        for (size_t j = i + 1; j < scanners.size(); ++j) {
            const int dx = std::abs(scanners[i][0] - scanners[j][0]);
            const int dy = std::abs(scanners[i][1] - scanners[j][1]);
            const int dz = std::abs(scanners[i][2] - scanners[j][2]);
            max_manhattan = std::max(max_manhattan, dx + dy + dz);
        }
    }
    fmt::print("{}\n", max_manhattan);
}

}
