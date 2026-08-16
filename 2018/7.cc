#include "common.h"
#include <algorithm>

namespace aoc_2018_7 {

static void
part1(std::array<small_vector<int>, 26> dependencies, int max_task, aoc::Answer &answer)
{
    std::string result;
    int tasks[26];
    int n = 0;

    for (int i = 0; i <= max_task; i++)
        if (dependencies[i].empty())
            tasks[n++] = i;

    while (n) {
        int *p = std::min_element(tasks, tasks + n);
        int u = std::exchange(*p, tasks[--n]);

        result += static_cast<char>(u + 'A');

        for (int i = 0; i <= max_task; i++) {
            if (!dependencies[i].empty()) {
                erase(dependencies[i], u);
                if (dependencies[i].empty())
                    tasks[n++] = i;
            }
        }
    }
    answer.add(result);
}

static void
part2(std::array<small_vector<int>, 26> dependencies, int max_task, aoc::Answer &answer)
{
    struct Task {
        int id;
        int complete_at;
    };

    constexpr int n_workers = 5;

    inplace_vector<int, 26> pending_tasks;
    inplace_vector<Task, n_workers> active_tasks;

    for (int i = 0; i <= max_task; i++)
        if (dependencies[i].empty())
            pending_tasks.push_back(i);

    int t = 0;
    while (!pending_tasks.empty() || !active_tasks.empty()) {
        // Assign new tasks, if possible:
        while (!pending_tasks.empty() && active_tasks.size() < n_workers) {
            auto it = std::ranges::min_element(pending_tasks);
            active_tasks.push_back(Task{*it, t + *it + 61});
            std::swap(*it, pending_tasks.back());
            pending_tasks.pop_back();
        }

        // Figure out when the earliest current task will complete:
        t = std::ranges::min(active_tasks, {}, &Task::complete_at).complete_at;

        // Handle all work which has been done at time `t`:
        auto completed = std::ranges::partition(active_tasks, λx(x.complete_at != t));
        for (auto &task : completed) {
            for (int j = 0; j <= max_task; j++) {
                if (!dependencies[j].empty()) {
                    erase(dependencies[j], task.id);
                    if (dependencies[j].empty())
                        pending_tasks.push_back(j);
                }
            }
        }
        active_tasks.erase(completed.begin(), completed.end());
    }

    answer.add(t);
}

void run(std::string_view buf, aoc::Answer &answer)
{
    std::vector<std::string_view> cs;
    std::array<small_vector<int>, 26> dependencies;
    int max_task = 0;
    for (auto line : split_lines(buf)) {
        split(line, cs, λx((uint8_t)(x - 'A') >= 26));
        int c1 = cs[1][0] - 'A';
        int c2 = cs[2][0] - 'A';
        dependencies[c2].push_back(c1);
        max_task = std::max({max_task, c1, c2});
    }

    part1(dependencies, max_task, answer);
    part2(std::move(dependencies), max_task, answer);
}
AOC_REGISTER_SOLVER(2018, 7, run);

}
