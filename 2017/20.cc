#include "common.h"
#include "dense_map.h"

namespace aoc_2017_20 {

struct Particle {
    std::array<int64_t, 3> p;
    std::array<int64_t, 3> v;
    std::array<int64_t, 3> a;
};

static int part1(std::span<const Particle> particles)
{
    // Long enough to be considered "long-term", but small enough to avoid
    // overflowing (on my input, at least):
    constexpr static int64_t dt = 50'000;

    auto it = std::ranges::min_element(particles, {}, [&](const Particle &p) noexcept {
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

    dense_map<std::array<int64_t, 3>, size_t, CrcHasher> occupied;
    occupied.reserve(particles.size());
    std::vector<uint32_t> pending_deletion;
    pending_deletion.reserve(particles.size());

    auto collide = [&] {
        next.clear();
        occupied.clear();
        pending_deletion.clear();

        for (const Particle &p : particles) {
            if (auto [it, inserted] = occupied.try_emplace(p.p, next.size()); inserted)
                next.push_back(p);
            else if (!std::ranges::contains(pending_deletion, it->second))
                pending_deletion.push_back(it->second);
        }

        std::ranges::sort(pending_deletion);
        for (size_t i = pending_deletion.size(); i--;)
            erase_swap(next, pending_deletion[i]);

        std::swap(particles, next);
    };

    std::array<uint16_t, 16> hist{0};
    for (size_t i = 0;; i = (i + 1) & (hist.size() - 1)) {
        step();
        collide();

        if (std::ranges::all_of(hist, λx(x == particles.size())))
            break;
        hist[i] = particles.size();
    }

    return particles.size();
}

void run(std::string_view buf)
{
    auto nums = find_numbers<int>(buf);
    ASSERT(nums.size() % 9 == 0);

    std::vector<Particle> particles(nums.size() / 9);
    for (size_t i = 0; i < nums.size() / 9; i++) {
        particles[i] = Particle{
            .p = {nums[9 * i], nums[9 * i + 1], nums[9 * i + 2]},
            .v = {nums[9 * i + 3], nums[9 * i + 4], nums[9 * i + 5]},
            .a = {nums[9 * i + 6], nums[9 * i + 7], nums[9 * i + 8]},
        };
    }

    fmt::print("{}\n", part1(particles));
    fmt::print("{}\n", part2(particles));
}

}
