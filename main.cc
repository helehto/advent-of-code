#include "config.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fmt/core.h>
#include <fnmatch.h>
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

#define PROBLEM_NAMESPACE(year, day) GLUE(aoc_, GLUE3(year, _, day))

#define X_DECLARE_RUN_FUNCS(year, day)                                                   \
    namespace GLUE(aoc_, GLUE3(year, _, day)) {                                          \
    extern void run(FILE *);                                                             \
    }
#define X_PROBLEM_TABLE_INITIALIZERS(year, day)                                          \
    {year, day, PROBLEM_NAMESPACE(year, day)::run},

struct Problem {
    int year;
    int day;
    void (*func)(FILE *);
};

struct Options {
    const char *input_file = nullptr;
    int iterations = 1;
    double target_time = -1;
    bool json = false;
    std::vector<const Problem *> problems_to_run;
};

X_FOR_EACH_PROBLEM(X_DECLARE_RUN_FUNCS);
static constexpr Problem problems[] = {X_FOR_EACH_PROBLEM(X_PROBLEM_TABLE_INITIALIZERS)};

static std::vector<const Problem *> glob_problem(const char *pattern)
{
    std::vector<const Problem *> result;

    for (const auto &p : problems) {
        char s[64];
        snprintf(s, sizeof(s), "%d/%d", p.year, p.day);
        if (fnmatch(pattern, s, 0) == 0)
            result.push_back(&p);
    }

    return result;
}

static std::vector<uint64_t>
run_problem(const Problem &p, std::string input_path, const Options &opts)
{
    using namespace std::chrono;

    FILE *f = fopen(input_path.c_str(), "r");

    std::vector<uint64_t> durations;
    durations.reserve(opts.iterations);
    uint64_t total_duration = 0;

    auto run = [&] {
        const auto start = high_resolution_clock::now();
        p.func(f);
        const auto end = high_resolution_clock::now();
        uint64_t duration = duration_cast<nanoseconds>(end - start).count();
        durations.push_back(duration);
        total_duration += duration;
    };

    run();
    for (int i = 1; i < opts.iterations || total_duration < opts.target_time * 1e9; i++) {
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
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

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
    Options opts;

    while (true) {
        static struct option long_options[] = {
            {"input-file", required_argument, 0, 'f'},
            {"iterations", required_argument, 0, 'i'},
            {"json", no_argument, 0, 'j'},
            {"target-time", required_argument, 0, 't'},
        };

        int option_index;
        int c = getopt_long(argc, argv, "f:i:jt:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'f':
            opts.input_file = optarg;
            break;
        case 'i':
            opts.iterations = atoi(optarg);
            assert(opts.iterations > 0);
            break;
        case 'j':
            opts.json = true;
            break;
        case 't':
            opts.target_time = strtod(optarg, nullptr);
            if (opts.target_time <= 0)
                die("invalid target time '%s'", optarg);
            break;
        }
    }

    for (int i = optind; i < argc; i++) {
        std::vector<const Problem *> problems = glob_problem(argv[i]);
        if (problems.empty())
            die("invalid problem or pattern '%s'", argv[i]);
        opts.problems_to_run.insert(end(opts.problems_to_run), begin(problems),
                                    end(problems));
    }
    if (opts.problems_to_run.empty())
        die("no problems specified");

    std::vector<TimingData> timings;
    for (const auto *p : opts.problems_to_run) {
        auto input_path = opts.input_file
                              ? opts.input_file
                              : fmt::format("../inputs/input-{}-{}.txt", p->year, p->day);
        timings.push_back({p->year, p->day, run_problem(*p, input_path, opts)});
    }

    if (opts.json) {
        const char *separator = "";
        fmt::print("[");
        for (auto &td : timings) {
            fmt::print("{}{}", separator, td);
            separator = ",";
        }
        fmt::print("]\n");
    }
}
