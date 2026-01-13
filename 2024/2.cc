#include "common.h"

namespace aoc_2024_2 {

enum class Order { increasing, decreasing, invalid };

constexpr bool compatible(Order order, int a, int b)
{
    DEBUG_ASSERT(order != Order::invalid);
    int d = b - a;
    return (order == Order::increasing && d >= 1 && d <= 3) ||
           (order == Order::decreasing && d >= -3 && d <= -1);
}

constexpr Order get_order(std::span<const int> nums)
{
    const auto d = nums[1] - nums[0];

    if (d >= -3 && d <= -1) {
        for (size_t i = 1; i < nums.size(); ++i)
            if (!compatible(Order::decreasing, nums[i - 1], nums[i]))
                return Order::invalid;
        return Order::decreasing;
    }

    if (d >= 1 && d <= 3) {
        for (size_t i = 1; i < nums.size(); ++i)
            if (!compatible(Order::increasing, nums[i - 1], nums[i]))
                return Order::invalid;
        return Order::increasing;
    }

    return Order::invalid;
}

constexpr Order get_concatenated_order(std::span<const int> a, std::span<const int> b)
{
    const Order order_a = get_order(a);
    if (order_a == Order::invalid)
        return Order::invalid;

    const Order order_b = get_order(b);
    if (order_b != order_a)
        return Order::invalid; // also covers the case where b is invalid

    return compatible(order_a, a.back(), b.front()) ? order_a : Order::invalid;
}

constexpr bool is_safe2(std::span<const int> nums)
{
    DEBUG_ASSERT(nums.size() >= 5);

    if (get_order(nums.subspan(1)) != Order::invalid)
        return true;
    if (get_order(nums.subspan(0, nums.size() - 1)) != Order::invalid)
        return true;

    {
        const auto order = get_order(nums.subspan(2));
        if (order != Order::invalid && compatible(order, nums[0], nums[1]))
            return true;
    }
    {
        const auto order = get_order(nums.subspan(0, nums.size() - 2));
        if (order != Order::invalid &&
            compatible(order, nums[nums.size() - 2], nums[nums.size() - 1]))
            return true;
    }

    for (size_t i = 2; i + 2 < nums.size(); ++i) {
        const auto head = std::span(nums).subspan(0, i);
        const auto tail = std::span(nums).subspan(i + 1);
        const auto order = get_concatenated_order(head, tail);
        if (order != Order::invalid)
            return true;
    }

    return false;
}

void run(std::string_view buf)
{
    int s1 = 0;
    int s2 = 0;
    small_vector<int> nums;
    for (std::string_view line : split_lines(buf)) {
        find_numbers(line, nums);
        s1 += get_order(nums) != Order::invalid;
        s2 += is_safe2(nums);
    }

    fmt::print("{}\n", s1);
    fmt::print("{}\n", s2);
}

}
