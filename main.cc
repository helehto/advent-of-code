#include "common.h"
#include "config.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fmt/core.h>
#include <fnmatch.h>
#include <getopt.h>
#include <string_view>
#include <sys/mman.h>
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
    extern void run(std::string_view);                                                   \
    }
#define X_PROBLEM_TABLE_INITIALIZERS(year, day)                                          \
    {year, day, PROBLEM_NAMESPACE(year, day)::run},

#define ASSERT_ERRNO_MSG(expr, func) ASSERT_MSG(expr, #func ": {}", strerror(errno))

struct Problem {
    int year;
    int day;
    void (*func)(std::string_view);
};

struct Options {
    const char *input_file = nullptr;
    int iterations = 1;
    double target_time = -1;
    bool json = false;
    std::vector<const Problem *> problems_to_run;
};

X_FOR_EACH_PROBLEM(X_DECLARE_RUN_FUNCS)
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

struct ProblemData {
    int year;
    int day;
    std::vector<uint64_t> durations;
    std::string output;
};

static void redirect_stdout(int &memfd, int &original_stdout)
{
    fflush(stdout);

    if (original_stdout < 0) {
        original_stdout = dup(STDOUT_FILENO);
        ASSERT_ERRNO_MSG(original_stdout >= 0, "dup");
    }

    if (memfd >= 0) {
        ASSERT_ERRNO_MSG(lseek(memfd, 0, SEEK_SET) >= 0, "lseek");
        ASSERT_ERRNO_MSG(ftruncate(memfd, 0) >= 0, "ftruncate");
    } else {
        memfd = memfd_create("output", MFD_CLOEXEC);
        ASSERT_ERRNO_MSG(memfd >= 0, "memfd_create");
    }

    ASSERT_ERRNO_MSG(dup2(memfd, STDOUT_FILENO) == STDOUT_FILENO, "dup2");
}

static std::string restore_stdout(int memfd, int original_stdout)
{
    fflush(stdout);
    ASSERT_ERRNO_MSG(dup2(original_stdout, STDOUT_FILENO) == STDOUT_FILENO, "dup2");
    char buf[4096];
    ssize_t bytes_read = pread(memfd, buf, sizeof(buf), 0);
    ASSERT(bytes_read >= 0 && bytes_read < 4096);
    return std::string(buf, buf + bytes_read);
}

static std::string slurp(FILE *f)
{
    const int fd = fileno(f);
    const off_t size = lseek(fd, 0, SEEK_END);
    ASSERT(size > 0);

    std::string contents;
    contents.resize(size);
    ASSERT(pread(fd, contents.data(), size, 0) == size);

    return contents;
}

static std::pair<std::vector<uint64_t>, std::string>
run_problem(const Problem &p, std::string input_path, const Options &opts)
{
    using namespace std::chrono;

    std::string input;
    {
        FILE *f = fopen(input_path.c_str(), "r");
        if (!f) {
            fprintf(stderr, "%s: %s", input_path.c_str(), strerror(errno));
            exit(1);
        }
        input = slurp(f);

        while (isspace(input.back()))
            input.pop_back();

        fclose(f);
    }

    std::vector<uint64_t> durations;
    durations.reserve(opts.iterations);
    uint64_t total_duration = 0;

    auto run = [&] {
        const auto start = high_resolution_clock::now();
        p.func(input);
        const auto end = high_resolution_clock::now();
        uint64_t duration = duration_cast<nanoseconds>(end - start).count();
        durations.push_back(duration);
        total_duration += duration;
    };

    std::string output;
    if (opts.json) {
        // Capture the output of the first output if we're dumping JSON.
        static int memfd = -1;
        static int original_stdout = -1;
        redirect_stdout(memfd, original_stdout);
        run();
        output = restore_stdout(memfd, original_stdout);
    } else {
        run();
    }

    for (int i = 1; i < opts.iterations || total_duration < opts.target_time * 1e9; i++) {
        run();
    }

    return {durations, output};
}

template <>
struct fmt::formatter<ProblemData> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const ProblemData &p, FormatContext &ctx) const
    {
        auto out = ctx.out();

        const char *separator = "";
        out = fmt::format_to(out, "[{},{},[", p.year, p.day);
        for (auto &t : p.durations) {
            out = fmt::format_to(out, "{}{}", separator, t);
            separator = ",";
        }
        out = fmt::format_to(out, "],\"");

        for (char c : p.output) {
            if (c == '\n')
                out = fmt::format_to(out, "\\n");
            else if (isprint(c) && c != '"')
                out = fmt::format_to(out, "{}", c);
            else
                out = fmt::format_to(out, "\\x{:02x}", (uint8_t)c);
        }

        out = fmt::format_to(out, "\"]");
        return out;
    }
};

int main(int argc, char **argv)
{
    Options opts;

    while (true) {
        static struct option long_options[] = {
            {"input-file", required_argument, nullptr, 'f'},
            {"iterations", required_argument, nullptr, 'i'},
            {"json", no_argument, nullptr, 'j'},
            {"target-time", required_argument, nullptr, 't'},
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

    std::vector<ProblemData> timings;
    for (const auto *p : opts.problems_to_run) {
        auto input_path = opts.input_file
                              ? opts.input_file
                              : fmt::format("../inputs/input-{}-{}.txt", p->year, p->day);
        auto [times, output] = run_problem(*p, input_path, opts);
        timings.push_back({p->year, p->day, std::move(times), std::move(output)});
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
