#include "common.h"

namespace aoc_2017_20 {

struct Particle {
    std::array<int64_t, 3> p;
    std::array<int64_t, 3> v;
    std::array<int64_t, 3> a;
};

static int part1(std::span<const Particle> particles)
{
    constexpr static int64_t dt = 50'000;

    auto it = std::ranges::min_element(particles, {}, [&](const Particle &p) {
        const auto x = p.p[0] + dt * p.v[0] + dt * (dt + 1) * p.a[0] / 2;
        const auto y = p.p[1] + dt * p.v[1] + dt * (dt + 1) * p.a[1] / 2;
        const auto z = p.p[2] + dt * p.v[2] + dt * (dt + 1) * p.a[2] / 2;
        return std::abs(x) + std::abs(y) + std::abs(z);
    });

    return std::distance(particles.begin(), it);
}

static int part2(std::vector<Particle> &particles)
{
    auto step = [&] {
        for (Particle &p : particles) {
            p.v[0] += p.a[0];
            p.v[1] += p.a[1];
            p.v[2] += p.a[2];
            p.p[0] += p.v[0];
            p.p[1] += p.v[1];
            p.p[2] += p.v[2];
        }
    };

    std::vector<Particle> next;
    next.reserve(particles.size());

    auto collide = [&] {
        auto proj = [](const Particle &p) { return p.p; };
        std::ranges::sort(particles, {}, proj);

        next.clear();
        for (auto it = particles.begin(); it != particles.end();) {
            auto e = std::ranges::equal_range(it, particles.end(), it->p, {}, proj);
            const auto len = std::ranges::size(e);
            if (len == 1)
                next.push_back(*it);
            it += len;
        }
        std::swap(particles, next);
    };

    std::array<uint16_t, 16> hist{0};
    for (size_t i = 0;; i = (i + 1) & (hist.size() - 1)) {
        step();
        collide();

        if (std::ranges::all_of(hist, [&](uint16_t h) { return h == particles.size(); }))
            break;
        hist[i] = particles.size();
    }

    return particles.size();
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<Particle> particles;
    particles.reserve(lines.size());

    for (std::string_view line : lines) {
        auto nums = find_numbers_n<int, 9>(line);
        particles.push_back(Particle{
            .p = {nums[0], nums[1], nums[2]},
            .v = {nums[3], nums[4], nums[5]},
            .a = {nums[6], nums[7], nums[8]},
        });
    }

    fmt::print("{}\n", part1(particles));
    fmt::print("{}\n", part2(particles));
}

}
