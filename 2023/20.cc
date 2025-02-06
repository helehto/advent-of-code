#include "common.h"
#include "dense_map.h"
#include <algorithm>
#include <cstdio>
#include <functional>
#include <numeric>
#include <queue>
#include <ranges>

namespace aoc_2023_20 {

enum {
    BROADCASTER,
    FLIPFLOP,
    CONJUNCTION,
};

struct Component {
    int type;
    bool on = false;                          // flipflop
    dense_map<std::string_view, bool> inputs; // conjunction
    std::vector<std::string_view> outputs;
};

struct Circuit {
    dense_map<std::string_view, Component> components_by_name;
    std::vector<std::string_view> rx_comp_input_names;
    std::string_view rx_input_name;
};

static Circuit parse_input(std::span<const std::string_view> lines)
{
    dense_map<std::string_view, Component> components_by_name;
    std::string_view rx_input_name;

    for (std::string_view line : lines) {
        auto arrow = line.find("->");
        ASSERT(arrow != std::string_view::npos);

        int type = -1;

        size_t i = 0;
        if (line.front() == '%') {
            type = FLIPFLOP;
            i++;
        } else if (line.front() == '&') {
            type = CONJUNCTION;
            i++;
        }

        std::string_view name = strip(line.substr(i, arrow - i));

        if (name == "broadcaster") {
            ASSERT(type == -1);
            type = BROADCASTER;
        }

        std::vector<std::string_view> outputs;
        split(line.substr(arrow + 2), outputs, ',');
        for (auto &s : outputs)
            s = strip(s);
        ASSERT(!outputs.empty());

        if (outputs.front() == "rx") {
            ASSERT(outputs.size() == 1);
            ASSERT(rx_input_name.empty());
            rx_input_name = name;
        }

        ASSERT(type != -1);
        components_by_name.emplace(name, Component{
                                             .type = type,
                                             .outputs = std::move(outputs),
                                         });
    }
    ASSERT(!rx_input_name.empty());
    ASSERT(components_by_name.count(rx_input_name));

    for (auto &[name, comp] : components_by_name) {
        for (auto &sink_name : comp.outputs) {
            if (auto it = components_by_name.find(sink_name);
                it != components_by_name.end())
                it->second.inputs[name] = 0;
        }
    }

    std::vector<std::string_view> rx_comp_input_names;
    for (auto s : std::views::keys(components_by_name.at(rx_input_name).inputs))
        rx_comp_input_names.push_back(s);
    ASSERT(rx_comp_input_names.size() == 4);

    return Circuit{
        std::move(components_by_name),
        std::move(rx_comp_input_names),
        rx_input_name,
    };
}

struct State {
    dense_map<std::string_view, int64_t> cycle_lengths;
    std::array<int64_t, 2> pulses{{0, 0}};
    int64_t total_cycle_length = 0;
    int64_t presses = 1;
};

struct Pulse {
    std::string_view sink_name;
    std::string_view source_name;
    int value;
};

static void press_button(State &state, Circuit &circuit)
{
    auto &[components_by_name, rx_comp_input_names, rx_input_name] = circuit;
    std::queue<Pulse> pending{};
    pending.emplace("broadcaster", "button", 0);

    while (!pending.empty()) {
        const auto [comp_name, source_name, v] = pending.front();
        pending.pop();
        state.pulses[v]++;

        if (comp_name == rx_input_name && v) {
            state.cycle_lengths.emplace(source_name, state.presses);

            bool done = std::ranges::all_of(rx_comp_input_names, [&](auto s) {
                return state.cycle_lengths.count(s) != 0;
            });
            if (done) {
                state.total_cycle_length = 1;
                for (std::string_view input : rx_comp_input_names)
                    state.total_cycle_length =
                        std::lcm(state.total_cycle_length, state.cycle_lengths[input]);
                return;
            }
        }

        if (comp_name == "rx")
            continue;
        auto &comp = circuit.components_by_name.at(comp_name);

        std::optional<int> outgoing_pulse;
        switch (comp.type) {
        case BROADCASTER:
            outgoing_pulse = v;
            break;

        case FLIPFLOP:
            if (v == 0) {
                comp.on = !comp.on;
                outgoing_pulse = comp.on;
            }
            break;

        case CONJUNCTION:
            comp.inputs[source_name] = v;
            outgoing_pulse.emplace(false);
            for (auto v : std::views::values(comp.inputs))
                *outgoing_pulse |= (v == 0);
            break;
        }

        if (outgoing_pulse.has_value()) {
            for (auto &sink : comp.outputs)
                pending.emplace(sink, comp_name, *outgoing_pulse);
        }
    }
};

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);
    auto circuit = parse_input(lines);

    {
        State state;
        Circuit c = circuit;
        for (int i = 0; i < 1000; i++)
            press_button(state, c);
        fmt::print("{}\n", state.pulses[0] * state.pulses[1]);
    }

    {
        State state;
        for (; state.total_cycle_length == 0; state.presses++)
            press_button(state, circuit);
        fmt::print("{}\n", state.total_cycle_length);
    }
}

}
