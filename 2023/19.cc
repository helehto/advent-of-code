#include "common.h"
#include "dense_map.h"
#include <optional>
#include <variant>

namespace aoc_2023_19 {

constexpr int ACCEPTED = -1;
constexpr int REJECTED = -2;

struct Comparison {
    int16_t index;
    bool less_than;
    int16_t constant;
    int16_t target;
};

using Workflow = boost::container::static_vector<std::variant<Comparison, int>, 8>;
using Hypercube = std::array<std::pair<int64_t, int64_t>, 4>;

static Workflow parse_workflow(const dense_map<std::string_view, int> &workflow_index,
                               std::string_view s,
                               std::vector<std::string_view> &fields)
{
    Workflow workflow;
    split(s, fields, ',');

    auto name_to_index = [&](std::string_view name) {
        if (name == "A")
            return ACCEPTED;
        if (name == "R")
            return REJECTED;
        return workflow_index.at(name);
    };

    for (auto &step : fields) {
        auto colon = step.find(':');
        if (colon == std::string_view::npos) {
            workflow.emplace_back(name_to_index(step));
            continue;
        }

        Comparison cmp;

        auto expression = step.substr(0, colon);
        char c = expression[0];
        cmp.index = c == 'x' ? 0 : c == 'm' ? 1 : c == 'a' ? 2 : 3;
        cmp.less_than = expression[1] == '<';
        std::from_chars(expression.begin() + 2, expression.end(), cmp.constant);
        cmp.target = name_to_index(step.substr(colon + 1));
        workflow.push_back(cmp);
    }

    return workflow;
}

static int apply_workflow(const Workflow &workflow, std::span<const int> nums)
{
    for (auto &step : workflow) {
        if (auto *target = std::get_if<int>(&step))
            return *target;

        auto &cmp = std::get<Comparison>(step);
        if (cmp.less_than ? nums[cmp.index] < cmp.constant
                          : nums[cmp.index] > cmp.constant)
            return cmp.target;
    }

    ASSERT_MSG(false, "Fell off the end of a workflow!");
}

static int part1(std::span<const Workflow> workflows,
                 std::span<const std::string_view> parts,
                 int start_workflow)
{
    int sum = 0;
    std::vector<int> nums;
    for (auto part : parts) {
        find_numbers(part, nums);
        for (int index = start_workflow; index >= 0;) {
            index = apply_workflow(workflows[index], nums);
            if (index == ACCEPTED)
                sum += nums[0] + nums[1] + nums[2] + nums[3];
        }
    }
    return sum;
}

static int64_t hypercube_volume(const Hypercube &cube)
{
    int64_t result = 1;
    for (auto [a, b] : cube)
        result *= (b - a + 1);
    return result;
}

static int64_t part2(std::span<const Workflow> workflows, int start_workflow)
{
    std::vector<std::pair<int, Hypercube>> queue;
    queue.emplace_back(start_workflow,
                       Hypercube{{{1, 4000}, {1, 4000}, {1, 4000}, {1, 4000}}});

    int64_t volume = 0;
    while (!queue.empty()) {
        auto [name, cube] = queue.back();
        queue.pop_back();

        auto handle_step = [&](int index, Hypercube cube) {
            if (index == ACCEPTED)
                volume += hypercube_volume(cube);
            else if (index != REJECTED)
                queue.emplace_back(index, cube);
        };

        for (auto &step : workflows[name]) {
            if (auto *target = std::get_if<int>(&step)) {
                handle_step(*target, cube);
                break;
            }

            auto &cmp = std::get<Comparison>(step);
            auto other_cube = cube;
            if (cmp.less_than) {
                cube[cmp.index].first = cmp.constant;
                other_cube[cmp.index].second = cmp.constant - 1;
            } else {
                cube[cmp.index].second = cmp.constant;
                other_cube[cmp.index].first = cmp.constant + 1;
            }

            handle_step(cmp.target, other_cube);
        }
    }

    return volume;
}

void run(std::string_view buf)
{
    auto lines = split_lines(buf);
    dense_map<std::string_view, int> workflow_index;

    size_t i = 0;
    while (i < lines.size() && !lines[i].empty()) {
        ASSERT(workflow_index.emplace(lines[i].substr(0, lines[i].find('{')), i).second);
        i++;
    }
    const size_t num_workflows = i;

    std::vector<Workflow> workflows(num_workflows);
    std::vector<std::string_view> fields;
    for (i = 0; i < lines.size() && !lines[i].empty(); i++) {
        auto &s = lines[i];
        size_t i = s.find('{');
        ASSERT(i != std::string_view::npos);
        workflows[workflow_index.at(s.substr(0, i))] =
            parse_workflow(workflow_index, {s.begin() + i + 1, s.end() - 1}, fields);
    }
    i++;

    const auto in = workflow_index.at("in");
    fmt::print("{}\n", part1(workflows, std::span(lines.begin() + i, lines.end()), in));
    fmt::print("{}\n", part2(workflows, in));
}

}
