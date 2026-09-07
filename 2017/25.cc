#include <algorithm>
#include <aoc/base.h>
#include <aoc/inplace_vector.h>
#include <aoc/macros.h>
#include <aoc/small_vector.h>
#include <aoc/string.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#ifdef __x86_64__
constexpr bool is_x86_64 = true;
#elifdef __aarch64__
constexpr bool is_x86_64 = false;
#else
#error "Sorry, this monstrosity only supports x86-64 and AArch64."
#endif

// Later on, we cast a void* received from mmap() into a function pointer to
// execute the emitted machine code, which is only conditionally supported.
// Since we assume GCC/Clang on x86-64 or AArch64 Linux, it works, so we don't
// care.
#pragma GCC diagnostic ignored "-Wconditionally-supported"

namespace aoc_2017_25 {

struct State {
    std::array<int, 2> write;
    std::array<int, 2> move;
    std::array<int, 2> transition;
};

struct Relocation {
    enum Kind : uint8_t {
        generic,
        x86_64_ret_block_offset,
        x86_64_state_jmp_offset,
        aarch64_ret_block_offset,
        aarch64_state_jmp_offset,
    };

    Kind kind;
    uint32_t arg;
    uint8_t *p;
    int32_t offset;

    void apply(int32_t value) const
    {
        value += offset;
        memcpy(p, &value, sizeof(value));
    }

    void apply_aarch64_mask_shifted(int32_t value, int32_t mask, int shift) const
    {
        ASSERT((value + offset) % 4 == 0);
        value = (value + offset) / 4;
        ASSERT(value >= -mask && value <= mask);
        const auto mask_shifted = static_cast<uint32_t>(mask) << shift;
        uint32_t insn;
        memcpy(&insn, p, sizeof(insn));
        insn = (insn & ~mask_shifted) | ((value << shift) & mask_shifted);
        memcpy(p, &insn, sizeof(insn));
    }

    void apply_aarch64_condbr19(int32_t value) const
    {
        apply_aarch64_mask_shifted(value, 0x7ffff, 5);
    }

    void apply_aarch64_jump26(int32_t value) const
    {
        apply_aarch64_mask_shifted(value, 0x03ffffff, 0);
    }
};

struct Assembler {
    uint8_t *out;
    small_vector<Relocation> relocations;

    template <typename... Args>
    void emit(Args... args)
    {
        uint8_t *p = out;
        ((*p++ = static_cast<uint8_t>(args)), ...);
        out = p;
    }

    void emit32(uint32_t value)
    {
        emit(static_cast<uint8_t>(value & 0xff),
             static_cast<uint8_t>((value >> 8) & 0xff),
             static_cast<uint8_t>((value >> 16) & 0xff),
             static_cast<uint8_t>((value >> 24) & 0xff));
    }

    Relocation make_jmp_relocation(Relocation::Kind kind = Relocation::Kind::generic,
                                   uint32_t arg = 0)
    {
        return Relocation{
            .kind = kind,
            .arg = arg,
            .p = out - 4,
            .offset = is_x86_64 ? -4 : 0, // TODO: ugh
        };
    }

    void add_jmp_relocation(Relocation::Kind kind, uint32_t arg = 0)
    {
        relocations.push_back(make_jmp_relocation(kind, arg));
    }
};

static const uint8_t *emit_prologue_x86_64(Assembler &as)
{
    // jmp .Lstates
    // .Lret: ret
    as.emit(0xeb, 0x01);
    const uint8_t *ret_block = as.out;
    as.emit(0xc3);
    return ret_block;
}

/// Assemble x86-64 machine code for handling a single state of the Turing
/// machine.
static void emit_single_state_x86_64(Assembler &as, const State &state)
{
    // dec %rsi
    as.emit(0x48, 0xff, 0xce);

    // jz <ret>
    as.emit(0x0f, 0x84, 0x00, 0x00, 0x00, 0x00);
    as.add_jmp_relocation(Relocation::Kind::x86_64_ret_block_offset);

    // cmpb $0, (%rdi)
    as.emit(0x80, 0x3f, 0x00);

    // jnz .eq1
    as.emit(0x0f, 0x85, 0x00, 0x00, 0x00, 0x00);
    const Relocation eq1 = as.make_jmp_relocation();

    // movb $X, (%rdi)
    as.emit(0xc6, 0x07, state.write[0]);

    // lea X(%rdi), %rdi
    as.emit(0x48, 0x8d, 0x7f, state.move[0]);

    // jmp <next-state-if-0>
    as.emit(0xe9, 0x00, 0x00, 0x00, 0x00);
    as.add_jmp_relocation(Relocation::Kind::x86_64_state_jmp_offset, state.transition[0]);

    // .eq1: movb $X, (%rdi)
    eq1.apply(as.out - eq1.p);
    as.emit(0xc6, 0x07, state.write[1]);

    // lea X(%rdi), %rdi
    as.emit(0x48, 0x8d, 0x7f, state.move[1]);

    // jmp <next-state-if-1>
    as.emit(0xe9, 0x00, 0x00, 0x00, 0x00);
    as.add_jmp_relocation(Relocation::Kind::x86_64_state_jmp_offset, state.transition[1]);
}

static const uint8_t *emit_prologue_aarch64(Assembler &as)
{
    // Set up a real stack frame. Not really necessary in this case, but it
    // helps with debugging (which is already painful enough as it is).
    as.emit32(0xa9bf7bfd); // stp x29, x30, [sp, #-16]!
    as.emit32(0x910003fd); // mov x29, sp
    as.emit32(0x14000000); // b .Lstates
    auto rel = as.make_jmp_relocation(Relocation::Kind::aarch64_state_jmp_offset);
    const uint8_t *ret_block = as.out;
    as.emit32(0xa8c17bfd); // .Lret: ldp x29, x30, [sp], #16
    as.emit32(0xd65f03c0); // ret
    rel.apply_aarch64_jump26(as.out - rel.p);
    return ret_block;
}

/// Assemble ARMv8 machine code for handling a single state of the Turing
/// machine.
static void emit_single_state_aarch64(Assembler &as, const State &state)
{
    // sub x1, x1, #1
    // cbz x1, <ret>
    as.emit32(0xd1000421);
    as.emit32(0xb4000081);
    as.add_jmp_relocation(Relocation::Kind::aarch64_ret_block_offset);

    // ldrb w2, [x0]
    // cbnz x2, .eq1
    as.emit32(0x39400002);
    as.emit32(0xb5000002);
    auto eq1 = as.make_jmp_relocation();

    // movz w9, #X  (where X is 0 or 1)
    // strb w9, [x0]
    as.emit32(0x52800000 | (state.write[0] << 5) | 9);
    as.emit32(0x39000009);

    // {add|sub} x0, x0, #1; depending on move direction
    as.emit32(state.move[0] > 0 ? 0x91000400 : 0xd1000400);

    // b <next-state-if-0>
    as.emit32(0x14000000);
    as.add_jmp_relocation(Relocation::Kind::aarch64_state_jmp_offset,
                          state.transition[0]);

    // .eq1:
    // movz w9, #X  (where X is 0 or 1)
    // strb w9, [x0]
    eq1.apply_aarch64_condbr19(as.out - eq1.p);
    as.emit32(0x52800000 | (state.write[1] << 5) | 9);
    as.emit32(0x39000009);

    // {add|sub} x0, x0, #1; depending on move direction
    as.emit32(state.move[1] > 0 ? 0x91000400 : 0xd1000400);

    // b <next-state-if-1>
    as.emit32(0x14000000);
    as.add_jmp_relocation(Relocation::Kind::aarch64_state_jmp_offset,
                          state.transition[1]);
}

static const uint8_t *emit_prologue(Assembler &as)
{
    if constexpr (is_x86_64)
        return emit_prologue_x86_64(as);
    else
        return emit_prologue_aarch64(as);
}

static void emit_single_state(Assembler &as, const State &state)
{
    if constexpr (is_x86_64)
        emit_single_state_x86_64(as, state);
    else
        emit_single_state_aarch64(as, state);
}

/// Assemble machine code for a specialized function that simulates the Turing
/// machine described in `states`.
///
/// The signature of the constructed function is void(uint8_t *head, size_t n),
/// where `head` is the initial head of the tape and `n` is the number of
/// execution steps.
///
/// On entry, following the calling convention:
///
///     * x86-64: %rdi contains `head`, %rsi contains `n`
///     * Aarch64: x0 contains `head`, x1 contains `n`
///
/// These are kept in the same registers throughout the function.
static size_t assemble_turing_machine(uint8_t *out, std::span<const State> states)
{
    Assembler as(out);
    const uint8_t *ret_block = emit_prologue(as);

    // .Lstates: (this is where the code for all blocks are laid out, in order.)
    small_vector<uint8_t *> state_blocks(states.size());
    for (size_t s = 0; s < 6; ++s) {
        state_blocks[s] = as.out;
        emit_single_state(as, states[s]);
    }

    // Apply all relocations.
    for (const Relocation &rel : as.relocations) {
        switch (rel.kind) {
        case Relocation::Kind::x86_64_ret_block_offset:
            rel.apply(ret_block - rel.p);
            break;
        case Relocation::Kind::x86_64_state_jmp_offset:
            rel.apply(state_blocks[rel.arg] - rel.p);
            break;
        case Relocation::Kind::aarch64_ret_block_offset:
            rel.apply_aarch64_condbr19(ret_block - rel.p);
            break;
        case Relocation::Kind::aarch64_state_jmp_offset:
            rel.apply_aarch64_jump26(state_blocks[rel.arg] - rel.p);
            break;
        case Relocation::Kind::generic:
            ASSERT(false);
        }
    }

    return as.out - out;
}

void run(std::string_view buf, aoc::Answer &answer)
{
    auto lines = split_lines(buf);
    auto [n] = find_numbers_n<int, 1>(lines[1]);

    inplace_vector<State, 16> states;

    for (size_t i = 4; i < lines.size(); i += 10) {
        State s;
        s.write = {
            find_numbers_n<int, 1>(lines[i + 1])[0],
            find_numbers_n<int, 1>(lines[i + 5])[0],
        };
        s.move = {
            lines[i + 2].ends_with("left.") ? -1 : 1,
            lines[i + 6].ends_with("left.") ? -1 : 1,
        };
        s.transition = {
            lines[i + 3][lines[i + 3].size() - 2] - 'A',
            lines[i + 7][lines[i + 7].size() - 2] - 'A',
        };
        states.push_back(s);
    }

    std::vector<uint8_t> tape(100'000);

    void *buffer = mmap(nullptr, getpagesize(), PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED)
        ASSERT_MSG(false, "mmap: {}", strerror(errno));

    assemble_turing_machine(static_cast<uint8_t *>(buffer), states);

    if (mprotect(buffer, getpagesize(), PROT_READ | PROT_EXEC) < 0)
        ASSERT_MSG(false, "mprotect: {}", strerror(errno));

    // Ensure icache coherency, required on e.g. AArch64, before executing the
    // generated code.
    __builtin___clear_cache(static_cast<char *>(buffer),
                            static_cast<char *>(buffer) + getpagesize());

    auto *simulate = reinterpret_cast<void (*)(uint8_t *, size_t)>(buffer);

    simulate(tape.data() + tape.size() / 2, n + 1);
    munmap(buffer, getpagesize());
    answer.add(std::ranges::count(tape, 1));
}
AOC_REGISTER_SOLVER(2017, 25, run);

}
