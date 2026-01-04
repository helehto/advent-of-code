#include "common.h"
#include "dense_map.h"

namespace aoc_2023_20 {

enum {
    UNTYPED,
    FLIPFLOP,
    CONJUNCTION,
};

struct Component {
    int type = UNTYPED;
    bool on = false;                // flipflop
    uint64_t input_signal_mask = 0; // conjunction
    uint64_t inputs_mask = 0;
    small_vector<uint8_t, 8> outputs;
};

struct Circuit {
    std::vector<Component> components;
    size_t rx_pred_input_index = SIZE_MAX;
    size_t broadcaster_index = SIZE_MAX;
};

static Circuit parse_input(std::string_view buf)
{
    struct ParsedLine {
        int8_t type;
        uint8_t source;
        small_vector<uint8_t, 8> outputs;
    };

    auto lines = split_lines(buf);
    ASSERT(lines.size() < 64);

    Circuit result;
    auto &components = result.components;
    size_t rx_input_index = SIZE_MAX;
    auto &broadcaster_index = result.broadcaster_index;

    dense_map<std::string_view, size_t> node_map;
    node_map.reserve(lines.size());

    std::vector<std::string_view> output_names;

    auto parsed_lines_buffer = std::make_unique_for_overwrite<ParsedLine[]>(lines.size());
    std::span parsed_lines(parsed_lines_buffer.get(), lines.size());

    {
        auto get_node = [&](std::string_view s) {
            ASSERT(node_map.size() <= 64);
            auto [it, inserted] = node_map.try_emplace(s, node_map.size());
            return it->second;
        };

        for (size_t k = 0; std::string_view line : lines) {
            auto &[type, source, outputs] = parsed_lines[k++];

            type = -1;
            if (line.front() == '%')
                type = FLIPFLOP;
            else if (line.front() == '&')
                type = CONJUNCTION;

            if (type >= 0)
                line.remove_prefix(1);

            auto arrow = line.find("->");
            ASSERT(arrow != std::string_view::npos);
            std::string_view name = strip(line.substr(0, arrow));
            source = get_node(name);

            if (name == "broadcaster")
                broadcaster_index = source;

            split(line.substr(arrow + 2), output_names, ',');
            for (std::string_view s : output_names) {
                auto output = get_node(strip(s));
                ASSERT(output < 64);
                ASSERT(source != output);
                outputs.push_back(output);
            }
            ASSERT(!outputs.empty());
        }
    }
    ASSERT_MSG(broadcaster_index < node_map.size(), "No broadcaster node in circuit!?");

    components.resize(node_map.size());
    for (const auto &[type, source, outputs] : parsed_lines) {
        components[source] = Component{
            .type = type,
            .outputs = std::move(outputs),
        };
    }

    // Fill in back-edges (input mask).
    for (size_t i = 0; i < components.size(); i++) {
        const auto &comp = components[i];
        for (auto sink : comp.outputs)
            components[sink].inputs_mask |= UINT64_C(1) << i;
    }

    ASSERT_MSG(node_map.count("rx"), "No rx node in circuit!?");
    rx_input_index = node_map.at("rx");
    auto rx_pred = std::countr_zero(components[rx_input_index].inputs_mask);
    result.rx_pred_input_index = rx_pred;

    return result;
}

struct State {
    dense_map<uint8_t, int64_t> cycle_lengths;
    std::array<int64_t, 2> pulses{{0, 0}};
    int64_t presses = 1;
};

struct Pulse {
    uint8_t sink;
    uint8_t source;
    int value;
};

static int64_t press_button(State &state, Circuit &circuit)
{
    auto &[components, rx_pred_input_index, broadcaster_index] = circuit;
    small_vector<Pulse, 256> pending;

    auto &broadcaster = components[broadcaster_index];
    for (auto sink : broadcaster.outputs)
        pending.emplace_back(sink, broadcaster_index, 0);
    state.pulses[0]++; // account for button press

    auto check_cycle = [&](const Component &comp) -> std::optional<int64_t> {
        int64_t cycle_length = 1;
        for (auto m = comp.inputs_mask; m != 0; m &= m - 1) {
            int input = std::countr_zero(m);
            auto it = state.cycle_lengths.find(input);
            if (it == state.cycle_lengths.end())
                return std::nullopt;
            cycle_length = std::lcm(cycle_length, it->second);
        }
        return cycle_length;
    };

    for (size_t i = 0; i < pending.size(); ++i) {
        const auto [comp_index, source, v] = pending[i];
        auto &comp = circuit.components[comp_index];
        state.pulses[v]++;

        if (v && comp_index == rx_pred_input_index) {
            state.cycle_lengths.emplace(source, state.presses);
            if (auto length = check_cycle(comp))
                return *length;
        }

        std::optional<int> outgoing_pulse;
        if (comp.type == FLIPFLOP) {
            if (v == 0) {
                comp.on = !comp.on;
                outgoing_pulse.emplace(comp.on);
            }
        } else {
            comp.input_signal_mask &= ~(UINT64_C(1) << source);
            if (v)
                comp.input_signal_mask |= (UINT64_C(1) << source);
            outgoing_pulse.emplace(comp.input_signal_mask != comp.inputs_mask);
        }

        if (outgoing_pulse.has_value())
            for (auto sink : comp.outputs)
                pending.emplace_back(sink, comp_index, *outgoing_pulse);
    }

    return 0;
}

void run(std::string_view buf)
{
    auto circuit = parse_input(buf);

    State state;
    for (; state.presses <= 1000; state.presses++)
        press_button(state, circuit);
    fmt::print("{}\n", state.pulses[0] * state.pulses[1]);

    for (;; state.presses++) {
        if (auto cycle_length = press_button(state, circuit)) {
            fmt::print("{}\n", cycle_length);
            break;
        }
    }
}

}
