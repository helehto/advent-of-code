#include "common.h"
#include "thread_pool.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fmt/core.h>
#include <fnmatch.h>
#include <getopt.h>
#include <optional>
#include <string_view>
#include <sys/mman.h>
#include <thread>
#include <tuple>
#include <vector>

using namespace std::literals;
using Problem = aoc::Problem;

#define die(fmt, ...)                                                                    \
    do {                                                                                 \
        fprintf(stderr, "error: " fmt "\n" __VA_OPT__(, ) __VA_ARGS__);                  \
        exit(EXIT_FAILURE);                                                              \
    } while (0)

#define ASSERT_ERRNO_MSG(expr, func) ASSERT_MSG(expr, #func ": {}", strerror(errno))

struct Options {
    const char *input_file = nullptr;
    int iterations = 1;
    int num_threads = 0;
    double target_time = -1;
    bool stable_mode = false;
    bool json = false;
    std::vector<Problem> problems_to_run;
};

static std::vector<Problem> glob_problem(std::span<const Problem> problems,
                                         const char *pattern)
{
    std::vector<Problem> result;
    result.reserve(problems.size());

    for (const auto &p : problems) {
        char s[64];
        snprintf(s, sizeof(s), "%d/%d", p.year, p.day);
        if (fnmatch(pattern, s, 0) == 0)
            result.push_back(p);
    }

    return result;
}

struct ProblemData {
    int year;
    int day;
    std::vector<uint64_t> durations;
    std::string output;
};

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

static std::string format_answer(const aoc::Answer &a)
{
    return a.num_parts > 1 ? fmt::format("{}\n{}\n", a.part1, a.part2)
                           : fmt::format("{}\n", a.part1);
}

static std::pair<std::vector<uint64_t>, std::string>
run_solver(const Problem &s, std::string input_path, const Options &opts)
{
    using namespace std::chrono;

    std::string input;
    {
        FILE *f = fopen(input_path.c_str(), "r");
        if (!f)
            die("%s: %s", input_path.c_str(), strerror(errno));
        input = slurp(f);

        while (input.back() == '\n')
            input.pop_back();

        fclose(f);
    }

    std::vector<uint64_t> durations;
    durations.reserve(opts.iterations);
    uint64_t total_duration = 0;
    aoc::Answer answer;
    std::optional<aoc::Answer> reference;

    auto run = [&] {
        answer.clear();
        const auto start = high_resolution_clock::now();
        s.run(input, answer);
        const auto end = high_resolution_clock::now();
        uint64_t duration = duration_cast<nanoseconds>(end - start).count();
        durations.push_back(duration);
        total_duration += duration;

        // Verify that the solver produces the same output every time.
        if (!reference)
            reference = answer;
        if (answer.num_parts != reference->num_parts ||
            answer.part1 != reference->part1 || answer.part2 != reference->part2) {
            die("%d/%d: non-deterministic output\n"
                "--- iteration %zu:\n%s"
                "--- iteration %zu:\n%s",
                s.year, s.day, durations.size() - 1, format_answer(*reference).c_str(),
                durations.size(), format_answer(answer).c_str());
        }
    };

    run();
    std::string output = format_answer(*reference);
    if (!opts.json)
        fmt::print("{}", output);

    if (opts.stable_mode) {
        // Run for `warmup_duration` or `warmup_iterations` iterations to warm
        // up, whichever is longer, to determine the batch size.
        constexpr uint64_t warmup_duration = 20'000'000; // 20 ms
        constexpr size_t warmup_iterations = 3;
        while (total_duration < warmup_duration || durations.size() < warmup_iterations)
            run();

        // Run in batches of N runs until the timing of the last N runs look
        // (somewhat) stable, or the time spent exceeds what was given in -t.
        // The batch size is chosen to be ~50 ms based on the warmup runs, or
        // at least two runs.
        const size_t N = std::max<size_t>(
            warmup_iterations, (warmup_duration * durations.size()) / total_duration);
        size_t num_batches = 0;

        while (true) {
            for (size_t i = 0; i < N; i++)
                run();

            std::span batch(durations.end() - N, N);

            // If we have sufficiently many runs, discard the top 10% to avoid
            // outliers causing us to run for longer than necessary.
            if (N >= 20) {
                auto first_outlier = batch.end() - (N + 9) / 10;
                std::ranges::nth_element(batch, first_outlier);
                batch = batch.subspan(0, first_outlier - batch.begin());
            }

            uint64_t sum = std::ranges::fold_left(batch, UINT64_C(0), λab(a + b));
            uint64_t min = std::ranges::min(batch);
            auto mean = double(sum) / double(N);

            double ratio = mean / min;
            DV(min, mean, ratio);
            ++num_batches;
            if (ratio <= 1.05 ||
                (opts.target_time > 0 && total_duration >= opts.target_time * 1e9)) {
                DV(num_batches);
                break;
            }
        }
    } else if (opts.target_time > 0) {
        while (total_duration < opts.target_time * 1e9)
            run();
    } else {
        for (int i = 1; i < opts.iterations; i++)
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
            {"jobs", no_argument, nullptr, 'j'},
            {"json", no_argument, nullptr, 'J'},
            {"target-time", required_argument, nullptr, 't'},
            {"stable", no_argument, nullptr, 's'},
        };

        int option_index;
        int c = getopt_long(argc, argv, "f:i:j:Jst:", long_options, &option_index);
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
            opts.num_threads = atoi(optarg);
            assert(opts.num_threads >= 0);
            break;
        case 'J':
            opts.json = true;
            break;
        case 's':
            opts.stable_mode = true;
            break;
        case 't':
            opts.target_time = strtod(optarg, nullptr);
            if (opts.target_time <= 0)
                die("invalid target time '%s'", optarg);
            break;
        }
    }

    // Copy the table of solvers and sort it, since its order is not guaranteed
    // (depends on the link order).
    small_vector<Problem, 512> all_problems(__start_aoc_solvers, __stop_aoc_solvers);
    std::ranges::sort(all_problems, {}, λa(std::pair(a.year, a.day)));

    for (int i = optind; i < argc; i++) {
        std::vector<Problem> problems = glob_problem(all_problems, argv[i]);
        if (problems.empty())
            die("invalid problem or pattern '%s'", argv[i]);
        opts.problems_to_run.insert(end(opts.problems_to_run), begin(problems),
                                    end(problems));
    }
    if (opts.problems_to_run.empty())
        die("no problems specified");

    ThreadPool::get().start(opts.num_threads > 0 ? opts.num_threads
                                                 : std::thread::hardware_concurrency());

    std::vector<ProblemData> timings;
    for (const Problem &p : opts.problems_to_run) {
        auto input_path = opts.input_file
                              ? opts.input_file
                              : fmt::format("../inputs/input-{}-{}.txt", p.year, p.day);
        auto [times, output] = run_solver(p, input_path, opts);
        timings.push_back({p.year, p.day, std::move(times), std::move(output)});
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
