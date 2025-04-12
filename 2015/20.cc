#include "common.h"
#include "inplace_vector.h"

namespace aoc_2015_20 {

constexpr int32_t highly_abundant_numbers_k[] = {
    0,        2,        3,        4,        6,        8,        10,       12,
    16,       18,       20,       24,       30,       36,       42,       48,
    60,       72,       84,       90,       96,       108,      120,      144,
    168,      180,      210,      216,      240,      288,      300,      336,
    360,      420,      480,      504,      540,      600,      630,      660,
    720,      840,      960,      1008,     1080,     1200,     1260,     1440,
    1560,     1620,     1680,     1800,     1920,     1980,     2100,     2160,
    2340,     2400,     2520,     2880,     3024,     3120,     3240,     3360,
    3600,     3780,     3960,     4200,     4320,     4620,     4680,     5040,
    5760,     5880,     6120,     6240,     6300,     6720,     7200,     7560,
    7920,     8400,     8820,     9240,     10080,    10920,    11340,    11760,
    11880,    12240,    12600,    13440,    13860,    15120,    16380,    16800,
    17640,    18480,    19800,    20160,    21840,    22680,    23760,    25200,
    27720,    30240,    32760,    35280,    36960,    37800,    39600,    40320,
    41580,    42840,    43680,    45360,    47520,    47880,    49140,    50400,
    52920,    54600,    55440,    60480,    64680,    65520,    70560,    73080,
    73920,    75600,    80640,    83160,    85680,    90720,    92400,    95760,
    98280,    100800,   105840,   109200,   110880,   120120,   120960,   126000,
    128520,   131040,   138600,   147840,   151200,   161280,   163800,   166320,
    180180,   181440,   184800,   191520,   194040,   196560,   211680,   214200,
    218400,   221760,   239400,   240240,   249480,   257040,   262080,   277200,
    294840,   302400,   314160,   327600,   332640,   360360,   383040,   388080,
    393120,   415800,   428400,   443520,   458640,   471240,   480480,   491400,
    498960,   514080,   524160,   526680,   540540,   554400,   582120,   589680,
    604800,   609840,   622440,   637560,   655200,   665280,   718200,   720720,
    776160,   786240,   831600,   887040,   914760,   917280,   942480,   982800,
    997920,   1048320,  1053360,  1081080,  1108800,  1164240,  1179360,  1201200,
    1219680,  1244880,  1275120,  1310400,  1330560,  1375920,  1386000,  1413720,
    1441440,  1552320,  1572480,  1580040,  1607760,  1638000,  1663200,  1769040,
    1774080,  1801800,  1884960,  1940400,  1965600,  1995840,  2079000,  2106720,
    2162160,  2328480,  2356200,  2402400,  2439360,  2489760,  2494800,  2522520,
    2550240,  2620800,  2633400,  2661120,  2751840,  2772000,  2827440,  2882880,
    3104640,  3144960,  3160080,  3243240,  3326400,  3492720,  3538080,  3598560,
    3603600,  3769920,  3825360,  3880800,  3931200,  3991680,  4084080,  4158000,
    4213440,  4324320,  4656960,  4712400,  4914000,  4979520,  4989600,  5045040,
    5266800,  5405400,  5569200,  5654880,  5765760,  6098400,  6126120,  6320160,
    6486480,  6652800,  6683040,  6846840,  7068600,  7207200,  7539840,  7567560,
    7650720,  7761600,  7862400,  7900200,  7927920,  8168160,  8288280,  8426880,
    8482320,  8648640,  9313920,  9369360,  9424800,  9729720,  9896040,  9959040,
    9979200,  10090080, 10450440, 10533600, 10810800, 11309760, 11531520, 11891880,
    12113640, 12186720, 12196800, 12252240, 12640320, 12972960, 13693680, 14137200,
    14414400, 15079680, 15135120, 15800400, 16216200, 16576560, 16964640, 17297280,
    17907120, 18018000, 18378360, 18849600, 19126800, 19459440, 19792080, 19958400,
    20049120, 20180160, 20540520, 20900880, 21067200, 21621600, 22619520, 23063040,
    23284800, 23562000, 23783760, 24227280, 24504480, 25225200, 25945920, 26860680,
    27027000, 27387360, 28274400, 28828800, 30270240, 30630600, 31600800, 32432400,
    33153120, 33929280, 34234200, 34594560, 35814240, 36036000, 36756720, 38918880,
    39584160, 40360320, 41081040, 42411600, 42882840, 43243200, 45239040, 45405360,
    46569600, 46781280, 46846800, 47124000, 47401200, 47567520, 48454560, 48648600,
    49008960, 49729680, 50450400, 51891840, 53333280, 53721360, 54054000, 54774720,
    56548800, 57657600, 59099040, 60540480, 61261200, 63201600, 64864800, 66306240,
    67026960, 68468400, 71628480, 72072000, 73513440, 75675600, 77837760, 79168320,
    79279200, 79999920, 81081000, 81681600, 81995760, 82162080, 84823200, 85765680,
    86486400, 89369280, 89535600, 90810720, 91891800, 94802400, 95135040, 95855760,
    97297200, 98017920, 99459360,
};

constexpr int32_t highly_abundant_numbers_sigma_k[] = {
    1,         3,         4,         7,         12,        15,        18,
    28,        31,        39,        42,        60,        72,        91,
    96,        124,       168,       195,       224,       234,       252,
    280,       360,       403,       480,       546,       576,       600,
    744,       819,       868,       992,       1170,      1344,      1512,
    1560,      1680,      1860,      1872,      2016,      2418,      2880,
    3048,      3224,      3600,      3844,      4368,      4914,      5040,
    5082,      5952,      6045,      6120,      6552,      6944,      7440,
    7644,      7812,      9360,      9906,      9920,      10416,     10890,
    12096,     12493,     13440,     14040,     14880,     15120,     16128,
    16380,     19344,     19890,     20520,     21060,     21168,     22568,
    24384,     25389,     28800,     29016,     30752,     31122,     34560,
    39312,     40320,     40656,     42408,     43200,     43524,     48360,
    48960,     52416,     59520,     61152,     62496,     66690,     71424,
    72540,     79248,     83328,     87120,     89280,     99944,     112320,
    120960,    131040,    137826,    145152,    148800,    149916,    159120,
    161280,    168480,    169344,    180048,    181440,    187200,    188160,
    203112,    205200,    208320,    232128,    243840,    246240,    270816,
    280098,    280800,    292608,    307520,    318864,    345600,    348192,
    365904,    369024,    386880,    403200,    409448,    424080,    430528,
    471744,    483840,    489600,    502944,    518400,    550368,    580320,
    587520,    624960,    638352,    677040,    714240,    733824,    737616,
    749952,    786240,    800280,    833280,    861840,    870480,    874944,
    950976,    967200,    999936,    1045440,   1071360,   1109472,   1199328,
    1219680,   1259840,   1285632,   1399216,   1451520,   1572480,   1584960,
    1653912,   1693440,   1785600,   1798992,   1909440,   1929564,   2021760,
    2032128,   2083200,   2160576,   2177280,   2227680,   2246400,   2257920,
    2437344,   2462400,   2520672,   2529600,   2572752,   2620800,   2695680,
    2843568,   2926080,   2976000,   3249792,   3361176,   3413760,   3690240,
    3826368,   3830400,   3921372,   4178304,   4305280,   4390848,   4464096,
    4642560,   4838400,   4913376,   5088960,   5122656,   5166336,   5228496,
    5416320,   5571072,   5732272,   5875200,   5937120,   6035328,   6220800,
    6604416,   6775704,   6854400,   6912000,   6963840,   7041216,   7499520,
    7582848,   7660224,   8124480,   8491392,   8545212,   8749440,   8851392,
    8985600,   9434880,   9999360,   10342080,  10445760,  10499328,  10539984,
    11007360,  11162976,  11203920,  11321856,  11509680,  11606400,  11773440,
    12065760,  12265344,  12856320,  13313664,  13604760,  13735680,  14284800,
    14636160,  15118080,  15394104,  15410304,  15513120,  16790592,  17117568,
    17141760,  17366076,  17637760,  17772480,  17998848,  18570240,  19019520,
    20321280,  20848320,  21587904,  21665280,  22189440,  22686048,  23154768,
    23986560,  24998400,  25185888,  26127360,  26732160,  27013896,  28304640,
    29030400,  30248064,  30355200,  30481920,  31449600,  32140800,  34122816,
    34369920,  34473600,  34836480,  35007804,  35414400,  35712000,  36018528,
    36578304,  37739520,  38188800,  38890368,  40965120,  41860800,  42479424,
    43872192,  44029440,  44323200,  44553600,  45732192,  47056464,  47174400,
    48746880,  51663360,  52669440,  53569152,  53625600,  53913600,  54190080,
    54456584,  58496256,  58521600,  61471872,  64995840,  66424320,  68787264,
    68874624,  71245440,  73804800,  75620160,  77995008,  79035264,  82252800,
    83566080,  84494592,  87091200,  88440768,  88565760,  90994176,  91601280,
    91824480,  92207808,  94859856,  96768000,  97493760,  98267520,  104993280,
    105753600, 107243136, 107716320, 108635904, 110826240, 111421440, 118879488,
    119632968, 123919488, 124416000, 125798400, 132088320, 134991360, 138116160,
    144789120, 146240640, 149990400, 156281664, 158505984, 159325056, 162489600,
    164828160, 169827840, 171714816, 179988480, 184923648, 186157440, 190466640,
    199987200, 200933568, 201670560, 211653120, 211921920, 215517456, 216280800,
    217183680, 219477024, 220776192, 223259520, 225227520, 226437120, 227485440,
    239645952, 239984640, 243125064, 248814720, 250967808, 257126400, 259983360,
    266273280, 272125440, 276773952, 277385472, 291876480, 302230656, 302361600,
    317604672, 319527936, 319979520, 335811840, 342351360, 346155264, 365783040,
    368101440, 372782592, 375269760, 378194544, 379975680, 380540160, 380975616,
    385689600, 406425600, 408348864, 416785824, 424972800, 426037248, 431758080,
    437987088, 449971200, 453720960, 454030080, 463095360, 470136576, 481178880,
    487710720,
};

static_assert(std::size(highly_abundant_numbers_k) ==
              std::size(highly_abundant_numbers_sigma_k));

static int part1(const int N)
{
    // As the number of presents for a house `k` is 10*σ(k), where σ denotes
    // the sum of divisors, we need to find the first house k for which σ(k) ≥
    // N/10.
    //
    // "Record-breaking" numbers, for which σ(k) > σ(m) for all m < k, are
    // called highly abundant numbers. The two tables above include n and σ(n),
    // respectively, for n ≤ 100,000,000. To solve the problem, we simply
    // binary search for the smallest admissible value of σ(n) and look up the
    // corresponding value of n.
    //
    // For more information:
    //
    //   https://en.wikipedia.org/wiki/Highly_abundant_number
    //   https://oeis.org/A002093
    //
    // (The tables were generated and copied here to avoid bloating compile
    // time with constexpr. See attic/highly-abundant-numbers.cc for the code
    // that generated them.)

    const int32_t *p = std::ranges::lower_bound(highly_abundant_numbers_sigma_k, N / 10);
    return highly_abundant_numbers_k[p - std::begin(highly_abundant_numbers_sigma_k)];
}

/// All primes less than 64.
constexpr int primes[] = {2,  3,  5,  7,  11, 13, 17, 19, 23,
                          29, 31, 37, 41, 43, 47, 53, 59, 61};

/// Compute a 64-bit bitset denoting which integers from 0-63 are divisors of
/// the given integer (represented as a prime signature).
static uint64_t signature_divisor_mask(std::span<const int> signature,
                                       const uint64_t acc = 1,
                                       const size_t i = 0)
{
    uint64_t result = 0;
    if (acc < 64)
        result = uint64_t(1) << acc;

    if (i < signature.size())
        for (int j = 0, p = 1; j <= signature[i]; ++j, p *= primes[i])
            result |= signature_divisor_mask(signature, acc * p, i + 1);

    return result;
}

/// Given a house number `k` and its corresponding prime signature
/// representation, compute the number of presents at that house for part 2.
static int64_t num_house_presents(const uint64_t k, std::span<const int> signature)
{
    int64_t result = 1;

    // Only the first 50 divisors matter; mask off everything else. (Or in
    // other words: only count elves that need at most 50 steps to reach this
    // house.)
    uint64_t m = signature_divisor_mask(signature) & ((uint64_t(1) << 51) - 1);

    for (; m; m &= m - 1)
        result += static_cast<int32_t>(static_cast<float>(k) /
                                       static_cast<float>(std::countr_zero(m)));

    return 11 * result;
}

/// Recursively generate house numbers via prime signatures, evaluating the
/// number of presents and keeping track of the house with an admissible number
/// of presente and lowest house number so far in `lowest`.
static void search(const int N,
                   int k,
                   inplace_vector<int, 16> &signature,
                   std::span<const int> primes,
                   int &lowest)
{
    if (num_house_presents(k, signature) >= N)
        lowest = std::min(lowest, k);

    const int max_exponent = signature.empty() ? INT_MAX : signature.back();
    signature.push_back(0);

    while (signature.back() + 1 <= max_exponent) {
        k *= primes.front();
        if (k > lowest)
            break;
        signature.back()++;
        search(N, k, signature, primes.subspan(1), lowest);
    }

    signature.pop_back();
}

static int part2(const int N)
{
    inplace_vector<int, 16> signature;
    int lowest = INT_MAX;
    search(N, 1, signature, primes, lowest);
    return lowest;
}

void run(std::string_view buf)
{
    auto N = find_numbers_n<int, 1>(buf)[0];
    fmt::print("{}\n", part1(N));
    fmt::print("{}\n", part2(N));
}

}
