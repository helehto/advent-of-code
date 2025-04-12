#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <vector>

int main()
{
    std::vector<uint64_t> a(100'000'000, 1);

    for (size_t d = 2; d < a.size(); ++d)
        for (size_t k = d; k < a.size(); k += d)
            a[k] += d;

    std::vector<uint64_t> ks;
    std::vector<uint64_t> sigmas;

    size_t max = 0;
    for (size_t k = 0; k < a.size(); ++k) {
        if (a[k] > max) {
            ks.push_back(k);
            sigmas.push_back(a[k]);
            max = a[k];
        }
    }

    fmt::print("constexpr int32_t highly_abundant_numbers_k[] = {{\n{},\n}};\n\n",
               fmt::join(ks, ","));
    fmt::print("constexpr int32_t highly_abundant_numbers_sigma_k[] = {{\n{},\n}};\n\n",
               fmt::join(sigmas, ","));
}
