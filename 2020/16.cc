#include "common.h"

namespace aoc_2020_16 {

static bool in_interval(int n, const std::pair<int16_t, int16_t> &interval)
{
    const auto [start, end] = interval;
    return n >= start && n <= end;
}

static bool in_any_interval(int n, std::span<const std::pair<int16_t, int16_t>> intervals)
{
    for (auto interval : intervals) {
        if (in_interval(n, interval))
            return true;
    }
    return false;
}

/// Solve part 1, removing all invalid tickets from `tickets` as a side-effect
/// in preparation for part 2.
static int part1(std::vector<int16_t> &nums,
                 std::vector<std::string_view> &tickets,
                 const std::vector<std::pair<int16_t, int16_t>> &intervals)
{
    int error_rate = 0;
    for (size_t i = 0; i < tickets.size();) {
        std::string_view ticket = tickets[i];
        find_numbers(ticket, nums);

        bool ticket_ok = true;
        for (const auto n : nums) {
            if (!in_any_interval(n, intervals)) {
                ticket_ok = false;
                error_rate += n;
            }
        }

        if (ticket_ok) {
            ++i;
        } else {
            tickets[i] = tickets.back();
            tickets.pop_back();
        }
    }
    return error_rate;
}

static int64_t part2(std::vector<int16_t> &nums,
                     std::span<const std::string_view> field_names,
                     std::string_view your_ticket_str,
                     std::vector<std::string_view> &tickets,
                     const std::vector<std::pair<int16_t, int16_t>> &intervals)
{
    std::vector<int16_t> your_ticket;
    find_numbers(your_ticket_str, your_ticket);

    std::vector<uint32_t> field_mask(your_ticket.size(),
                                     (uint32_t(1) << your_ticket.size()) - 1);

    // Remove bits in the mask for each field that correspond to values which
    // are not compatible with that field (i.e. the given value does not lie
    // within either of the given interval for that field).
    for (std::string_view ticket : tickets) {
        find_numbers(ticket, nums);
        for (size_t i = 0; i < nums.size(); i++) {
            for (size_t j = 0; j < intervals.size(); j += 2) {
                if (!in_any_interval(nums[i], std::span(intervals.data() + j, 2)))
                    field_mask[i] &= ~(uint32_t(1) << (j / 2));
            }
        }
    }

    // Iteratively eliminate bits from fields that only have a single possible
    // choice remaining from all other masks. After this, all ticket columns
    // uniquely correspond to a single field.
    for (size_t _ = 0; _ < field_mask.size(); ++_) {
        for (size_t i = 0; i < field_mask.size(); i++) {
            for (size_t j = 0; j < field_mask.size(); j++) {
                if (i != j && std::has_single_bit(field_mask[i]))
                    field_mask[j] &= ~field_mask[i];
            }
        }
    }

    // Compute the product of all departure fields.
    int64_t result = 1;
    for (size_t i = 0; i < field_mask.size(); ++i) {
        size_t index = std::countr_zero(field_mask[i]);
        if (field_names[index].starts_with("departure"))
            result *= your_ticket[i];
    }

    return result;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);

    std::vector<std::string_view> field_names;
    std::vector<std::pair<int16_t, int16_t>> intervals;
    std::vector<int16_t> nums;

    size_t i = 0;
    for (; !lines[i].empty(); ++i) {
        auto [a0, a1, b0, b1] = find_numbers_n<int, 4>(lines[i]);
        field_names.push_back(lines[i].substr(0, lines[i].find(':')));
        intervals.emplace_back(a0, -a1);
        intervals.emplace_back(b0, -b1);
    }
    i += 2; // skip blank line + 'your ticket:'

    std::string_view your_ticket = lines[i];
    i += 3; // skip our ticket + blank line + 'nearby tickets:'

    std::vector<std::string_view> tickets(lines.begin() + i, lines.end());

    fmt::print("{}\n", part1(nums, tickets, intervals));
    fmt::print("{}\n", part2(nums, field_names, your_ticket, tickets, intervals));
}
}
