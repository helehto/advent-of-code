#include "common.h"
#include "inplace_vector.h"
#include <list>

namespace aoc_2021_18 {

enum { START_PAIR = -1, END_PAIR = -2 };

static std::list<int> parse_line(std::string_view line)
{
    std::list<int> result;

    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c >= '0' && c <= '9')
            result.push_back(c - '0');
        else if (c == '[')
            result.push_back(START_PAIR);
        else if (c == ']')
            result.push_back(END_PAIR);
    }

    return result;
}

static bool explode(std::list<int> &lst)
{
    auto it = lst.begin();
    for (int depth = 0;; ++it) {
        if (it == lst.end())
            return false;
        else if (*it == START_PAIR)
            depth++;
        else if (*it == END_PAIR)
            depth--;
        else if (depth > 4)
            break;
    }

    const auto lbracket = std::next(it, -1);
    const auto a = it++;
    const auto b = it++;
    const auto rbracket = it;

    if (auto l = std::find_if(std::reverse_iterator(lbracket), lst.rend(), λx(x >= 0));
        l != lst.rend())
        *l += *a;
    if (auto r = std::find_if(rbracket, lst.end(), λx(x >= 0)); r != lst.end())
        *r += *b;

    *rbracket = 0;
    lst.erase(lbracket, rbracket);
    return true;
}

static bool split(std::list<int> &lst)
{
    auto it = std::ranges::find_if(lst, λx(x >= 10));
    if (it != lst.end()) {
        const int n = *it;
        *it++ = START_PAIR;
        it = std::next(lst.emplace(it, n / 2));
        it = std::next(lst.emplace(it, (n + 1) / 2));
        it = std::next(lst.emplace(it, END_PAIR));
        return true;
    }
    return false;
}

static std::list<int> add(std::list<int> &&a, std::list<int> &&b)
{
    std::list<int> sum;
    sum.push_back(START_PAIR);
    sum.splice(sum.end(), std::move(a));
    sum.splice(sum.end(), std::move(b));
    sum.push_back(END_PAIR);

    while (explode(sum) || split(sum))
        ;

    return sum;
}

template <typename Iterator>
static std::pair<int, Iterator> magnitude(Iterator i)
{
    if (*i < 0) {
        auto [l, j] = magnitude(std::next(i));
        auto [r, k] = magnitude(j);
        return std::pair(3 * l + 2 * r, std::next(k));
    }
    return std::pair(*i, std::next(i));
}

static int part1(std::span<const std::string_view> lines)
{
    std::list<int> sum = parse_line(lines[0]);
    for (size_t i = 1; i < lines.size(); ++i)
        sum = add(std::move(sum), parse_line(lines[i]));
    return magnitude(sum.begin()).first;
}

static int part2(std::span<const std::string_view> lines)
{
    int max_mag = INT_MIN;
    for (size_t i = 0; i < lines.size(); ++i) {
        for (size_t j = 0; j < lines.size(); ++j) {
            auto a = parse_line(lines[i]);
            auto b = parse_line(lines[j]);
            auto sum = add(std::move(a), std::move(b));
            max_mag = std::max(max_mag, magnitude(sum.begin()).first);
        }
    }

    return max_mag;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    fmt::print("{}\n", part1(lines));
    fmt::print("{}\n", part2(lines));
}

}
