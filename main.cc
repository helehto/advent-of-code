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

#define GLUE_(x, y) x##y
#define GLUE(x, y) GLUE_(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

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
run_problem(const Problem &p, std::string input_path, int iterations)
{
    using namespace std::chrono;

    FILE *f = input_path.empty() ? stdin : fopen(input_path.c_str(), "r");

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
    const char *input_file = nullptr;
    bool read_from_stdin = false;
    int iterations = 1;

    while (true) {
        static struct option long_options[] = {
            {"input-file", required_argument, 0, 'f'},
            {"iterations", required_argument, 0, 'i'},
            {"stdin", no_argument, 0, 's'},
        };

        int option_index;
        int c = getopt_long(argc, argv, "f:i:st", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'f':
            input_file = optarg;
            break;
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
        std::vector<const Problem *> problems = glob_problem(argv[i]);
        if (problems.empty())
            die("invalid problem or pattern '%s'", argv[i]);
        problems_to_run.insert(end(problems_to_run), begin(problems), end(problems));
    }
    if (problems_to_run.empty())
        die("no problems specified");

    if (read_from_stdin) {
        if (problems_to_run.size() > 1)
            die("can only specify one problem with -s, got %zu", problems_to_run.size());
        run_problem(*problems_to_run[0], "", 1);
        return 0;
    }

    std::vector<TimingData> timings;
    for (const auto *p : problems_to_run) {
        auto input_path = input_file
                              ? input_file
                              : fmt::format("../inputs/input-{}-{}.txt", p->year, p->day);
        timings.push_back({p->year, p->day, run_problem(*p, input_path, iterations)});
    }

    const char *separator = "";
    fmt::print("[");
    for (auto &td : timings) {
        fmt::print("{}{}", separator, td);
        separator = ",";
    }
    fmt::print("]\n");
}
