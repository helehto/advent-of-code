#pragma once

#include <cstdio>
#include <fmt/core.h>
#include <fmt/ranges.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define GLUE_(x, y) x##y
#define GLUE(x, y) GLUE_(x, y)
#define GLUE3(x, y, z) GLUE(GLUE(x, y), z)

#define ASSERT(x)                                                                        \
    do {                                                                                 \
        if (!(x)) [[unlikely]] {                                                         \
            const char *_pretty_function = __PRETTY_FUNCTION__;                          \
            [&] [[gnu::cold, gnu::noinline]] () {                                        \
                fflush(stdout);                                                          \
                fprintf(stderr,                                                          \
                        "\x1b[1;31m" __FILE__                                            \
                        ":" STRINGIFY(__LINE__) ": %s: Assertion `%s' failed.\x1b[m\n",  \
                        _pretty_function, #x);                                           \
            }();                                                                         \
            __builtin_trap();                                                            \
        }                                                                                \
    } while (0)

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

#ifdef NDEBUG
#define DEBUG_ASSERT(...)
#define DEBUG_ASSERT_MSG(...)
#else
#define DEBUG_ASSERT(...) ASSERT(__VA_ARGS__)
#define DEBUG_ASSERT_MSG(...) ASSERT_MSG(__VA_ARGS__)
#endif

#ifdef DEBUG
#define D(x, ...) fmt::print("[DEBUG] " x "\n" __VA_OPT__(, ) __VA_ARGS__)
#else
#define D(...)
#endif

#define ARG_COUNT_(x31, x30, x29, x28, x27, x26, x25, x24, x23, x22, x21, x20, x19, x18, \
                   x17, x16, x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3,   \
                   x2, x1, x0, ...)                                                      \
    x0
#define ARG_COUNT(...)                                                                   \
    ARG_COUNT_(__VA_ARGS__, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,  \
               16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define DV_FORMAT_1(x) #x " = \x1b[1m{}\x1b[m"
#define DV_FORMAT_2(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_1(__VA_ARGS__)
#define DV_FORMAT_3(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_2(__VA_ARGS__)
#define DV_FORMAT_4(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_3(__VA_ARGS__)
#define DV_FORMAT_5(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_4(__VA_ARGS__)
#define DV_FORMAT_6(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_5(__VA_ARGS__)
#define DV_FORMAT_7(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_6(__VA_ARGS__)
#define DV_FORMAT_8(x, ...) DV_FORMAT_1(x) ", " DV_FORMAT_7(__VA_ARGS__)
#define DV_FORMAT(...) GLUE(DV_FORMAT_, ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define DV(var, ...)                                                                     \
    fmt::print(                                                                          \
        "\x1b[1;30;43m {:>4d} \x1b[m " DV_FORMAT(var __VA_OPT__(, ) __VA_ARGS__) "\n",   \
        __LINE__, var __VA_OPT__(, ) __VA_ARGS__)
