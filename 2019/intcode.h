/// This file contains the common Intcode VM implementation for all such
/// problems in 2019.

#pragma once

#include "common.h"
#include "dense_map.h"

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

/// A memory model where memory stored as flat array which has a fixed size
/// after construction. Does not support indexing outside of this.
template <typename ValueT>
struct FlatMemory {
    using address_type = uint32_t;
    using value_type = ValueT;

    std::vector<value_type> mem;

    void reset(std::span<const value_type> init)
    {
        mem.resize(init.size());
        if (!init.empty())
            memcpy(mem.data(), init.data(), init.size() * sizeof(value_type));
    }

    /// Read a value from memory at the address `addr`.
    value_type rd(const value_type addr) { return mem[addr]; }

    /// Write `val` to memory at the address `addr`.
    void wr(const value_type addr, const value_type val) { mem[addr] = val; }
};

/// A memory model where memory is split into 'low' and 'high' memory at a
/// pre-defined address.
///
/// Low memory is backed by a contiguous array, which is fast to access. The
/// program is assumed to be entirely located in low memory, as this makes
/// reading instructions and their operands just a pointer offset.
///
/// High memory is backed by a hash table to save memory and avoid allocating
/// an enormous vector in case the program writes to huge addresses. The
/// trade-off is that each individual memory operation here is (relatively)
/// much more expensive than in low memory.
template <typename ValueT>
struct SplitMemory {
    using address_type = uint32_t;
    using value_type = ValueT;

    constexpr static address_type highmem_start_addr = 8192;

    // Backing containers for low memory and high memory, respectively.
    std::vector<value_type> low;
    dense_map<address_type, value_type> high;

    void reset(std::span<const value_type> init)
    {
        ASSERT(init.size() < highmem_start_addr);
        low.clear();
        low.resize(highmem_start_addr, 0);
        if (!init.empty())
            memcpy(low.data(), init.data(), init.size() * sizeof(value_type));
        high.clear();
    }

    /// Sanity check that `val` is a valid address and cast it to the
    /// appropriate type.
    address_type addr_from_value(const value_type val)
    {
        ASSERT(val >= 0 && static_cast<address_type>(val) <=
                               std::numeric_limits<address_type>::max());
        return static_cast<address_type>(val);
    }

    /// Read a value from memory at the address `addr_value`.
    value_type rd(const value_type addr_value)
    {
        const address_type addr = addr_from_value(addr_value);
        if (addr < highmem_start_addr) [[likely]] {
            return low[addr];
        } else {
            auto it = high.find(addr);
            return it != high.end() ? it->second : 0;
        }
    }

    /// Write `val` to memory at the address `addr_value`.
    void wr(const value_type addr_value, const value_type val)
    {
        const address_type addr = addr_from_value(addr_value);
        if (addr < highmem_start_addr) [[likely]] {
            low[addr] = val;
        } else {
            high[addr] = val;
        }
    }
};

template <typename Memory>
struct IntcodeVM {
    /// Type used for addresses.
    using address_type = uint32_t;

    /// Type used for values.
    using value_type = Memory::value_type;

    /// Backing type for memory.
    Memory mem;

    // Containers for OP_IN (opcode 3) and OP_OUT (opcode 4), respectively.
    std::vector<value_type> input;
    std::vector<value_type> output;

    /// The program counter, containing the address of the currently executing
    /// instruction.
    size_t pc = 0;

    /// The relative base value, modified by OP_SETRBASE (opcode 9) and used by
    /// the relative addressing mode.
    ssize_t relative_base = 0;

    enum class Mode { position, immediate, relative };

    struct DecodedInstruction {
        int opcode;
        Mode operand_modes[3];
    };

    IntcodeVM() { reset(); }

    void reset(std::span<const value_type> prog = {})
    {
        mem.reset(prog);
        input.clear();
        output.clear();
        pc = 0;
        relative_base = 0;
    }

    /// Read the value of the operand at offset `operand_index` from pc, given
    /// the operand modes defined in the instruction `instr`.
    value_type read_op(const DecodedInstruction &instr, const int operand_index)
    {
        const auto operand = mem.rd(pc + operand_index);
        switch (instr.operand_modes[operand_index - 1]) {
        case Mode::position:
            return mem.rd(operand);
        case Mode::immediate:
            return operand;
        case Mode::relative:
            return mem.rd(operand + relative_base);
        }
        std::unreachable();
    }

    /// Write `val` to the address given by the value of the operand at offset
    /// `operand_index` from pc, given the operand modes in the instruction
    /// `instr`.
    void
    write_op(const DecodedInstruction &instr, const int operand_index, value_type val)
    {
        const auto operand = mem.rd(pc + operand_index);
        switch (instr.operand_modes[operand_index - 1]) {
        case Mode::position:
            mem.wr(operand, val);
            break;
        case Mode::immediate:
            ASSERT_MSG(false, "Cannot write to an operand in immediate mode!");
        case Mode::relative:
            mem.wr(operand + relative_base, val);
            break;
        }
    }

    template <int Multiplier>
    static Mode decode_operand_mode(value_type *instr)
    {
        if (*instr < Multiplier)
            return Mode::position;
        if (*instr < 2 * Multiplier) {
            *instr -= 1 * Multiplier;
            return Mode::immediate;
        }
        if (*instr < 3 * Multiplier) {
            *instr -= 2 * Multiplier;
            return Mode::relative;
        }
        [[unlikely]] ASSERT(false);
    }

    static DecodedInstruction decode(value_type instr)
    {
        ASSERT(instr >= 0);
        DecodedInstruction result;
        result.operand_modes[2] = decode_operand_mode<10000>(&instr);
        result.operand_modes[1] = decode_operand_mode<1000>(&instr);
        result.operand_modes[0] = decode_operand_mode<100>(&instr);
        result.opcode = instr;
        return result;
    }

    /// Run the program until it is halted for any reason, which is described
    /// by the return value. Any values passed in `extra_input` are appeneded
    /// to the input vector before executing the program.
    HaltReason run(std::initializer_list<value_type> extra_input = {})
    {
        input.insert(end(input), begin(extra_input), end(extra_input));

        while (true) {
            const DecodedInstruction instr = decode(mem.rd(pc));
            value_type op1;
            value_type op2;

            switch (instr.opcode) {
            case OP_ADD:
            case OP_MUL:
                op1 = read_op(instr, 1);
                op2 = read_op(instr, 2);
                write_op(instr, 3, (instr.opcode & 1) ? op1 + op2 : op1 * op2);
                pc += 4;
                break;

            case OP_IN:
                if (input.empty())
                    return HaltReason::need_input;

                if (input.data() == nullptr) {
                    // For whatever reason, GCC 14.2.1 emits a warning due to
                    // -Warray-bounds inside the .erase() call below, noting
                    // that the "source object is likely at address zero",
                    // despite the fact that vector is definitely not empty at
                    // this point. Tell the compiler that it can assume that
                    // vector buffer is non-null at this point to silence the
                    // warning.
                    std::unreachable();
                }

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
                pc = (!!op1) == (instr.opcode & 1) ? read_op(instr, 2) : pc + 3;
                break;

            case OP_LT:
            case OP_EQ: {
                op1 = read_op(instr, 1);
                op2 = read_op(instr, 2);
                const int lt = (op1 < op2);
                const int eq = (op1 == op2);
                write_op(instr, 3, (instr.opcode & 1) ? lt : eq);
                pc += 4;
            } break;

            case OP_SETRBASE:
                op1 = read_op(instr, 1);
                relative_base += op1;
                pc += 2;
                break;

            default:
                // Check for uncommon opcodes here rather than folding into
                // them into a case statement above, so that [[unlikely]] can
                // be used.
                if (instr.opcode == OP_HALT) [[unlikely]] {
                    return HaltReason::op99;
                } else {
                    ASSERT_MSG(false, "Unknown opcode {}", instr.opcode);
                }
            }
        }
    }
};
