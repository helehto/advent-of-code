#include "common.h"

// See the 'Cosmological decay' section on Wikipedia:
// https://en.wikipedia.org/wiki/Look-and-say_sequence

static constexpr std::string_view atomic_names[] = {
    "",   "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne", "Na", "Mg", "Al",
    "Si", "P",  "S",  "Cl", "Ar", "K",  "Ca", "Sc", "Ti", "V",  "Cr", "Mn", "Fe", "Co",
    "Ni", "Cu", "Zn", "Ga", "Ge", "As", "Se", "Br", "Kr", "Rb", "Sr", "Y",  "Zr", "Nb",
    "Mo", "Tc", "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I",  "Xe", "Cs",
    "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb", "Dy", "Ho", "Er", "Tm",
    "Yb", "Lu", "Hf", "Ta", "W",  "Re", "Os", "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi",
    "Po", "At", "Rn", "Fr", "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu",
};
static constexpr size_t NUM_ATOMS = std::size(atomic_names);

static consteval uint64_t lookup(std::string_view v)
{
    if (v == "")
        return 0;

    int i = 0;
    for (auto name : atomic_names) {
        if (v == name)
            return i;
        i++;
    }

    throw "bad atom name";
}

static consteval uint64_t make_decay(std::string_view a0,
                                     std::string_view a1 = "",
                                     std::string_view a2 = "",
                                     std::string_view a3 = "",
                                     std::string_view a4 = "",
                                     std::string_view a5 = "",
                                     std::string_view a6 = "")
{
    return lookup(a0) | lookup(a1) << 8 | lookup(a2) << 16 | lookup(a3) << 24 |
           lookup(a4) << 32 | lookup(a5) << 40 | lookup(a6) << 48;
}

static constexpr uint64_t atomic_decay[] = {
    0,
    make_decay("H"),
    make_decay("Hf", "Pa", "H", "Ca", "Li"),
    make_decay("He"),
    make_decay("Ge", "Ca", "Li"),
    make_decay("Be"),
    make_decay("B"),
    make_decay("C"),
    make_decay("N"),
    make_decay("O"),
    make_decay("F"),
    make_decay("Ne"),
    make_decay("Pm", "Na"),
    make_decay("Mg"),
    make_decay("Al"),
    make_decay("Ho", "Si"),
    make_decay("P"),
    make_decay("S"),
    make_decay("Cl"),
    make_decay("Ar"),
    make_decay("K"),
    make_decay("Ho", "Pa", "H", "Ca", "Co"),
    make_decay("Sc"),
    make_decay("Ti"),
    make_decay("V"),
    make_decay("Cr", "Si"),
    make_decay("Mn"),
    make_decay("Fe"),
    make_decay("Zn", "Co"),
    make_decay("Ni"),
    make_decay("Cu"),
    make_decay("Eu", "Ca", "Ac", "H", "Ca", "Zn"),
    make_decay("Ho", "Ga"),
    make_decay("Ge", "Na"),
    make_decay("As"),
    make_decay("Se"),
    make_decay("Br"),
    make_decay("Kr"),
    make_decay("Rb"),
    make_decay("Sr", "U"),
    make_decay("Y", "H", "Ca", "Tc"),
    make_decay("Er", "Zr"),
    make_decay("Nb"),
    make_decay("Mo"),
    make_decay("Eu", "Ca", "Tc"),
    make_decay("Ho", "Ru"),
    make_decay("Rh"),
    make_decay("Pd"),
    make_decay("Ag"),
    make_decay("Cd"),
    make_decay("In"),
    make_decay("Pm", "Sn"),
    make_decay("Eu", "Ca", "Sb"),
    make_decay("Ho", "Te"),
    make_decay("I"),
    make_decay("Xe"),
    make_decay("Cs"),
    make_decay("Ba"),
    make_decay("La", "H", "Ca", "Co"),
    make_decay("Ce"),
    make_decay("Pr"),
    make_decay("Nd"),
    make_decay("Pm", "Ca", "Zn"),
    make_decay("Sm"),
    make_decay("Eu", "Ca", "Co"),
    make_decay("Ho", "Gd"),
    make_decay("Tb"),
    make_decay("Dy"),
    make_decay("Ho", "Pm"),
    make_decay("Er", "Ca", "Co"),
    make_decay("Tm"),
    make_decay("Yb"),
    make_decay("Lu"),
    make_decay("Hf", "Pa", "H", "Ca", "W"),
    make_decay("Ta"),
    make_decay("Ge", "Ca", "W"),
    make_decay("Re"),
    make_decay("Os"),
    make_decay("Ir"),
    make_decay("Pt"),
    make_decay("Au"),
    make_decay("Hg"),
    make_decay("Tl"),
    make_decay("Pm", "Pb"),
    make_decay("Bi"),
    make_decay("Po"),
    make_decay("Ho", "At"),
    make_decay("Rn"),
    make_decay("Fr"),
    make_decay("Ra"),
    make_decay("Ac"),
    make_decay("Th"),
    make_decay("Pa"),
    make_decay("Hf", "Pa", "H", "Ca", "Pu"),
    make_decay("Np"),
};
static_assert(std::size(atomic_decay) == NUM_ATOMS);

static constexpr std::string_view atomic_sequences[] = {
    "",
    "22",
    "13112221133211322112211213322112",
    "312211322212221121123222112",
    "111312211312113221133211322112211213322112",
    "1321132122211322212221121123222112",
    "3113112211322112211213322112",
    "111312212221121123222112",
    "132112211213322112",
    "31121123222112",
    "111213322112",
    "123222112",
    "3113322112",
    "1113222112",
    "1322112",
    "311311222112",
    "1113122112",
    "132112",
    "3112",
    "1112",
    "12",
    "3113112221133112",
    "11131221131112",
    "13211312",
    "31132",
    "111311222112",
    "13122112",
    "32112",
    "11133112",
    "131112",
    "312",
    "13221133122211332",
    "31131122211311122113222",
    "11131221131211322113322112",
    "13211321222113222112",
    "3113112211322112",
    "11131221222112",
    "1321122112",
    "3112112",
    "1112133",
    "12322211331222113112211",
    "1113122113322113111221131221",
    "13211322211312113211",
    "311322113212221",
    "132211331222113112211",
    "311311222113111221131221",
    "111312211312113211",
    "132113212221",
    "3113112211",
    "11131221",
    "13211",
    "3112221",
    "1322113312211",
    "311311222113111221",
    "11131221131211",
    "13211321",
    "311311",
    "11131",
    "1321133112",
    "31131112",
    "111312",
    "132",
    "311332",
    "1113222",
    "13221133112",
    "3113112221131112",
    "111312211312",
    "1321132",
    "311311222",
    "11131221133112",
    "1321131112",
    "311312",
    "11132",
    "13112221133211322112211213322113",
    "312211322212221121123222113",
    "111312211312113221133211322112211213322113",
    "1321132122211322212221121123222113",
    "3113112211322112211213322113",
    "111312212221121123222113",
    "132112211213322113",
    "31121123222113",
    "111213322113",
    "123222113",
    "3113322113",
    "1113222113",
    "1322113",
    "311311222113",
    "1113122113",
    "132113",
    "3113",
    "1113",
    "13",
    "3",
    "1311222113321132211221121332211",
    "31221132221222112112322211",
};
static_assert(std::size(atomic_decay) == NUM_ATOMS);

static void step(std::vector<int> &new_counts, std::span<int> counts)
{
    std::fill(begin(new_counts), end(new_counts), 0);

    for (size_t i = 0; i < counts.size(); i++) {
        if (counts[i] > 0) {
            auto products = atomic_decay[i];
            for (; products; products >>= 8)
                new_counts[products & 0xff] += counts[i];
        }
    }
}

static size_t length_of(std::span<int> counts)
{
    size_t length = 0;

    for (size_t i = 0; i < counts.size(); i++) {
        length += counts[i] * atomic_sequences[i].size();
    }

    return length;
}

void run_2015_10(FILE *f)
{
    std::string s;
    getline(f, s);

    std::vector<int> counts(NUM_ATOMS), new_counts(NUM_ATOMS);
    for (size_t i = 1; i < std::size(atomic_sequences); i++) {
        if (s == atomic_sequences[i]) {
            counts[i]++;
            break;
        }
    }

    int i = 0;

    for (; i < 40; i++) {
        step(new_counts, counts);
        std::swap(new_counts, counts);
    }
    fmt::print("{}\n", length_of(counts));

    for (; i < 50; i++) {
        step(new_counts, counts);
        std::swap(new_counts, counts);
    }
    fmt::print("{}\n", length_of(counts));
}
