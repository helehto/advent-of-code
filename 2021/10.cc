#include "common.h"
#include <fmt/core.h>
#include <vector>

int run_2021_10(FILE *f)
{
    std::string s;
    int score1 = 0;
    std::vector<char> stack;
    std::vector<uint64_t> scores2;

    while (getline(f, s)) {
        stack.clear();
        bool incorrect = false;

        for (char c : s) {
            if (c == '(') {
                stack.push_back(')');
            } else if (c == '[') {
                stack.push_back(']');
            } else if (c == '{') {
                stack.push_back('}');
            } else if (c == '<') {
                stack.push_back('>');
            } else {
                if (c != stack.back()) {
                    if (c == ')')
                        score1 += 3;
                    else if (c == ']')
                        score1 += 57;
                    else if (c == '}')
                        score1 += 1197;
                    else if (c == '>')
                        score1 += 25137;
                    incorrect = true;
                }
                stack.pop_back();
            }
        }

        if (!stack.empty() && !incorrect) {
            uint64_t score2 = 0;
            for (size_t i = stack.size(); i--;) {
                score2 = 5 * score2;
                if (stack[i] == ')')
                    score2 += 1;
                else if (stack[i] == ']')
                    score2 += 2;
                else if (stack[i] == '}')
                    score2 += 3;
                else if (stack[i] == '>')
                    score2 += 4;
            }
            scores2.push_back(score2);
        }
    }

    fmt::print("{}\n", score1);
    std::sort(begin(scores2), end(scores2));
    fmt::print("{}\n", scores2[scores2.size() / 2]);
    return 0;
}
