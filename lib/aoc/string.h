#pragma once

#include <aoc/macros.h>
#include <aoc/small_vector.h>
#include <array>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

// Moved to a separate TU due to the heavy <hwy/highway.h> dependency.
std::vector<std::string_view> split_lines(std::string_view s);
std::vector<std::string_view> &
split(std::string_view s, std::vector<std::string_view> &out, char c);

template <typename Predicate>
constexpr std::vector<std::string_view> &
split(std::string_view s, std::vector<std::string_view> &out, Predicate &&predicate)
{
    out.clear();

    while (true) {
        while (!s.empty() && predicate(s.front()))
            s.remove_prefix(1);
        if (s.empty())
            break;

        size_t i = 0;
        while (i < s.size() && !predicate(s[i]))
            i++;
        out.emplace_back(s.data(), i);
        s.remove_prefix(i);
    }

    return out;
}

constexpr std::string_view strip(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\n'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\n'))
        s.remove_suffix(1);
    return s;
}

template <typename T>
constexpr void find_numbers_impl(std::string_view s, auto &&sink)
{
    const char *p = s.data();
    const char *end = p + s.size();

    while (true) {
        [[maybe_unused]] int mul_overflow, add_overflow;

        while (true) {
            if (p == end) [[unlikely]]
                return;
            if (*p >= '0' && *p <= '9')
                break;
            p++;
        }

        const char *first = p;

        T value{};
        do {
            mul_overflow = __builtin_mul_overflow(value, 10, &value);
            DEBUG_ASSERT_MSG(!mul_overflow, "Overflow: {} * {}", value, 10);
            add_overflow = __builtin_add_overflow(value, *p - '0', &value);
            DEBUG_ASSERT_MSG(!add_overflow, "Overflow: {} + {}", value, *p - '0');
            p++;
        } while (p != end && *p >= '0' && *p <= '9');

        if constexpr (std::is_signed_v<T>) {
            if (first != s.data() && first[-1] == '-') {
                [[maybe_unused]] int neg_overflow;
                neg_overflow = __builtin_sub_overflow(0, value, &value);
                DEBUG_ASSERT(!neg_overflow);
            }
        }

        sink(value);
    }
}

template <typename T>
constexpr void find_numbers(std::string_view s, small_vector_base<T> &result)
{
    result.clear();
    find_numbers_impl<T>(s, [&](auto &&v) { result.push_back(static_cast<T &&>(v)); });
}

template <typename T>
constexpr void find_numbers(std::string_view s, std::vector<T> &result)
{
    result.clear();
    find_numbers_impl<T>(s, [&](auto &&v) { result.push_back(static_cast<T &&>(v)); });
}

template <typename T, size_t N>
constexpr std::array<T, N> find_numbers_n(std::string_view s)
{
    std::array<T, N> result{};
    size_t i = 0;

    find_numbers_impl<T>(s, [&](auto &&v) {
        ASSERT(i < result.size());
        result[i++] = static_cast<T &&>(v);
    });

    ASSERT_MSG(i == N, "Expected {} numbers, but got only {}!", N, i);
    return result;
}

template <typename T>
constexpr std::vector<T> find_numbers(std::string_view s)
{
    std::vector<T> result;
    find_numbers(s, result);
    return result;
}

template <typename T>
constexpr small_vector<T> find_numbers_small(std::string_view s)
{
    small_vector<T> result;
    find_numbers(s, result);
    return result;
}
