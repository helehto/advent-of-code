#include "common.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walloca"
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#include <Eigen/Core>
#include <Eigen/Dense>
#pragma GCC diagnostic pop

namespace aoc_2023_24 {

struct Hailstone {
    Eigen::Vector3d p;
    Eigen::Vector3d v;
};

static int64_t part1(std::span<const Hailstone> hailstones)
{
    int64_t count = 0;
    constexpr int64_t area_min = 200000000000000;
    constexpr int64_t area_max = 400000000000000;

    for (size_t i = 0; i < hailstones.size(); ++i) {
        auto &a = hailstones[i];

        for (size_t j = i + 1; j < hailstones.size(); ++j) {
            auto &b = hailstones[j];

            const double det = a.v[0] * (-b.v[1]) - (-b.v[0]) * a.v[1];
            if (fabs(det) <= 1e-9)
                continue;

            const double dx = b.p[0] - a.p[0];
            const double dy = b.p[1] - a.p[1];
            const double s = (-b.v[1] * dx + b.v[0] * dy) / det;
            const double t = (-a.v[1] * dx + a.v[0] * dy) / det;

            if (s > 0 && t > 0) {
                const auto c = a.p + s * a.v;
                count += (c[0] >= area_min && c[0] <= area_max) &&
                         (c[1] >= area_min && c[1] <= area_max);
            }
        }
    }

    return count;
}

static auto generate_lhs_rhs(const Hailstone &a, const Hailstone &b)
{
    const auto dp = a.p - b.p;
    const auto dv = a.v - b.v;

    Eigen::MatrixXd lhs{{
        {0, -dv[2], dv[1], 0, -dp[2], dp[1]},
        {dv[2], 0, -dv[0], dp[2], 0, -dp[0]},
        {-dv[1], dv[0], 0, -dp[1], dp[0], 0},
    }};

    Eigen::VectorXd rhs{{
        b.p[1] * b.v[2] - b.p[2] * b.v[1] - (a.p[1] * a.v[2] - a.p[2] * a.v[1]),
        b.p[2] * b.v[0] - b.p[0] * b.v[2] - (a.p[2] * a.v[0] - a.p[0] * a.v[2]),
        b.p[0] * b.v[1] - b.p[1] * b.v[0] - (a.p[0] * a.v[1] - a.p[1] * a.v[0]),
    }};

    return std::pair(lhs, rhs);
}

static int64_t part2(std::span<const Hailstone> hailstones)
{
    const auto [a01, b01] = generate_lhs_rhs(hailstones[0], hailstones[1]);
    const auto [a02, b02] = generate_lhs_rhs(hailstones[0], hailstones[2]);

    Eigen::MatrixXd a(a01.rows() + a02.rows(), a01.cols());
    a << a01, a02;

    Eigen::VectorXd b(b01.rows() + b02.rows());
    b << b01, b02;

    // Naively multiplying by the inverse instead of decomposing the matrix
    // yields the wrong answer when optimizations are enabled due to the huge
    // inputs :(
    Eigen::VectorXd x = a.fullPivLu().solve(b);
    return round(x[0]) + round(x[1]) + round(x[2]);
}

void run(std::string_view buf)
{
    std::vector<Hailstone> hailstones;

    for (auto &line : split_lines(buf)) {
        auto [x, y, z, vx, vy, vz] = find_numbers_n<int64_t, 6>(line);
        hailstones.emplace_back(
            Eigen::Vector3d{
                static_cast<double>(x),
                static_cast<double>(y),
                static_cast<double>(z),
            },
            Eigen::Vector3d{
                static_cast<double>(vx),
                static_cast<double>(vy),
                static_cast<double>(vz),
            });
    }

    fmt::print("{}\n", part1(hailstones));
    fmt::print("{}\n", part2(hailstones));
}

}
