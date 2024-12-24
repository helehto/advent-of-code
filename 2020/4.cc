#include "common.h"
#include <vector>

namespace aoc_2020_4 {

struct Passport {
    enum {
        HAS_BYR = 1 << 0,
        HAS_IYR = 1 << 1,
        HAS_PID = 1 << 2,
        HAS_EYR = 1 << 3,
        HAS_ECL = 1 << 4,
        HAS_HGT = 1 << 5,
        HAS_HCL = 1 << 6,
        HAS_ALL_FIELDS = 0x7f,
    };

    std::string_view byr, iyr, pid, eyr, ecl, hgt, hcl;
    int present_mask = 0;

    static bool valid_year(std::string_view s, int min, int max)
    {
        int value = 0;
        std::from_chars(s.data(), s.data() + s.size(), value);
        return s.size() == 4 && value >= min && value <= max;
    }

    static bool valid_hgt(std::string_view s)
    {
        int value = 0;
        auto r = std::from_chars(s.data(), s.data() + s.size(), value);
        std::string_view unit(r.ptr, s.data() + s.size() - r.ptr);
        return (unit == "cm" && value >= 150 && value <= 193) ||
               (unit == "in" && value >= 59 && value <= 76);
    }

    static bool valid_hcl(std::string_view s)
    {
        if (s.size() != 7 || s[0] != '#')
            return false;
        return std::ranges::all_of(s.substr(1), [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
    }

    static bool valid_ecl(std::string_view s)
    {
        constexpr std::string_view valid[] = {
            "amb", "blu", "brn", "gry", "grn", "hzl", "oth",
        };
        return std::ranges::find(valid, s) != std::end(valid);
    }

    static bool valid_pid(std::string_view s)
    {
        if (s.size() != 9)
            return false;
        return std::ranges::all_of(s, [](char c) { return c >= '0' && c <= '9'; });
    }

    bool valid() const
    {
        return valid_year(byr, 1920, 2002) && valid_year(iyr, 2010, 2020) &&
               valid_year(eyr, 2020, 2030) && valid_pid(pid) && valid_ecl(ecl) &&
               valid_hgt(hgt) && valid_hcl(hcl);
    }
};

void run(FILE *f)
{
    auto [buf, lines] = slurp_lines(f);

    std::vector<std::string_view> fields;
    std::vector<std::string_view> kvs;

    size_t i = 0;
    int valid_part1 = 0;
    int valid_part2 = 0;
    do {
        Passport p;

        for (; i < lines.size() && !lines[i].empty(); i++) {
            split(lines[i], fields, ' ');
            for (std::string_view field : fields) {
                split(field, kvs, ':');

                if (kvs[0] == "byr") {
                    p.byr = kvs[1];
                    p.present_mask |= Passport::HAS_BYR;
                } else if (kvs[0] == "iyr") {
                    p.iyr = kvs[1];
                    p.present_mask |= Passport::HAS_IYR;
                } else if (kvs[0] == "pid") {
                    p.pid = kvs[1];
                    p.present_mask |= Passport::HAS_PID;
                } else if (kvs[0] == "eyr") {
                    p.eyr = kvs[1];
                    p.present_mask |= Passport::HAS_EYR;
                } else if (kvs[0] == "ecl") {
                    p.ecl = kvs[1];
                    p.present_mask |= Passport::HAS_ECL;
                } else if (kvs[0] == "hgt") {
                    p.hgt = kvs[1];
                    p.present_mask |= Passport::HAS_HGT;
                } else if (kvs[0] == "hcl") {
                    p.hcl = kvs[1];
                    p.present_mask |= Passport::HAS_HCL;
                }
            }
        }
        i++;

        if (p.present_mask == Passport::HAS_ALL_FIELDS) {
            valid_part1++;
            valid_part2 += p.valid();
        }
    } while (i < lines.size());

    fmt::print("{}\n", valid_part1);
    fmt::print("{}\n", valid_part2);
}

}
