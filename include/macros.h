/// This file contains a bunch of useful preprocessor macros and hackery.

#pragma once

#include <cstdio>
#include <fmt/core.h>
#include <fmt/ranges.h>

// The STRINGIFY() macro expands its argument into a string literal.
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

// The GLUE() and GLUE3() macros pastes two or three tokens, respectively, into
// a single token.
#define GLUE_(x, y) x##y
#define GLUE(x, y) GLUE_(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

/// Assertion macro which is always active, regardless of whether NDEBUG is
/// defined.
#define ASSERT(x)                                                                        \
    do {                                                                                 \
        if (!(x)) [[unlikely]] {                                                         \
            /* Capture __PRETTY_FUNCTION__ outside of the lambda; inside it will report  \
             * "operator()" which is not very helpful. */                                \
            const char *_pretty_function = __PRETTY_FUNCTION__;                          \
                                                                                         \
            /* Mark the lambda as cold and noinline to encourage the compiler to move it \
             * out of line from the main code. This reduces icache impact. */            \
            [&] [[gnu::cold, gnu::noinline]] () {                                        \
                fflush(stdout);                                                          \
                fprintf(stderr,                                                          \
                        "\x1b[1;31m" __FILE__                                            \
                        ":" STRINGIFY(__LINE__) ": %s: Assertion `%s' failed.\x1b[m\n",  \
                        _pretty_function, #x);                                           \
            }();                                                                         \
                                                                                         \
            /* Trap instead of using abort() so that we end up breaking at the exact     \
             * location of the assertion that tripped when running under a debugger. */  \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

/// Assertion macro which is always active, regardless of whether NDEBUG is
/// defined. This one supports printing a custom message (for e.g. printin the
/// state of the offending value(s)).
#define ASSERT_MSG(x, format, ...)                                                       \
    do {                                                                                 \
        if (!(x)) [[unlikely]] {                                                         \
            const char *_pretty_function = __PRETTY_FUNCTION__;                          \
            [&] [[gnu::cold, gnu::noinline]] () {                                        \
                fflush(stdout);                                                          \
                fprintf(stderr,                                                          \
                        "\x1b[1;31m" __FILE__                                            \
                        ":" STRINGIFY(__LINE__) ": %s: Assertion `%s' failed:\x1b[m ",   \
                        _pretty_function, #x);                                           \
                fmt::print(stderr, format "\n" __VA_OPT__(, ) __VA_ARGS__);              \
            }();                                                                         \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

/// DEBUG_ASSERT and DEBUG_ASSERT_MSG are assertion macros active iff NDEBUG is
/// not defined, like assert() in <cassert>.
#ifdef NDEBUG
#define DEBUG_ASSERT(x) __attribute__((assume(x)))
#define DEBUG_ASSERT_MSG(x, format, ...) __attribute__((assume(x)))
#else
#define DEBUG_ASSERT(...) ASSERT(__VA_ARGS__)
#define DEBUG_ASSERT_MSG(...) ASSERT_MSG(__VA_ARGS__)
#endif

// The ARG_COUNT() variadic macro counts the number of arguments passed to it.
#define ARG_COUNT_(x31, x30, x29, x28, x27, x26, x25, x24, x23, x22, x21, x20, x19, x18, \
                   x17, x16, x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3,   \
                   x2, x1, x0, ...)                                                      \
    x0
#define ARG_COUNT(...)                                                                   \
    ARG_COUNT_(__VA_ARGS__, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,  \
               16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

// Implementation details of the DV() macro below.
#define DV_FORMAT_1(x) #x " = \x1b[1m{}\x1b[m"
#define DV_FORMAT_2(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_1(__VA_ARGS__)
#define DV_FORMAT_3(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_2(__VA_ARGS__)
#define DV_FORMAT_4(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_3(__VA_ARGS__)
#define DV_FORMAT_5(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_4(__VA_ARGS__)
#define DV_FORMAT_6(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_5(__VA_ARGS__)
#define DV_FORMAT_7(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_6(__VA_ARGS__)
#define DV_FORMAT_8(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_7(__VA_ARGS__)
#define DV_FORMAT(...) GLUE(DV_FORMAT_, ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

/// The DV() macro dumps the expressions and the values of the expressions
/// passed to it to stdout, along with the line number. Intended as a quick and
/// short-term debugging utility.
#define DV(var, ...)                                                                     \
    fmt::print(                                                                          \
        "\x1b[1;30;43m {:>4d} \x1b[m " DV_FORMAT(var __VA_OPT__(, ) __VA_ARGS__) "\n",   \
        __LINE__, var __VA_OPT__(, ) __VA_ARGS__)

// Implementation details of the λ() macro below.
#define λ_1(a, body) [&]() noexcept { return body; }
#define λ_2(a, body) [&]([[maybe_unused]] auto &&a) noexcept { return body; }
#define λ_3(a, b, body)                                                                  \
    [&]([[maybe_unused]] auto &&a, [[maybe_unused]] auto &&b) noexcept { return body; }
#define λ_4(a, b, c, body)                                                               \
    [&]([[maybe_unused]] auto &&a, [[maybe_unused]] auto &&b,                            \
        [[maybe_unused]] auto &&c) noexcept { return body; }
#define λ_5(a, b, c, d, body)                                                            \
    [&]([[maybe_unused]] auto &&a, [[maybe_unused]] auto &&b, [[maybe_unused]] auto &&c, \
        [[maybe_unused]] auto &&d) noexcept { return body; }
#define λ_(...) GLUE(λ_, ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

// Concise form of a common pattern of lambda functions; particularly useful
// for one-off lambdas in algorithms, e.g. `find(range, λ(x, x >= 42))`. It
// takes a number of names to bind the arguments to, followed by an expression
// which will be the body of a lambda taking the given number of arguments.
#define λ(...) λ_(__VA_ARGS__)

// Even more concise form of λ() for a few different common argument names and
// arities, e.g. `find(range, λx(x >= 42))` or `sort(range, λab(a > b))`.
#define λa(body) λ(a, body)
#define λx(body) λ(x, body)
#define λab(body) λ(a, b, body)
#define λxy(body) λ(x, y, body)
