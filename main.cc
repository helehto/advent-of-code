#include "common.h"
#include "thread_pool.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fmt/core.h>
#include <fnmatch.h>
#include <getopt.h>
#include <optional>
#include <string_view>
#include <sys/mman.h>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <vector>

using namespace std::literals;
using namespace std::chrono_literals;
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
    bool stable_mode = false;

    // How many seconds and/or iterations to run. Run until both
    // `min_iterations` and `min_duration` have been reached, and continue
    // until either `max_iterations` or `max_duration` has been reached.
    int64_t min_iterations = 1;
    int64_t max_iterations = INT64_MAX;
    std::chrono::nanoseconds min_duration = 0ns;
    std::chrono::nanoseconds max_duration = std::chrono::nanoseconds::max();

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
    std::vector<std::chrono::nanoseconds> durations;
    std::string output;
};

static std::string format_answer(const aoc::Answer &a)
{
    return a.num_parts > 1 ? fmt::format("{}\n{}\n", a.part1, a.part2)
                           : fmt::format("{}\n", a.part1);
}

static std::string slurp(int fd)
{
    std::string contents;
    while (true) {
        char buf[16384];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0)
            die("read: %s", strerror(errno));
        if (n == 0)
            break;
        contents.append(buf, n);
    }
    return contents;
}

static std::pair<std::vector<std::chrono::nanoseconds>, std::string>
run_solver(const Problem &s, std::string input_path, const Options &opts)
{
    using namespace std::chrono;

    std::string input;
    {
        int fd = input_path == "-" ? STDIN_FILENO
                                   : open(input_path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            die("%s: %s", input_path.c_str(), strerror(errno));
        input = slurp(fd);
        if (fd != STDIN_FILENO)
            close(fd);

        while (input.back() == '\n')
            input.pop_back();
    }

    std::vector<std::chrono::nanoseconds> durations;
    durations.reserve(opts.iterations);
    auto total_duration = 0ns;
    aoc::Answer answer;
    std::optional<aoc::Answer> reference;

    auto run = [&] {
        answer.clear();
        const auto start = high_resolution_clock::now();
        s.run(input, answer);
        const auto end = high_resolution_clock::now();
        auto duration = end - start;
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
        constexpr auto warmup_duration = 20ms; // 20 ms
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
        size_t min_iterations = std::max<size_t>(opts.min_iterations, N);

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

            auto sum = std::ranges::fold_left(batch, 0ns, λab(a + b));
            auto min = std::ranges::min(batch);
            auto mean = std::chrono::duration<double>(sum) / double(N);

            double ratio = mean / min;
            ++num_batches;
            if (total_duration >= opts.min_duration &&
                num_batches * N >= min_iterations &&
                (ratio <= 1.05 || total_duration >= opts.max_duration ||
                 num_batches * N >= static_cast<uint64_t>(opts.max_iterations))) {
                break;
            }
        }
    } else {
        size_t iterations = 1;
        while ((total_duration < opts.min_duration ||
                iterations < static_cast<uint64_t>(opts.min_iterations)) &&
               total_duration < opts.max_duration &&
               iterations < static_cast<uint64_t>(opts.max_iterations)) {
            run();
            ++iterations;
        }
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
            out = fmt::format_to(out, "{}{}", separator, t / 1ns);
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
            {"jobs", no_argument, nullptr, 'j'},
            {"json", no_argument, nullptr, 'J'},
            {"min-iterations", required_argument, nullptr, 'i'},
            {"max-iterations", required_argument, nullptr, 'I'},
            {"min-duration", required_argument, nullptr, 't'},
            {"max-duration", required_argument, nullptr, 'T'},
            {"stable", no_argument, nullptr, 's'},
        };

        int option_index;
        int c = getopt_long(argc, argv, "f:i:I:j:Jst:T:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'f':
            opts.input_file = optarg;
            break;
        case 'i': {
            std::string_view arg(optarg);
            auto r =
                std::from_chars(arg.data(), arg.data() + arg.size(), opts.min_iterations);
            if (r.ec != std::errc() || r.ptr != arg.data() + arg.size() ||
                opts.min_iterations <= 0)
                die("invalid minimum number of iterations '%s'", optarg);
            break;
        }
        case 'I': {
            std::string_view arg(optarg);
            auto r =
                std::from_chars(arg.data(), arg.data() + arg.size(), opts.max_iterations);
            if (r.ec != std::errc() || r.ptr != arg.data() + arg.size() ||
                opts.max_iterations <= 0)
                die("invalid maximum number of iterations '%s'", optarg);
            break;
        }
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
        case 't': {
            std::string_view arg(optarg);
            double min_duration_sec;
            auto r =
                std::from_chars(arg.data(), arg.data() + arg.size(), min_duration_sec);
            if (r.ec != std::errc() || r.ptr != arg.data() + arg.size() ||
                min_duration_sec < 0)
                die("invalid minimum duration '%s'", optarg);
            opts.min_duration = std::chrono::nanoseconds(
                static_cast<int64_t>(floor(min_duration_sec * 1e9)));
            break;
        }
        case 'T': {
            std::string_view arg(optarg);
            double max_duration_sec;
            auto r =
                std::from_chars(arg.data(), arg.data() + arg.size(), max_duration_sec);
            if (r.ec != std::errc() || r.ptr != arg.data() + arg.size() ||
                max_duration_sec < 0)
                die("invalid maximum duration '%s'", optarg);
            opts.max_duration = std::chrono::nanoseconds(
                static_cast<int64_t>(floor(max_duration_sec * 1e9)));
            break;
        }
        }
    }

    if (opts.min_iterations > opts.max_iterations)
        die("min-iterations cannot be greater than max-iterations");
    if (opts.min_duration > opts.max_duration)
        die("min-duration cannot be greater than max-duration");

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
