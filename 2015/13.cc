#include "common.h"
#include "dense_map.h"
#include "thread_pool.h"

namespace aoc_2015_13 {

static Matrix<int> get_happiness_matrix(std::string_view buf)
{
    auto lines = split_lines(buf);
    int n = 0;

    dense_map<std::string_view, int> name_map;
    std::vector<std::string_view> words;
    for (auto &line : lines) {
        split(line, words, ' ');
        words[10].remove_suffix(1); // remove full stop
        if (auto [it, inserted] = name_map.emplace(words[0], n); inserted)
            n++;
        if (auto [it, inserted] = name_map.emplace(words[10], n); inserted)
            n++;
    }

    Matrix<int> matrix(n, n);
    for (auto &line : lines) {
        split(line, words, ' ');
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

static int total_happiness(MatrixView<const int> matrix, std::span<int> perm)
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

constexpr int64_t factorials[] = {
    UINT64_C(1),
    UINT64_C(1),
    UINT64_C(2),
    UINT64_C(6),
    UINT64_C(24),
    UINT64_C(120),
    UINT64_C(720),
    UINT64_C(5040),
    UINT64_C(40320),
    UINT64_C(362880),
    UINT64_C(3628800),
    UINT64_C(39916800),
    UINT64_C(479001600),
    UINT64_C(6227020800),
    UINT64_C(87178291200),
    UINT64_C(1307674368000),
    UINT64_C(20922789888000),
    UINT64_C(355687428096000),
    UINT64_C(6402373705728000),
    UINT64_C(121645100408832000),
    UINT64_C(2432902008176640000),
};

/// Returns the nth permutation of [0, 1, 2, ..., n-1].
static std::vector<int> nth_permutation(size_t n, size_t index)
{
    // cf. <https://en.wikipedia.org/wiki/Lehmer_code#Encoding_and_decoding>

    std::vector<int> indices(n);
    for (size_t i = indices.size(); i--;) {
        indices[indices.size() - 1 - i] = index / factorials[i];
        index = index % factorials[i];
    }

    std::vector<int> perm(n);
    for (size_t i = 0; i < perm.size(); i++)
        perm[i] = i;

    std::vector<int> result;
    for (size_t i = 0; i < indices.size(); i++) {
        result.push_back(perm[indices[i]]);
        perm.erase(perm.begin() + indices[i]);
    }

    return result;
}

static int max_happiness(MatrixView<const int> matrix)
{
    std::atomic<int> result = 0;

    auto max_happiness_between = [&](size_t start, size_t end) {
        auto perm = nth_permutation(matrix.rows, start);
        int happiness = 0;
        for (size_t i = start; i < end; i++) {
            int total = total_happiness(matrix, perm);
            happiness = std::max(happiness, total);
            std::ranges::next_permutation(perm);
        }
        return happiness;
    };

    const auto n = factorials[matrix.rows];
    ThreadPool::get().for_each_index(0, n, [&](size_t start, size_t end) {
        atomic_store_max(result, max_happiness_between(start, end));
    });

    return result.load();
}

void run(std::string_view buf)
{
    auto matrix = get_happiness_matrix(buf);
    ASSERT(matrix.rows + 1 < std::size(factorials));

    fmt::print("{}\n", max_happiness(matrix));

    Matrix<int> augmented(matrix.rows + 1, matrix.cols + 1);
    for (size_t i = 0; i < matrix.rows; i++) {
        for (size_t j = 0; j < matrix.cols; j++) {
            augmented(i, j) = matrix(i, j);
        }
    }
    fmt::print("{}\n", max_happiness(augmented));
}

}
