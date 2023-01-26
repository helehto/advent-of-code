#include "config.h"
#include <cassert>
#include <cstdio>
#include <fmt/core.h>
#include <getopt.h>
#include <string_view>

using namespace std::literals;

#define GLUE_(x, y) x ## y
#define GLUE(x, y) GLUE_(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define PROBLEM_FUNC_NAME(year, day) GLUE(run_, GLUE3(year, _, day))

#define X_DECLARE_RUN_FUNCS(year, day) extern void PROBLEM_FUNC_NAME(year, day)(FILE *);
#define X_PROBLEM_TABLE_INITIALIZERS(year, day) {year, day, PROBLEM_FUNC_NAME(year, day)},

struct Problem {
    int year;
    int day;
    void (*func)(FILE *);
};

X_FOR_EACH_PROBLEM(X_DECLARE_RUN_FUNCS);
static constexpr Problem problems[] = {
    X_FOR_EACH_PROBLEM(X_PROBLEM_TABLE_INITIALIZERS)
};

static const Problem *lookup_problem(int year, int day)
{
    for (auto &p : problems) {
        if (year == p.year && day == p.day)
            return &p;
    }

    return nullptr;
}

static void run(const Problem &p, int iterations)
{
    auto input_path = fmt::format("../inputs/input-{}-{}.txt", p.year, p.day);
    FILE *f = fopen(input_path.c_str(), "r");

    p.func(f);
    for (int i = 1; i < iterations; i++) {
        rewind(f);
        p.func(f);
    }

    fclose(f);
}

int main(int argc, char **argv)
{
    bool read_from_stdin = false;
    int iterations = 1;
    int year = 2022;
    const char *timing_output = nullptr;

    while (true) {
        static struct option long_options[] = {
            {"iterations", required_argument, 0, 'i'},
            {"stdin", no_argument, 0, 's'},
            {"timing-output", required_argument, 0, 't'},
            {"year", required_argument, 0, 'y'},
        };

        int option_index;
        int c = getopt_long(argc, argv, "i:st:y:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'i':
            iterations = atoi(optarg);
            assert(iterations > 0);
            break;
        case 's':
            read_from_stdin = true;
            break;
        case 't':
            timing_output = optarg;
            break;
        case 'y':
            year = atoi(optarg);
            assert(year >= 2015);
            break;
        }
    }

    if (optind + 1 != argc) {
        return 1;
    }

    int day = -1;
    if (argv[optind] == "all"sv) {
        day = -1;
    } else if ((day = atoi(argv[optind])) <= 0 || day >= 25) {
        fprintf(stderr, "bad problem %s\n", argv[optind]);
        return 1;
    }

    if (day == -1) {
        for (auto &p : problems)
            run(p, iterations);
    } else {
        const auto *p = lookup_problem(year, day);
        if (p == nullptr)
            fprintf(stderr, "bad problem %s\n", argv[optind]);
        if (read_from_stdin)
            p->func(stdin);
        else
            run(*p, iterations);
    }

    (void)timing_output;
}
