#include "common.h"
#include "inplace_vector.h"

namespace aoc_2017_18 {

enum class Opcode : uint8_t {
    snd_r,
    snd_i,
    set_ri,
    set_rr,
    add_ri,
    add_rr,
    mul_ri,
    mul_rr,
    mod_ri,
    mod_rr,
    rcv_r,
    jgz_ii,
    jgz_ri,
    jgz_rr,
};

struct Instruction {
    Opcode opcode;
    uint8_t reg;
    int32_t operand;
};

static std::vector<Instruction> assemble(std::span<const std::string_view> lines)
{
    std::vector<std::string_view> tokens;
    tokens.reserve(3);

    auto imm = [](std::string_view s) -> std::optional<int32_t> {
        int32_t n;
        if (std::from_chars(s.data(), s.data() + s.size(), n).ec == std::errc())
            return n;
        return std::nullopt;
    };

    auto parse_line = [&](std::string_view line) -> Instruction {
        split(line, tokens, ' ');
        const std::string_view opcode = tokens[0];
        const std::string_view a = tokens[1];
        const std::string_view b = tokens.size() >= 3 ? tokens[2] : "";
        const uint8_t rd = a[0] - 'a';
        const uint8_t rs = tokens.size() >= 3 ? b[0] - 'a' : 0;

        if (opcode == "snd") {
            if (auto n = imm(a))
                return Instruction(Opcode::snd_i, 0, *n);
            else
                return Instruction(Opcode::snd_r, rd, 0);
        } else if (opcode == "set") {
            if (auto n = imm(b))
                return Instruction(Opcode::set_ri, rd, *n);
            else
                return Instruction(Opcode::set_rr, rd, rs);
        } else if (opcode == "add") {
            if (auto n = imm(b))
                return Instruction(Opcode::add_ri, rd, *n);
            else
                return Instruction(Opcode::add_rr, rd, rs);
        } else if (opcode == "mul") {
            if (auto n = imm(b))
                return Instruction(Opcode::mul_ri, rd, *n);
            else
                return Instruction(Opcode::mul_rr, rd, rs);
        } else if (opcode == "mod") {
            if (auto n = imm(b))
                return Instruction(Opcode::mod_ri, rd, *n);
            else
                return Instruction(Opcode::mod_rr, rd, rs);
        } else if (opcode == "rcv") {
            return Instruction(Opcode::rcv_r, rd);
        } else if (opcode == "jgz") {
            if (auto cond = imm(a)) {
                if (auto offset = imm(b))
                    return Instruction(Opcode::jgz_ii, *cond, *offset);
                else
                    ASSERT(false);
            } else {
                if (auto offset = imm(b))
                    return Instruction(Opcode::jgz_ri, rd, *offset);
                else
                    return Instruction(Opcode::jgz_rr, rd, rs);
            }
        } else {
            ASSERT(false);
        }
    };

    std::vector<Instruction> program;
    program.reserve(lines.size());
    for (std::string_view line : lines)
        program.push_back(parse_line(line));

    return program;
}

struct Program {
    std::span<const Instruction> instrs;
    std::array<int64_t, 26> regs{};
    inplace_vector<int64_t, 256> input_queue;
    size_t pc = 0;
    size_t sent_values = 0;

    bool run(inplace_vector<int64_t, 256> &output_queue)
    {
        for (; pc < instrs.size(); ++pc) {
            auto [opcode, r, op] = instrs[pc];
            auto &rd = regs[r];

            using enum Opcode;
            switch (opcode) {
            case snd_i:
                if (!output_queue.try_push_back(op))
                    return true;
                sent_values++;
                break;
            case snd_r:
                if (!output_queue.try_push_back(rd))
                    return true;
                sent_values++;
                break;
            case set_ri:
                rd = op;
                break;
            case set_rr:
                rd = regs[op];
                break;
            case add_ri:
                rd += op;
                break;
            case add_rr:
                rd += regs[op];
                break;
            case mul_ri:
                rd *= op;
                break;
            case mul_rr:
                rd *= regs[op];
                break;
            case mod_ri:
                rd = modulo<int64_t>(rd, op);
                break;
            case mod_rr:
                rd = modulo<int64_t>(rd, regs[op]);
                break;
            case rcv_r:
                if (input_queue.empty())
                    return false;
                rd = input_queue.front();
                input_queue.erase(input_queue.begin());
                break;
            case jgz_ii:
                if (r > 0)
                    pc += op - 1;
                break;
            case jgz_ri:
                if (rd > 0)
                    pc += op - 1;
                break;
            case jgz_rr:
                if (rd > 0)
                    pc += regs[op] - 1;
                break;
            }
        }

        return true;
    }
};

static int64_t part1(std::span<const Instruction> instrs)
{
    inplace_vector<int64_t, 256> out;

    Program prog{instrs};
    while (prog.run(out))
        out.clear();

    return out.back();
}

static int64_t part2(std::span<const Instruction> instrs)
{
    Program p0{instrs}, p1{instrs};
    Program *running = &p0, *pending = &p1;
    p1.regs['p' - 'a'] = 1;

    do {
        running->run(pending->input_queue);
        std::swap(running, pending);
    } while (!p0.input_queue.empty() || !p1.input_queue.empty());

    return p1.sent_values;
}

void run(std::string_view buf)
{
    auto instrs = assemble(split_lines(buf));

    fmt::print("{}\n", part1(instrs));
    fmt::print("{}\n", part2(instrs));
}

}
