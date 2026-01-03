#include "common.h"
#include "thread_pool.h"

namespace aoc_2021_18 {

enum { START_PAIR = -1, END_PAIR = -2 };

constexpr small_vector<int8_t> parse_line(std::string_view line)
{
    small_vector<int8_t> result;

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

[[gnu::noinline]] constexpr bool explode(small_vector_base<int8_t> &lst)
{
    auto it = lst.begin();
    for (int depth = 0;; ++it) {
        if (it == lst.end())
            return false;
        if (*it < 0) {
            if (*it == START_PAIR)
                depth++;
            else
                depth--;
        } else if (depth > 4)
            break;
    }

    const auto lbracket = it - 1;
    const auto a = it++;
    const auto b = it++;
    const auto rbracket = it;

    if (auto l = std::find_if(std::reverse_iterator(lbracket), lst.rbegin(), λx(x >= 0));
        l != lst.rbegin())
        *l += *a;
    if (auto r = std::find_if(rbracket, lst.end(), λx(x >= 0)); r != lst.end())
        *r += *b;

    *rbracket = 0;
    lst.erase(lbracket, rbracket);
    return true;
}

[[gnu::noinline]] constexpr bool split(small_vector_base<int8_t> &lst)
{
    auto it = std::ranges::find_if(lst, λx(x >= 10));
    if (it != lst.end()) {
        const int n = *it;
        *it++ = START_PAIR;
        lst.insert(it, {
                           static_cast<int8_t>(n / 2),
                           static_cast<int8_t>((n + 1) / 2),
                           END_PAIR,
                       });
        return true;
    }
    return false;
}

constexpr void add(small_vector_base<int8_t> &a, std::span<const int8_t> b)
{
    a.reserve(a.size() + b.size() + 2);
    a.insert(a.begin(), START_PAIR);
    a.append_range(b);
    a.push_back(END_PAIR);

    while (explode(a) || split(a))
        ;
}

constexpr std::pair<int, const int8_t *> magnitude(const int8_t *p)
{
    if (*p < 0) {
        auto [l, p2] = magnitude(p + 1);
        auto [r, p3] = magnitude(p2);
        return std::pair(3 * l + 2 * r, p3 + 1);
    }
    return std::pair(*p, p + 1);
}

constexpr int part1(std::span<const small_vector<int8_t>> lines)
{
    small_vector<int8_t> sum = lines[0];
    for (size_t i = 1; i < lines.size(); ++i)
        add(sum, lines[i]);
    return magnitude(sum.begin()).first;
}

static int part2(std::span<const small_vector<int8_t>> lines)
{
    std::atomic_int max_mag = INT_MIN;

    ThreadPool::get().for_each(lines, [&](const auto &line) {
        for (size_t j = 0; j < lines.size(); ++j) {
            auto sum = line;
            add(sum, lines[j]);
            atomic_store_max(max_mag, magnitude(sum.begin()).first);
        }
    });

    std::atomic_thread_fence(std::memory_order_seq_cst);
    return max_mag.load(std::memory_order_relaxed);
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<small_vector<int8_t>> input;
    input.reserve(lines.size());
    for (std::string_view line : lines)
        input.push_back(parse_line(line));

    fmt::print("{}\n", part1(input));
    fmt::print("{}\n", part2(input));
}

}
