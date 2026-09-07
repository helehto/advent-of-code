#pragma once

#include <aoc/macros.h>
#include <fmt/base.h>
#include <iterator>
#include <string>
#include <string_view>

// Machinery for solver registration.
namespace aoc {

// Formatted output from a solver. Most solvers output two solutions, with the
// exception being the final day of the year which only outputs one.
struct Answer {
    std::string part1;
    std::string part2;
    int num_parts = 0;

    void clear()
    {
        part1.clear();
        part2.clear();
        num_parts = 0;
    }

    void add_vformatted(fmt::string_view fmt, fmt::format_args args)
    {
        if (num_parts == 0)
            fmt::vformat_to(std::back_inserter(part1), fmt, args);
        else if (num_parts == 1)
            fmt::vformat_to(std::back_inserter(part2), fmt, args);
        else
            ASSERT(false);
        num_parts++;
    }

    template <typename... T>
    [[gnu::noinline]] void add_formatted(fmt::format_string<T...> fmt, T &&...args)
    {
        add_vformatted(fmt.get(), fmt::make_format_args(args...));
    }

    template <typename T>
    [[gnu::noinline]] void add(T &&value)
    {
        add_formatted("{}", static_cast<T &&>(value));
    }
};

struct Problem {
    int year;
    int day;
    void (*run)(std::string_view, aoc::Answer &answer);
};

// Define and register a solver for a given year and day. Must be used inside a
// namespace, with the function body following the macro invocation.
// clang-format off
#define AOC_REGISTER_SOLVER(y, d, f)                           \
    __attribute__((used, retain, section("aoc_solvers")))      \
    constinit extern const ::aoc::Problem _solver = {y, d, f}; \
    static_assert(true, "You forgot a semicolon! --> ")
// clang-format on

} // namespace aoc
extern const aoc::Problem __start_aoc_solvers[];
extern const aoc::Problem __stop_aoc_solvers[];
