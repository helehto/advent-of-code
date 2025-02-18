#include "common.h"

namespace aoc_2020_18 {

enum : uint8_t { T_END, T_LPAR, T_RPAR, T_NUM, T_ADD, T_MUL };
struct Token {
    uint8_t type;
    uint8_t value;
};

static void tokenize(std::string_view str, std::vector<Token> &tokens)
{
    tokens.clear();

    for (const char c : str) {
        if (c == '(') {
            tokens.push_back(Token{T_LPAR});
        } else if (c == ')') {
            tokens.push_back(Token{T_RPAR});
        } else if (c == '+') {
            tokens.push_back(Token{T_ADD});
        } else if (c == '*') {
            tokens.push_back(Token{T_MUL});
        } else if (c >= '0' && c <= '9') {
            tokens.push_back(Token{T_NUM, static_cast<uint8_t>(c - '0')});
        }
    }

    tokens.push_back(Token{T_END});
}

struct Evaluator {
    const Token *tokens;
    int add_precedence = 0;
    int mul_precedence = 0;
    size_t i = 0;

    int64_t unary_expr()
    {
        const Token tok = tokens[i++];
        if (tok.type == T_NUM) {
            return tok.value;
        } else if (tok.type == T_LPAR) {
            int64_t result = expr(-1);
            i++;
            return result;
        }
        __builtin_trap();
    }

    int64_t expr(int min_precedence)
    {
        int64_t result = unary_expr();

        while (true) {
            const Token tok = tokens[i];
            if (tok.type == T_ADD) {
                if (min_precedence > add_precedence)
                    return result;
                i++;
                result += expr(add_precedence + 1);
            } else if (tok.type == T_MUL) {
                if (min_precedence > mul_precedence)
                    return result;
                i++;
                result *= expr(mul_precedence + 1);
            } else {
                return result;
            }
        }
    }
};

static int64_t evaluate(std::span<const Token> tokens, int add_precedence)
{
    Evaluator e{
        .tokens = tokens.data(),
        .add_precedence = add_precedence,
    };
    return e.expr(-1);
}

void run(std::string_view buf)
{
    int64_t sum1 = 0;
    int64_t sum2 = 0;
    std::vector<Token> tokens;
    for (std::string_view line : split_lines(buf)) {
        tokenize(line, tokens);
        sum1 += evaluate(tokens, 0);
        sum2 += evaluate(tokens, 1);
    }
    fmt::print("{}\n", sum1);
    fmt::print("{}\n", sum2);
}

}
