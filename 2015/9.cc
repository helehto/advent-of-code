#include "common.h"
#include "dense_map.h"

namespace aoc_2015_9 {

static Matrix<int> get_distance_matrix(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    int n = 0;

    dense_map<std::string_view, int> name_map;
    std::vector<std::string_view> words;
    for (auto line : lines) {
        split(line, words, ' ');
        if (auto [it, inserted] = name_map.emplace(words[0], n); inserted)
            n++;
        if (auto [it, inserted] = name_map.emplace(words[2], n); inserted)
            n++;
    }

    Matrix<int> dist(n, n);
    for (auto line : lines) {
        split(line, words, ' ');
        auto i = name_map.at(words[0]);
        auto j = name_map.at(words[2]);
        int distance = -1;
        auto r = std::from_chars(begin(words[4]), end(words[4]), distance);
        ASSERT(r.ec == std::errc());
        dist(i, j) = distance;
        dist(j, i) = distance;
    }

    return dist;
}

static int evaluate_route(const Matrix<int> &dist, std::span<int> perm)
{
    int cost = 0;

    for (size_t i = 1; i < perm.size(); i++)
        cost += dist(perm[i - 1], perm[i]);

    return cost;
}

template <typename Compare>
static int solve(const Matrix<int> &dist, Compare comp)
{
    std::vector<int> perm;
    perm.resize(dist.rows);
    for (size_t i = 0; i < perm.size(); i++)
        perm[i] = i;

    int cost = evaluate_route(dist, perm);
    while (std::next_permutation(begin(perm), end(perm)))
        cost = std::min(cost, evaluate_route(dist, perm), comp);

    return cost;
}

void run(FILE *f)
{
    const auto dist = get_distance_matrix(f);
    fmt::print("{}\n", solve(dist, std::less<>()));
    fmt::print("{}\n", solve(dist, std::greater<>()));
}

}
