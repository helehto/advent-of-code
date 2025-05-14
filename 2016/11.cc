#include "common.h"
#include "dense_set.h"

namespace aoc_2016_11 {

struct FloorItems {
    union {
        struct {
            uint8_t microchip_mask;
            uint8_t generator_mask;
        };
        uint16_t combined;
    };

    /// Check whether this combination of items is safe to leave on a floor.
    bool safe() const
    {
        // Check all potential items at once using a bit of SWAR:
        constexpr uint64_t ones = 0x0101010101010101;
        const auto danger = _pdep_u64(microchip_mask & ~generator_mask, ones) * 0xff;
        return (danger & (generator_mask * ones) & ~0x80402010'08040201) == 0;
    }
};

struct CanonicalState;

struct State {
    union {
        std::array<FloorItems, 4> items;
        uint64_t items_u64;
    };
    int32_t floor;

    constexpr CanonicalState canonicalize() const;
};

struct CanonicalState {
    // Each uint8_t element in `pairs` is split into two 4-bit quantities
    // signifying on which two floors that a matching generator/microchip pair
    // is located. Since the pairs can be permuted freely without affecting the
    // final result, the array is sorted in to collapse all of the different
    // permutations into a single canonical representation.
    std::array<uint8_t, 8> pairs;
    int32_t floor;

    constexpr bool operator==(const CanonicalState &other) const = default;
};

constexpr CanonicalState State::canonicalize() const
{
    CanonicalState result = {.floor = floor};
    result.pairs.fill(0xff);

    for (int i = 0, k = 0; i < 8; ++i) {
        constexpr uint64_t mask = 0x0001'0001'0001'0001;
        const auto g = std::countr_zero(items_u64 & (mask << i)) / 16;
        const auto m = std::countr_zero(items_u64 & (mask << (i + 8))) / 16;
        result.pairs[k++] = m << 4 | g;
    }

    // Sort the pairs branchlessly using a size-optimal sorting network for 8
    // elements, from
    // <https://bertdobbelaere.github.io/sorting_networks.html#N8L19D6>.
    auto f = [&](size_t i, size_t j) {
        int min = std::min(result.pairs[i], result.pairs[j]);
        int max = std::max(result.pairs[i], result.pairs[j]);
        result.pairs[i] = min;
        result.pairs[j] = max;
    };
    f(0, 2), f(1, 3), f(4, 6), f(5, 7);
    f(0, 4), f(1, 5), f(2, 6), f(3, 7);
    f(0, 1), f(2, 3), f(4, 5), f(6, 7);
    f(2, 4), f(3, 5);
    f(1, 4), f(3, 6);
    f(1, 2), f(3, 4), f(5, 6);

    return result;
}

}

template <>
struct std::hash<aoc_2016_11::CanonicalState> {
    size_t operator()(const aoc_2016_11::CanonicalState &state) const noexcept
    {
        return _mm_crc32_u64(state.floor, std::bit_cast<uint64_t>(state.pairs));
    }
};

namespace aoc_2016_11 {

static size_t element_index(std::string_view e)
{
    if (e == "strontium" || e == "hydrogen")
        return 0;
    else if (e == "plutonium" || e == "lithium")
        return 1;
    else if (e == "thulium")
        return 2;
    else if (e == "ruthenium")
        return 3;
    else if (e == "curium")
        return 4;
    else
        ASSERT_MSG(false, "Unknown element {}", e);
}

static State parse_input(std::string_view buf)
{
    State state{};
    auto lines = split_lines(buf);
    ASSERT(lines.size() == 4);

    std::vector<std::string_view> tmp, tmp2;
    for (size_t i = 0; std::string_view line : lines) {
        line = line.substr(line.find("contains") + 9);
        split(line, tmp, ',');

        for (std::string_view s : tmp) {
            if (s.ends_with('.'))
                s.remove_suffix(1);

            split(strip(s), tmp2, ' ');
            std::string_view e = tmp2[tmp2.size() - 2];

            if (tmp2.back() == "generator") {
                state.items[i].generator_mask |= 1 << element_index(e);
            } else if (tmp2.back() == "microchip") {
                e = e.substr(0, e.find('-'));
                state.items[i].microchip_mask |= 1 << element_index(e);
            }
        }

        ++i;
    }

    return state;
}

/// Compute the next lexicographic permutation of the bits in `v`.
///
/// Taken from <https://graphics.stanford.edu/~seander/bithacks.html>.
constexpr size_t next_bit_permutation(size_t v)
{
    const auto t = v | (v - 1);
    return (t + 1) | (((~t & -~t) - 1) >> (std::countr_zero(v) + 1));
}

void run(std::string_view buf)
{
    std::vector<std::tuple<State, int>> queue;
    dense_set<CanonicalState> seen;

    // TODO: Use A* instead of BFS?
    auto search = [&](const State initial_state) {
        seen.clear();
        seen.reserve(20'000);
        seen.insert(initial_state.canonicalize());

        queue.reserve(20'000);
        queue = {{initial_state, 0}};

        for (size_t i = 0; i < queue.size(); ++i) {
            auto [state, steps] = queue[i];

            int min_floor = 0;
            while (state.items[min_floor].combined == 0)
                ++min_floor;
            if (min_floor == 3)
                return steps;

            auto queue_move = [&](const int dir, const uint16_t items_mask) {
                const int old_floor = state.floor;
                const int new_floor = state.floor + dir;

                auto new_items = state.items;
                new_items[old_floor].combined &= ~items_mask;
                new_items[new_floor].combined |= items_mask;

                if (new_items[old_floor].safe() && new_items[new_floor].safe()) {
                    const State new_state{new_items, new_floor};
                    if (seen.insert(new_state.canonicalize()).second)
                        queue.emplace_back(new_state, steps + 1);
                }
            };

            auto move_one_item = [&](int dir) {
                for (size_t m = state.items[state.floor].combined; m; m &= m - 1)
                    queue_move(dir, m & -m);
            };

            auto move_two_items = [&](int dir) {
                const uint32_t items = state.items[state.floor].combined;
                const int item_count = std::popcount(items);
                for (int m = 0b11; m < (1 << item_count); m = next_bit_permutation(m))
                    queue_move(dir, _pdep_u32(m, items));
            };

            if (state.floor > min_floor) {
                move_one_item(-1);
                move_two_items(-1);
            }
            if (state.floor < 3) {
                move_one_item(+1);
                move_two_items(+1);
            }
        }

        ASSERT_MSG(false, "No path found!?");
    };

    auto initial_state = parse_input(buf);

    // Part 1:
    fmt::print("{}\n", search(initial_state));

    // Part 2:
    initial_state.items[0].generator_mask |= 1 << 6 | 1 << 7;
    initial_state.items[0].microchip_mask |= 1 << 6 | 1 << 7;
    fmt::print("{}\n", search(initial_state));
}

}
