#include "config.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fmt/core.h>
#include <getopt.h>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

#define die(fmt, ...)                                                                    \
    do {                                                                                 \
        fprintf(stderr, "error: " fmt "\n" __VA_OPT__(, ) __VA_ARGS__);                  \
        exit(EXIT_FAILURE);                                                              \
    } while (0)

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

static constexpr const Problem *lookup_problem(int year, int day)
{
    for (const auto &p : problems) {
        if (year == p.year && day == p.day)
            return &p;
    }

    return nullptr;
}

static std::vector<uint64_t> run_problem(const Problem &p, int iterations)
{
    using namespace std::chrono;

    auto input_path = fmt::format("../inputs/input-{}-{}.txt", p.year, p.day);
    FILE *f = fopen(input_path.c_str(), "r");

    std::vector<uint64_t> durations;
    durations.reserve(iterations);

    auto run = [&] {
        const auto start = high_resolution_clock::now();
        p.func(f);
        const auto end = high_resolution_clock::now();
        durations.push_back(duration_cast<nanoseconds>(end - start).count());
    };

    run();
    for (int i = 1; i < iterations; i++) {
        rewind(f);
        run();
    }

    fclose(f);

    return durations;
}

struct TimingData {
    int year;
    int day;
    std::vector<uint64_t> durations;
};

template <>
struct fmt::formatter<TimingData> {
    constexpr auto parse(format_parse_context &ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const TimingData &td, FormatContext &ctx) const
    {
        auto out = ctx.out();

        const char *separator = "";
        out = fmt::format_to(out, "[{},{},[", td.year, td.day);
        for (auto &t : td.durations) {
            out = fmt::format_to(out, "{}{}", separator, t);
            separator = ",";
        }
        out = fmt::format_to(out, "]]");

        return out;
    }
};

int main(int argc, char **argv)
{
    bool read_from_stdin = false;
    int iterations = 1;

    while (true) {
        static struct option long_options[] = {
            {"iterations", required_argument, 0, 'i'},
            {"stdin", no_argument, 0, 's'},
        };

        int option_index;
        int c = getopt_long(argc, argv, "i:st", long_options, &option_index);
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
            break;
        }
    }

    std::vector<const Problem *> problems_to_run;
    for (int i = optind; i < argc; i++) {
        int y = 0, d = 0;
        const Problem *p;
        if (argv[i] == "all"sv) {
            for (auto &p : problems)
                problems_to_run.push_back(&p);
        } else if (sscanf(argv[i], "%d/%d", &y, &d) != 2 || !(p = lookup_problem(y, d))) {
            die("invalid problem %s", argv[i]);
        } else {
            problems_to_run.push_back(p);
        }
    }

    if (read_from_stdin) {
        if (problems_to_run.size() > 1)
            die("can only specify one problem with -s, got %zu", problems_to_run.size());
        run_problem(*problems_to_run[0], 1);
        return 0;
    }

    std::vector<TimingData> timings;
    for (const auto *p : problems_to_run)
        timings.push_back({p->year, p->day, run_problem(*p, iterations)});

    const char *separator = "";
    fmt::print("[");
    for (auto &td : timings) {
        fmt::print("{}{}", separator, td);
        separator = ",";
    }
    fmt::print("]\n");
}
