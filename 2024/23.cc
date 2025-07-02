#include "common.h"

namespace aoc_2024_23 {

/// Fixed-size bitset for representing a subset of all vertices.
struct VertexSet {
    // Since vertices are represented as two lowercase letters, this needs to
    // fit 26*26 = 676 bits, but rounding it up to 32*32 = 1024 bits leads to
    // nicer autovectorized code (being exactly two 512-bit vectors).
    alignas(64) std::array<uint64_t, (32 * 32) / 64> qwords{};

    /// Insert `v` into this set.
    constexpr void insert(size_t v) noexcept
    {
        qwords[v / 64] |= UINT64_C(1) << (v % 64);
    }

    /// Remove `v` from this set.
    constexpr void erase(size_t v) noexcept
    {
        qwords[v / 64] &= ~(UINT64_C(1) << (v % 64));
    }

    /// Return whether this set contains `v`.
    constexpr bool contains(size_t v) const noexcept
    {
        return qwords[v / 64] & (UINT64_C(1) << (v % 64));
    }

    /// Compute the intersection of this set with `other`.
    constexpr VertexSet operator&(const VertexSet &other) noexcept
    {
        VertexSet result;
        for (size_t i = 0; i < qwords.size(); ++i)
            result.qwords[i] = qwords[i] & other.qwords[i];
        return result;
    }

    /// Return the number of elements in this set.
    constexpr size_t count() const noexcept
    {
        return std::ranges::fold_left(qwords, 0, λab(a + std::popcount(b)));
    }

    /// Return true iff both a and b are empty.
    constexpr static bool union_is_empty(const VertexSet &a, const VertexSet &b)
    {
        uint64_t r = 0;

        // Jointly checking two sets compiles to a fast sequence with AVX-512:
        // vpternlogq+vporq for the union, followed by vptestnmd+kortestw to
        // test whether any bits are set.
        for (size_t i = 0; i < a.qwords.size(); ++i)
            r |= a.qwords[i] | b.qwords[i];

        return r == 0;
    }
};

struct BronKerbosch {
    const std::array<VertexSet, 32 * 32> &n;
    VertexSet clique;
    size_t clique_size = 0;
    VertexSet r;
    size_t rbits = 0;

    constexpr void run(VertexSet p, VertexSet x) noexcept
    {
        // If we have neither any potential nor excluded vertices left, we've
        // found a clique:
        if (VertexSet::union_is_empty(p, x)) {
            if (rbits > clique_size) {
                clique = r;
                clique_size = rbits;
            }
            return;
        }

        size_t pbits = p.count();

        for (size_t i = 0; i < p.qwords.size(); ++i) {
            for (uint64_t m = p.qwords[i]; m; m &= m - 1) {
                size_t v = 8 * sizeof(m) * i + std::countr_zero(m);

                // Recursive search with `v` added to the current candidate
                // clique:
                r.insert(v);
                rbits++;
                run(p & n[v], x & n[v]);
                r.erase(v);
                rbits--;

                // Move this vertex from the possible set to the excluded set.
                p.erase(v);
                pbits--;
                x.insert(v);

                // Since we are looking for the maximum clique, backtrack if we
                // can never exceed the current maximum clique in any recursive
                // call from here.
                if (rbits + pbits <= clique_size)
                    return;
            }
        }
    }
};

/// Find the maximum clique using the Bron-Kerbosch algorithm.
constexpr VertexSet bron_kerbosch(VertexSet nodes,
                                  const std::array<VertexSet, 32 * 32> &edges)
{
    BronKerbosch bk(edges);
    bk.run(nodes, {});
    return bk.clique;
}

void run(std::string_view buf)
{
    VertexSet nodes;
    std::array<VertexSet, 32 * 32> edges;

    std::vector<std::string_view> tokens;
    for (std::string_view line : split_lines(buf)) {
        split(line, tokens, '-');
        ASSERT(tokens[0].size() == 2);
        ASSERT(tokens[1].size() == 2);
        size_t a = (tokens[0][0] - 'a') * 32 + tokens[0][1] - 'a';
        size_t b = (tokens[1][0] - 'a') * 32 + tokens[1][1] - 'a';
        nodes.insert(a);
        nodes.insert(b);
        edges[a].insert(b);
        edges[b].insert(a);
    }

    // Part 1:
    {
        int triples = 0;

        // Pick the first vertex from the range 'ta'..'tz' to make all triples
        // that we inspect have a node beginning with 't' by construction.
        for (size_t a = ('t' - 'a') * 32; a <= ('t' - 'a') * 32 + 26; ++a) {
            if (!nodes.contains(a))
                continue;

            // Pick the second vertex from the neighbors of `a`.
            for (size_t i = 0; i < edges[a].qwords.size(); ++i) {
                for (uint64_t mb = edges[a].qwords[i]; mb; mb &= mb - 1) {
                    size_t b = 8 * sizeof(mb) * i + std::countr_zero(mb);
                    // Avoid duplicates: cull triples for which the subsequence
                    // of nodes starting with 't' is not in order.
                    if (b / 32 == 't' - 'a' && a > b)
                        continue;

                    // Finally, to form a 3-clique, pick the third vertex such
                    // that it has an edge to the first two -- this is the
                    // intersection of the neighbor sets:
                    const VertexSet cs = edges[a] & edges[b];

                    for (size_t j = b / 64; j < cs.qwords.size(); ++j) {
                        for (uint64_t mc = cs.qwords[j]; mc; mc &= mc - 1) {
                            size_t c = 8 * sizeof(mc) * j + std::countr_zero(mc);

                            // Avoid duplicates: cull triples for which the
                            // subsequence of nodes starting with 't' is not in
                            // order.
                            if (c / 32 == 't' - 'a' && a > c)
                                continue;

                            // Avoid duplicates: cull triples for which the
                            // last two elements are not in order.
                            if (b > c)
                                continue;

                            triples++;
                        }
                    }
                }
            }
        }
        fmt::print("{}\n", triples);
    }

    // Part 2:
    {
        VertexSet maximum_clique = bron_kerbosch(nodes, edges);

        std::string result;
        for (size_t i = 0; i < maximum_clique.qwords.size(); ++i) {
            for (uint64_t m = maximum_clique.qwords[i]; m; m &= m - 1) {
                size_t v = 8 * sizeof(m) * i + std::countr_zero(m);
                result += static_cast<char>('a' + v / 32);
                result += static_cast<char>('a' + v % 32);
                result += ',';
            }
        }
        fmt::print("{}\n", std::string_view(result).substr(0, result.size() - 1));
    }
}

}
