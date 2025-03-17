#include "common.h"
#include "inplace_vector.h"

namespace aoc_2017_23 {

enum class Opcode : uint8_t {
    set_ri,
    set_rr,
    mul_ri,
    mul_rr,
    sub_ri,
    sub_rr,
    jnz_ii,
    jnz_ri,
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

        if (opcode == "set") {
            if (auto n = imm(b))
                return Instruction(Opcode::set_ri, rd, *n);
            else
                return Instruction(Opcode::set_rr, rd, rs);
        } else if (opcode == "mul") {
            if (auto n = imm(b))
                return Instruction(Opcode::mul_ri, rd, *n);
            else
                return Instruction(Opcode::mul_rr, rd, rs);
        } else if (opcode == "sub") {
            if (auto n = imm(b))
                return Instruction(Opcode::sub_ri, rd, *n);
            else
                return Instruction(Opcode::sub_rr, rd, rs);
        } else if (opcode == "jnz") {
            if (auto cond = imm(a)) {
                return Instruction(Opcode::jnz_ii, *cond, *imm(b));
            } else {
                return Instruction(Opcode::jnz_ri, rd, *imm(b));
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
    std::array<int, 8> regs{};
    size_t pc = 0;
    size_t muls = 0;

    void run()
    {
        for (; pc < instrs.size(); ++pc) {
            auto [opcode, r, op] = instrs[pc];
            auto &rd = regs[r];

            using enum Opcode;
            switch (opcode) {
            case set_ri:
                rd = op;
                break;
            case set_rr:
                rd = regs[op];
                break;
            case sub_ri:
                rd -= op;
                break;
            case sub_rr:
                rd -= regs[op];
                break;
            case mul_ri:
                rd *= op;
                ++muls;
                break;
            case mul_rr:
                rd *= regs[op];
                ++muls;
                break;
            case jnz_ii:
                if (r != 0)
                    pc += op - 1;
                break;
            case jnz_ri:
                if (rd != 0)
                    pc += op - 1;
                break;
            }
        }
    }
};

static int64_t part1(std::span<const Instruction> instrs)
{
    Program prog{instrs};
    prog.run();
    return prog.muls;
}

constexpr bool is_composite(int n)
{
    // Fast path for all even inputs.
    if ((n & 1) == 0)
        return true;

    // Enough primes to support testing for compositeness up to 419² = 175561.
    alignas(32) constexpr float primes[]{
        3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,
        61,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107, 109, 113, 127, 131, 137,
        139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227,
        229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313,
        317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419};
    static_assert(std::size(primes) % 8 == 0);

    // Trial division by 8 primes at a time (single-precision floating point
    // math is safe here since the numbers are less than 2²³). A number `n` is
    // composite if there is any d such that d*⌊n/d⌋ = n.
    const __m256 n8 = _mm256_set1_ps(n);
    for (size_t i = 0; i < std::size(primes); i += 8) {
        const __m256 d = _mm256_loadu_ps(&primes[i]);           // d
        const __m256 q = _mm256_floor_ps(_mm256_div_ps(n8, d)); // ⌊n/d⌋
        const __m256 m = _mm256_mul_ps(q, d);                   // d*⌊n/d⌋
        const __m256 c = _mm256_cmp_ps(m, n8, _CMP_EQ_OQ);      // d*⌊n/d⌋ == n
        if (!_mm256_testz_ps(c, c))
            return true;
    }

    return false;
}

static int64_t part2(std::span<const Instruction> instrs)
{
    Program prog{instrs.subspan(0, 15)};
    prog.regs[0] = 1;
    prog.run();

    ASSERT(instrs[instrs.size() - 2].opcode == Opcode::sub_ri);
    int step = -instrs[instrs.size() - 2].operand;

    int composites = 0;
    for (int b = prog.regs[1], c = prog.regs[2]; b <= c; b += step)
        composites += is_composite(b);

    return composites;
}

void run(std::string_view buf)
{
    auto instrs = assemble(split_lines(buf));

    fmt::print("{}\n", part1(instrs));
    fmt::print("{}\n", part2(instrs));
}

}
