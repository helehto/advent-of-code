#include "common.h"
#include "dense_map.h"

namespace r = std::ranges;

static int hash(std::string_view f)
{
    uint8_t hash = 0;
    for (uint8_t c : f)
        hash = (hash + c) * 17;
    return hash;
}

static int part1(const std::vector<std::string_view> &fields)
{
    int sum = 0;
    for (auto f : fields)
        sum += hash(f);
    return sum;
}

static int part2(const std::vector<std::string_view> &fields)
{
    std::vector<std::pair<std::string_view, int>> boxes[256];
    dense_map<std::string_view, int> lens_to_box;
    lens_to_box.reserve(fields.size());

    for (auto f : fields) {
        const auto sep = f.find_first_of("=-");
        ASSERT(sep != std::string_view::npos);
        const auto lens = f.substr(0, sep);
        const auto h = hash(lens);
        auto &box = boxes[h];

        auto it = r::find_if(box, [&](auto p) { return p.first == lens; });

        if (f[sep] == '-') {
            if (it != box.end()) {
                box.erase(it);
                lens_to_box.erase(lens);
            }
        } else if (it != box.end()) {
            it->second = f[sep + 1] - '0';
        } else {
            box.emplace_back(lens, f[sep + 1] - '0');
            lens_to_box.emplace(lens, h);
        }
    }

    int sum = 0;
    for (auto &[lens, h] : lens_to_box) {
        const auto &box = boxes[h];
        auto jt = r::find_if(box, [&](auto p) { return p.first == lens; });
        ASSERT(jt != box.end());
        sum += (h + 1) * (jt - box.begin() + 1) * jt->second;
    }

    return sum;
}

void run_2023_15(FILE *f)
{
    std::string s;
    getline(f, s);
    std::vector<std::string_view> fields;
    split(s, fields, [&](char c) { return c == ','; });

    fmt::print("{}\n", part1(fields));
    fmt::print("{}\n", part2(fields));
}
