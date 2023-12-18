#include "common.h"
#include "dense_map.h"

static Matrix<int> get_happiness_matrix(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    int n = 0;

    dense_map<std::string_view, int> name_map;
    std::vector<std::string_view> words;
    for (auto &line : lines) {
        split(line, words);
        words[10].remove_suffix(1); // remove full stop
        if (auto [it, inserted] = name_map.emplace(words[0], n); inserted)
            n++;
        if (auto [it, inserted] = name_map.emplace(words[10], n); inserted)
            n++;
    }

    Matrix<int> matrix(n, n);
    for (auto &line : lines) {
        split(line, words);
        words[10].remove_suffix(1); // remove full stop
        auto i = name_map.at(words[0]);
        auto j = name_map.at(words[10]);
        int d = -1;
        auto r = std::from_chars(begin(words[3]), end(words[3]), d);
        ASSERT(r.ec == std::errc());
        if (words[2] == "lose")
            d = -d;
        matrix(i, j) = d;
    }

    return matrix;
}

static int total_happiness(const Matrix<int> &matrix, std::span<int> perm)
{
    int result = 0;

    for (size_t i = 1; i < perm.size(); i++) {
        result += matrix(perm[i - 1], perm[i]);
        result += matrix(perm[i], perm[i - 1]);
    }

    result += matrix(perm.front(), perm.back());
    result += matrix(perm.back(), perm.front());

    return result;
}

static int max_happiness(const Matrix<int> &matrix)
{
    std::vector<int> perm(matrix.rows);
    for (size_t i = 0; i < perm.size(); i++)
        perm[i] = i;

    int max_happiness = 0;
    do {
        int total = total_happiness(matrix, perm);
        max_happiness = std::max(max_happiness, total);
    } while (std::next_permutation(begin(perm), end(perm)));

    return max_happiness;
}

void run_2015_13(FILE *f)
{
    auto matrix = get_happiness_matrix(f);
    fmt::print("{}\n", max_happiness(matrix));

    Matrix<int> augmented(matrix.rows + 1, matrix.cols + 1);
    for (size_t i = 0; i < matrix.rows; i++) {
        for (size_t j = 0; j < matrix.cols; j++) {
            augmented(i, j) = matrix(i, j);
        }
    }
    fmt::print("{}\n", max_happiness(augmented));
}
