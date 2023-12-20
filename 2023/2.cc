#include "common.h"

struct Outcome {
    int r;
    int g;
    int b;
};

static void strip(std::string_view &sv)
{
    while (!sv.empty() && isspace(sv.front()))
        sv.remove_prefix(1);
    while (!sv.empty() && isspace(sv.back()))
        sv.remove_suffix(1);
}

void run_2023_2(FILE *f)
{
    auto [buf,lines]=slurp_lines(f);
    std::vector<std::vector<Outcome>> games;
    std::vector<std::string_view> tmp;
    std::vector<std::string_view> tmp2;

    for (std::string_view sv : lines) {
        sv.remove_prefix(sv.find(": ") + 2);

        std::vector<Outcome> outcomes;
        for (std::string_view game : split(sv, tmp, ';')) {
            Outcome outcome{};
            strip(game);

            for (std::string_view c : split(game, tmp2, ',')) {
                strip(c);
                int n;
                std::from_chars(c.begin(), c.end(), n);
                std::string_view color = c.substr(c.find(' ') + 1);
                if (color == "red")
                    outcome.r = n;
                else if (color == "blue")
                    outcome.b = n;
                else if (color == "green")
                    outcome.g = n;
            }

            outcomes.push_back(outcome);
        }

        games.push_back(std::move(outcomes));
    }

    int sum = 0;
    for (int id = 1; auto &game : games) {
        for (auto &outcome : game) {
            if (outcome.r > 12 || outcome.g > 13 || outcome.b > 14)
                goto next;
        }
        sum += id;
    next:
        id++;
    }
    fmt::print("{}\n", sum);

    int power_sum = 0;
    for (auto &game : games) {
        int maxr = 0, maxg = 0, maxb = 0;
        for (auto &outcome : game) {
            maxr = std::max(maxr, outcome.r);
            maxg = std::max(maxg, outcome.g);
            maxb = std::max(maxb, outcome.b);
        }
        power_sum += maxr * maxg * maxb;
    }
    fmt::print("{}\n", power_sum);
}
