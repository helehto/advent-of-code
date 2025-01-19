#include "common.h"
#include "dense_map.h"

namespace aoc_2019_9 {

enum {
    OP_ADD = 1,
    OP_MUL = 2,
    OP_IN = 3,
    OP_OUT = 4,
    OP_JT = 5,
    OP_JF = 6,
    OP_LT = 7,
    OP_EQ = 8,
    OP_SETRBASE = 9,
    OP_HALT = 99,
};

/// Describes the reason for IntcodeVM::run() to return.
enum class HaltReason {
    /// The program has been permanently halted by operation 99.
    /// IntcodeVM::run() will immediately return with this value again is
    /// called.
    op99,

    /// The program was halted since operation 3 was executed, but no input was
    /// available. IntcodeVM::run() can be invoked again after appending more
    /// elements to the input vector to resume the program.
    need_input,
};

struct IntcodeVM {
    /// Type used for addresses.
    using address_t = uint32_t;

    /// The type stored at each address in memory.
    using value_t = int64_t;

    /// Memory is split into 'low' and 'high' memory at the address defined by
    /// this constant.
    ///
    /// Low memory is backed by a contiguous array, which is fast to access.
    /// The program is assumed to be entirely located in low memory, as this
    /// makes reading instructions and their operands just a pointer offset.
    ///
    /// High memory is backed by a hash table to save memory and avoid
    /// allocating an enormous vector in case the program writes to huge
    /// addresses. The trade-off is that each individual memory operation here
    /// is (relatively) much more expensive than in low memory.
    constexpr static address_t highmem_start_addr = 4096;

    // Backing containers for low memory and high memory, respectively.
    std::vector<value_t> lowmem;
    dense_map<address_t, value_t> highmem;

    // Containers for OP_IN (opcode 3) and OP_OUT (opcode 4), respectively.
    std::vector<value_t> input;
    std::vector<value_t> output;

    /// The program counter, containing the address of the currently executing
    /// instruction.
    size_t pc = 0;

    /// The relative base value, modified by OP_SETRBASE (opcode 9) and used by
    /// the relative addressing mode.
    ssize_t relative_base = 0;

    enum class Mode { position, immediate, relative };

    IntcodeVM() { reset(); }

    void reset(std::span<const value_t> prog = {})
    {
        lowmem.clear();
        lowmem.resize(highmem_start_addr);
        highmem.clear();
        input.clear();
        output.clear();
        pc = 0;
        relative_base = 0;

        for (size_t i = 0; i < prog.size(); ++i)
            wr_addr(i, prog[i]);
    }

    /// Read a value from memory at the address `addr`.
    value_t rd_addr(const value_t addr)
    {
        ASSERT(addr >= 0 && addr <= std::numeric_limits<address_t>::max());
        if (addr < highmem_start_addr)
            return lowmem[addr];

        auto it = highmem.find(static_cast<address_t>(addr));
        return it != highmem.end() ? it->second : 0;
    }

    /// Write `val` to memory at the address `addr`.
    void wr_addr(const value_t addr, const value_t val)
    {
        ASSERT(addr >= 0);
        if (addr < highmem_start_addr) {
            lowmem[addr] = val;
        } else {
            ASSERT(addr <= std::numeric_limits<address_t>::max());
            highmem[addr] = val;
        }
    }

    /// Determine the operand mode of the operand at offset `operand_index`
    /// from pc, given the instruction `instr`.
    Mode operand_mode(const int instr, const int operand_index)
    {
        ASSERT(operand_index >= 1 && operand_index <= 3);

        int mode;
        if (operand_index == 3)
            mode = (instr / 10000) % 10;
        else if (operand_index == 2)
            mode = (instr / 1000) % 10;
        else
            mode = (instr / 100) % 10;

        ASSERT_MSG(mode >= 0 && mode < 3, "Instruction {} has invalid mode!", mode);
        return static_cast<Mode>(mode);
    }

    /// Read the value of the operand at offset `operand_index` from pc, given
    /// the operand modes defined in the instruction `instr`.
    value_t read_op(const int instr, const int operand_index)
    {
        const auto operand = rd_addr(pc + operand_index);
        switch (operand_mode(instr, operand_index)) {
        case Mode::position:
            return rd_addr(operand);
        case Mode::immediate:
            return operand;
        case Mode::relative:
            return rd_addr(operand + relative_base);
        }
        __builtin_unreachable();
    }

    /// Write `val` to the address given by the value of the operand at offset
    /// `operand_index` from pc, given the operand modes in the instruction
    /// `instr`.
    void write_op(const int instr, const int operand_index, value_t val)
    {
        const auto operand = rd_addr(pc + operand_index);
        switch (operand_mode(instr, operand_index)) {
        case Mode::position:
            wr_addr(operand, val);
            break;
        case Mode::immediate:
            ASSERT_MSG(false, "Cannot write to an operand in immediate mode!");
        case Mode::relative:
            wr_addr(operand + relative_base, val);
            break;
        }
    }

    /// Run the program until it is halted for any reason, which is described
    /// by the return value. Any values passed in `extra_input` are appeneded
    /// to the input vector before executing the program.
    HaltReason run(std::initializer_list<value_t> extra_input = {});
};

inline HaltReason IntcodeVM::run(std::initializer_list<value_t> extra_input)
{
    input.insert(end(input), begin(extra_input), end(extra_input));

    while (true) {
        ASSERT(pc < highmem_start_addr);
        const auto instr = lowmem[pc];
        const auto opcode = instr % 100;
        value_t op1;
        value_t op2;

        switch (opcode) {
        case OP_ADD:
        case OP_MUL: {
            op1 = read_op(instr, 1);
            op2 = read_op(instr, 2);
            const value_t sum = op1 + op2;
            const value_t product = op1 * op2;
            write_op(instr, 3, (opcode & 1) ? sum : product);
            pc += 4;
        } break;

        case OP_IN:
            if (input.empty())
                return HaltReason::need_input;

            write_op(instr, 1, input.front());
            input.erase(input.begin());
            pc += 2;
            break;

        case OP_OUT:
            op1 = read_op(instr, 1);
            output.push_back(op1);
            pc += 2;
            break;

        case OP_JT:
        case OP_JF:
            op1 = read_op(instr, 1);
            pc = (!!op1) == (opcode & 1) ? read_op(instr, 2) : pc + 3;
            break;

        case OP_LT:
        case OP_EQ: {
            op1 = read_op(instr, 1);
            op2 = read_op(instr, 2);
            const int lt = (op1 < op2);
            const int eq = (op1 == op2);
            write_op(instr, 3, (opcode & 1) ? lt : eq);
            pc += 4;
        } break;

        case OP_SETRBASE:
            op1 = read_op(instr, 1);
            relative_base += op1;
            pc += 2;
            break;

        default:
            // Check for uncommon opcodes here rather than folding into them
            // into a case statement above, so that __builtin_expect() can be
            // used.
            if (__builtin_expect(opcode == OP_HALT, 0)) {
                return HaltReason::op99;
            } else {
                ASSERT_MSG(false, "Unknown opcode {}", opcode);
            }
        }
    }
}

void run(FILE *f)
{
    auto buf = slurp(f);
    const auto prog = find_numbers<IntcodeVM::value_t>(buf);

    IntcodeVM vm;
    vm.reset(prog);
    vm.run({1});
    fmt::print("{}\n", vm.output.front());

    vm.reset(prog);
    vm.run({2});
    fmt::print("{}\n", vm.output.front());
}

}
