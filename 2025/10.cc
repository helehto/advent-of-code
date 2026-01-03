#include "common.h"
#include "thread_pool.h"

namespace aoc_2025_10 {

struct Machine {
    uint32_t goal;
    small_vector<uint32_t> wiring;
    small_vector<int> requirements;
};

static std::vector<Machine> parse_input(std::span<const std::string_view> lines)
{
    std::vector<Machine> machines(lines.size());
    small_vector<int> nums;

    for (size_t m = 0; std::string_view line : lines) {
        auto &[goal, wiring, requirements] = machines[m++];

        ASSERT(line.front() == '[');
        size_t i = line.find(']');
        std::string_view goal_str = line.substr(1, i - 1);
        for (size_t k = 0; k < goal_str.size(); ++k)
            goal |= (goal_str[k] == '#') << k;

        while (true) {
            size_t j = line.find('(', i);
            if (j == std::string_view::npos)
                break;
            j = line.find(')', i + 1);
            ASSERT(j != std::string_view::npos);
            find_numbers(line.substr(i + 1, j - i - 1), nums);
            for (auto &w = wiring.emplace_back(); auto n : nums)
                w |= 1zu << n;
            i = j + 1;
        }

        i = line.find('{', i);
        ASSERT(i != std::string_view::npos);
        size_t j = line.find('}', i);
        ASSERT(j == line.size() - 1);
        find_numbers(line.substr(i, j - i), nums);
        requirements = nums;

        // We use 32-bit masks in the solver below to represent subsets, make
        // sure the input fits.
        ASSERT(wiring.size() < 32);
        ASSERT(requirements.size() < 32);
    }

    return machines;
}

static int part1(std::span<const Machine> machines)
{
    int total_presses = 0;

    for (auto &[goal, wiring, _] : machines) {
        auto push_buttons = [&](size_t mask) -> uint32_t {
            uint32_t lights = 0;
            for (; mask; mask &= mask - 1)
                lights ^= wiring[std::countr_zero(mask)];
            return lights;
        };

        auto solvable_with_n_presses = [&](size_t n) -> bool {
            const size_t lo = (1zu << n) - 1;
            const size_t hi = 1zu << wiring.size();
            for (size_t mask = lo; mask < hi; mask = next_bit_permutation(mask))
                if (push_buttons(mask) == goal)
                    return true;
            return false;
        };

        auto minimum_presses = [&] -> size_t {
            for (size_t n = 1; n <= wiring.size(); ++n)
                if (solvable_with_n_presses(n))
                    return n;
            ASSERT_MSG(false, "No solution found!?");
        };

        total_presses += minimum_presses();
    }

    return total_presses;
}

/// Simplify the system of equations represented by the augmented matrix A.
static void simplify_system(MatrixView<int64_t> A)
{
    // The idea here is to perform Gaussian elimination on A to bring it into
    // reduced row echelon form.
    //
    // Using floating-point math would bring along the headache of precision
    // errors, so we do everything as integer arithmetic. In the general case,
    // the coefficients in the intermediate steps can grow exponentially, but
    // the problem input is small and the initial matrix A is a binary matrix,
    // so in practice this turns out to work fine.

    small_vector<size_t, 16> pivots;
    pivots.reserve(A.rows);

    // Forward pass. This brings the system into row echelon form, e.g.:
    //
    //     x x x x x x x x | x               x x x x x x x x | x
    //     x x x x x x x x | x               0 x x x x x x x | x
    //     x x x x x x x x | x      -->      0 0 0 x x x x x | x
    //     x x x x x x x x | x               0 0 0 0 x x x x | x
    //     x x x x x x x x | x               0 0 0 0 0 x x x | x
    //
    for (size_t i = 0; i < A.rows; ++i) {
        // Find the next pivot.
        size_t pivot = 0;
        for (; pivot < A.cols - 1; ++pivot) {
            for (size_t j = i; j < A.rows; ++j) {
                if (A(j, pivot) != 0) {
                    std::ranges::swap_ranges(A.row(i), A.row(j));
                    pivots.push_back(pivot);
                    goto found_pivot;
                }
            }
        }
        break;

    found_pivot:
        // Eliminate rows below this pivot.
        for (size_t j = i + 1; j < A.rows; ++j) {
            if (auto g = std::gcd(A(i, pivot), A(j, pivot))) {
                int64_t n = A(i, pivot) / g;
                int64_t d = A(j, pivot) / g;
                for (size_t k = 0; k < A.cols; ++k)
                    A(j, k) = d * A(i, k) - n * A(j, k);
            }
        }
    }

    // Back-substitution to clear all rows above the pivots. In addition to
    // simplifying the system by reducing the number of variables per equation,
    // for underdetermined systems it will "shift" the excess unknowns towards
    // the right side of the matrix, e.g.:
    //
    //     x x x x x x x x | x               x 0 0 0 0 0 x x | x
    //     0 x x x x x x x | x               0 x x 0 0 0 x x | x
    //     0 0 0 x x x x x | x      -->      0 0 0 x 0 0 x x | x
    //     0 0 0 0 x x x x | x               0 0 0 0 x 0 x x | x
    //     0 0 0 0 0 x x x | x               0 0 0 0 0 x x x | x
    //
    for (size_t i = 1; i < pivots.size(); ++i) {
        size_t pivot = pivots[i];
        for (size_t j = 0; j < i; ++j) {
            if (auto g = std::gcd(A(i, pivot), A(j, pivot))) {
                int64_t n = A(i, pivot) / g;
                int64_t d = A(j, pivot) / g;
                for (size_t k = 0; k < A.cols; ++k)
                    A(j, k) = n * A(j, k) - d * A(i, k);
            }
        }
    }
}

struct Solver {
    Solver(const Machine &machine);

    bool next_candidate(std::span<const int64_t> row,
                        std::span<int> sol,
                        const uint32_t cols_mask);
    void find_free_variable_assignments(std::span<int> sol, uint32_t unknown_mask);

    Matrix<int64_t> A;
    small_vector<int> bounds;
    small_vector<uint32_t> nonzero_cols_masks;
    small_vector<uint32_t> nonzero_rows_masks;
    int min_presses = INT_MAX;
};

Solver::Solver(const Machine &machine)
    : A(machine.requirements.size(), machine.wiring.size() + 1)
    , bounds(machine.wiring.size(), INT_MAX)
    , nonzero_cols_masks(A.rows)
    , nonzero_rows_masks(A.cols)
{
    const auto &[_, wiring, requirements] = machine;

    // Fill in the augmented matrix for this machine.
    for (size_t i = 0; i < wiring.size(); ++i)
        for (auto m = wiring[i]; m; m &= m - 1)
            A(std::countr_zero(m), i) = 1;
    std::ranges::copy(requirements, A.col(A.cols - 1).begin());

    // Simplify to a form which is faster to solve.
    simplify_system(A);

    // Generate upper bounds for the number of presses per button.
    for (size_t i = 0; i < wiring.size(); ++i)
        for (size_t m = wiring[i]; m; m &= m - 1)
            bounds[i] = std::min(bounds[i], requirements[std::countr_zero(m)]);

    // Pre-compute masks of non-zero columns/rows to enable quickly
    // skipping past zero entries.
    for (size_t i = 0; i < A.rows; ++i) {
        for (size_t j = 0; j < A.cols - 1; ++j) {
            if (A(i, j) != 0) {
                nonzero_cols_masks[i] |= 1zu << j;
                nonzero_rows_masks[j] |= 1zu << i;
            }
        }
    }
}

/// Generate the next candidate solution for the free variables specified by
/// `cols_mask`, updating `sol` in place. Returns false if there are no more
/// candidates.
bool Solver::next_candidate(std::span<const int64_t> row,
                            std::span<int> sol,
                            const uint32_t cols_mask)
{
    DEBUG_ASSERT(!std::has_single_bit(cols_mask));

    while (true) {
        // Generate the next combination for all but one unknown.
        {
            int carry = 0;
            for (uint32_t m = cols_mask; !std::has_single_bit(m);) {
                size_t c = 31 - std::countl_zero(m);
                sol[c]++;
                carry = (sol[c] > bounds[c]);
                if (carry == 0)
                    break;
                sol[c] = 0;
                m &= ~(1zu << c);
            }
            if (carry)
                return false;
        }

        // Compute the value for the last unknown in this row given the other
        // variables. It might not be a valid solution, so we need to check for
        // that.
        size_t c = std::countr_zero(cols_mask);
        int64_t d = row.back();
        for (size_t j = c + 1; j < row.size() - 1; ++j)
            d -= row[j] * sol[j];
        if (d % row[c] == 0) {
            auto val = d / row[c];
            if (val >= 0 && val <= bounds[c]) {
                sol[c] = val;
                return true;
            }
        }
    }
}

/// Recursively find all valid assignments for the free variables in the system
/// of equations represented by the augmented matrix A, updating `sol` in
/// place. Updates `min_presses` with the minimum number of presses found.
void Solver::find_free_variable_assignments(std::span<int> sol, uint32_t unknown_mask)
{
    if (unknown_mask == 0) {
        auto presses = *std::ranges::fold_left_first(sol, λab(a + b));
        min_presses = std::min(min_presses, presses);
        return;
    }

    // Find which variable(s) to solve for next. Pick a row with as few
    // unknowns as possible, to avoid a combinatorial explosion of candidates
    // to brute-force. (Ideally we want to find a row with only one, in that
    // case it can be solved directly.)
    size_t curr = SIZE_MAX;
    {
        size_t lowest_score = SIZE_MAX;
        for (size_t i = 0; i < A.rows; ++i) {
            if (auto m = unknown_mask & nonzero_cols_masks[i]) {
                if (size_t score = std::popcount(m); score < lowest_score) {
                    curr = std::countr_zero(m);
                    lowest_score = score;
                }
            }
        }
    }

    size_t i = std::countr_zero(nonzero_rows_masks[curr]);
    std::span<const int64_t> row = A.row(i);
    uint32_t row_unknowns_mask = unknown_mask & nonzero_cols_masks[i];

    small_vector<int> sol2(sol.begin(), sol.end());

    // If only we only have a single unknown in this row, there is no need for
    // brute force - we can compute it directly from the other ones.
    if (std::has_single_bit(row_unknowns_mask)) {
        const size_t pivot = std::countr_zero(row_unknowns_mask);
        int64_t d = row.back();

        for (auto m = nonzero_cols_masks[i] & ~(1zu << pivot); m; m &= m - 1) {
            const auto j = std::countr_zero(m);
            d -= sol[j] * row[j];
        }

        if (d % row[pivot] == 0) {
            auto val = d / row[pivot];
            if (val >= 0 && val <= bounds[curr]) {
                sol2[pivot] = val;
                find_free_variable_assignments(sol2, unknown_mask & ~row_unknowns_mask);
            }
        }
        return;
    }

    // Prepare for brute-forcing the unknowns in this row (cf next_candidate())
    // by replacing the unknown variables with 0.
    for (auto m = row_unknowns_mask; m; m &= m - 1)
        sol2[std::countr_zero(m)] = 0;

    // HACK: make sure we don't skip 0 for the last unresolved variable, that
    // is part of the minimal solution for some input rows.
    sol2[31 - std::countl_zero(row_unknowns_mask)]--;

    // Brute-force all combinations for the unknowns in this row.
    while (next_candidate(row, sol2, row_unknowns_mask))
        find_free_variable_assignments(sol2, unknown_mask & ~row_unknowns_mask);
}

static int minimize(const Machine &machine)
{
    Solver solver(machine);
    small_vector<int> sol(machine.wiring.size(), INT_MIN);
    solver.find_free_variable_assignments(sol, (1zu << machine.wiring.size()) - 1);
    ASSERT(solver.min_presses != INT_MAX);
    return solver.min_presses;
}

static int part2(std::span<const Machine> machines)
{
    std::atomic<int> result = 0;
    ThreadPool::get().for_each(machines, [&](const Machine &m) {
        result.fetch_add(minimize(m), std::memory_order_relaxed);
    });
    return result.load();
}

void run(std::string_view buf)
{
    const auto machines = parse_input(split_lines(buf));
    fmt::print("{}\n{}\n", part1(machines), part2(machines));
}

}
