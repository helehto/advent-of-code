#include "common.h"
#include <variant>

namespace aoc_2015_12 {

using Entity = std::variant<int64_t, std::string_view>;

struct Parser {
    std::string_view document;

    char next()
    {
        document.remove_prefix(1);
        return peek();
    }

    char peek() const { return !document.empty() ? document.front() : '\0'; }

    std::string_view parse_string();
    int64_t parse_object();
    int64_t parse_array();
    Entity parse_entity();
};

std::string_view Parser::parse_string()
{
    const char *start = document.data() + 1;

    while (next() != '"')
        ;
    next();

    return std::string_view(start, document.data() - start - 1);
}

int64_t Parser::parse_object()
{
    int sum = 0;
    bool has_red_value = false;

    if (next() == '}')
        return 0;

    while (next() != '}') {
        parse_string();
        next();

        auto value = parse_entity();
        if (auto *p = std::get_if<std::string_view>(&value))
            has_red_value |= (*p == "red");
        else if (auto *i = std::get_if<int64_t>(&value))
            sum += *i;

        if (peek() == '}')
            break;
        next();
    }

    next();
    return has_red_value ? 0 : sum;
}

int64_t Parser::parse_array()
{
    int sum = 0;

    while (next() != ']') {
        auto element = parse_entity();
        if (auto *i = std::get_if<int64_t>(&element))
            sum += *i;
        if (peek() == ']')
            break;
    }

    next();
    return sum;
}

Entity Parser::parse_entity()
{
    int64_t value;

    switch (peek()) {
    case '{':
        return parse_object();
    case '[':
        return parse_array();
    case '"':
        return parse_string();
    default:
        auto r = std::from_chars(begin(document), end(document), value);
        ASSERT(r.ec == std::errc() || !"bad input");
        document.remove_prefix(r.ptr - document.data());
        return value;
    }
}

void run(std::string_view buf)
{
    int sum = 0;
    for (int x : find_numbers<int>(buf))
        sum += x;

    fmt::print("{}\n", sum);
    fmt::print("{}\n", std::get<int64_t>(Parser{buf}.parse_entity()));
}

}
