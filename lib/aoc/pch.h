// Only include headers that are used in most source files here; dumping *all*
// headers here makes things end up slower. This list was initally produced by
// reading:
//
//     $ git grep -h '^#include' | sort | uniq -c | sort -rn | head -30
//
// followed by some hand-waving to remove a few headers.
//
// Each individual source file should still include every header it needs.
#include <algorithm>
#include <aoc/base.h>
#include <aoc/dense_map.h>
#include <aoc/dense_set.h>
#include <aoc/hash.h>
#include <aoc/inplace_vector.h>
#include <aoc/macros.h>
#include <aoc/math.h>
#include <aoc/small_vector.h>
#include <aoc/string.h>
#include <aoc/thread_pool.h>
#include <array>
#include <atomic>
#include <bit>
#include <charconv>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
