#include "common.h"
#include "inplace_vector.h"
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

// Later on, we cast a void* received from mmap() into a function pointer to
// execute the emitted machine code, which is only conditionally supported.
// Since we assume GCC/Clang on x86-64 Linux, it works, so we don't care.
#pragma GCC diagnostic ignored "-Wconditionally-supported"

namespace aoc_2017_25 {

struct State {
    std::array<int, 2> write;
    std::array<int, 2> move;
    std::array<int, 2> transition;
};

struct Relocation {
    enum Kind : uint8_t { generic, ret_block_offset, state_jmp_offset };

    Kind kind;
    uint32_t arg;
    uint8_t *p;
    int32_t offset;

    void apply(int32_t value) const
    {
        value += offset;
        memcpy(p, &value, sizeof(value));
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

    Relocation make_jmp_relocation(Relocation::Kind kind = Relocation::Kind::generic,
                                   uint32_t arg = 0)
    {
        return Relocation{
            .kind = kind,
            .arg = arg,
            .p = out - 4,
            .offset = -4,
        };
    }

    void add_jmp_relocation(Relocation::Kind kind, uint32_t arg = 0)
    {
        relocations.push_back(make_jmp_relocation(kind, arg));
    }
};

/// Assemble x86-64 machine code for handling a single state of the Turing
/// machine.
static void emit_single_state(Assembler &as, const State &state)
{
    // dec %rsi
    as.emit(0x48, 0xff, 0xce);

    // jz <ret>
    as.emit(0x0f, 0x84, 0x00, 0x00, 0x00, 0x00);
    as.add_jmp_relocation(Relocation::Kind::ret_block_offset);

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
    as.add_jmp_relocation(Relocation::Kind::state_jmp_offset, state.transition[0]);

    // .eq1: movb $X, (%rdi)
    eq1.apply(as.out - eq1.p);
    as.emit(0xc6, 0x07, state.write[1]);

    // lea X(%rdi), %rdi
    as.emit(0x48, 0x8d, 0x7f, state.move[1]);

    // jmp <next-state-if-1>
    as.emit(0xe9, 0x00, 0x00, 0x00, 0x00);
    as.add_jmp_relocation(Relocation::Kind::state_jmp_offset, state.transition[1]);
}

/// Assemble x86-64 machine code for a specialized function that simulates the
/// Turing machine described in `states`.
///
/// The signature of the constructed function is void(uint8_t *head, size_t n),
/// where `head` is the initial head of the tape and `n` is the number of
/// execution steps.
///
/// By the calling convention, %rdi contains `head` and %rsi contains `n` at
/// entry; these are kept in the same registers throughout the function.
static size_t assemble_turing_machine(uint8_t *out, std::span<const State> states)
{
    Assembler as(out);

    // jmp .Lstates
    // .Lret: ret
    as.emit(0xeb, 0x01);
    const uint8_t *ret_block = as.out;
    as.emit(0xc3);

    // .Lstates: (this is where the code for all blocks are laid out, in order.)
    small_vector<uint8_t *> state_blocks(states.size());
    for (size_t s = 0; s < 6; ++s) {
        state_blocks[s] = as.out;
        emit_single_state(as, states[s]);
    }

    // Apply all relocations.
    for (const Relocation &rel : as.relocations) {
        switch (rel.kind) {
        case Relocation::Kind::ret_block_offset:
            rel.apply(ret_block - rel.p);
            break;
        case Relocation::Kind::state_jmp_offset:
            rel.apply(state_blocks[rel.arg] - rel.p);
            break;
        case Relocation::Kind::generic:
            ASSERT(false);
        }
    }

    return as.out - out;
}

void run(std::string_view buf)
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

    auto *simulate = reinterpret_cast<void (*)(uint8_t *, size_t)>(buffer);

    simulate(tape.data() + tape.size() / 2, n + 1);
    munmap(buffer, getpagesize());
    fmt::print("{}\n", std::ranges::count(tape, 1));
}

}
