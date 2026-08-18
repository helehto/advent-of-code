#include "md5.h"

namespace md5 {

/// Table of MD5 hash functions specialized for different numbers of non-empty
/// blocks. Note that block 14 is always included since it contains the lower
/// 32 bits of the message length. (We assume that the upper 32 bits are always
/// zero.)
///
/// This is placed in the .cc file since 14 explicit instantiations bloat the
/// compile time quite a lot.
HashBlockFunc *const partial_hash_funcs[15] = {
    nullptr,
    hash_block<0b0100'0000'0000'0001, ResultType::only_a>,
    hash_block<0b0100'0000'0000'0011, ResultType::only_a>,
    hash_block<0b0100'0000'0000'0111, ResultType::only_a>,
    hash_block<0b0100'0000'0000'1111, ResultType::only_a>,
    hash_block<0b0100'0000'0001'1111, ResultType::only_a>,
    hash_block<0b0100'0000'0011'1111, ResultType::only_a>,
    hash_block<0b0100'0000'0111'1111, ResultType::only_a>,
    hash_block<0b0100'0000'1111'1111, ResultType::only_a>,
    hash_block<0b0100'0001'1111'1111, ResultType::only_a>,
    hash_block<0b0100'0011'1111'1111, ResultType::only_a>,
    hash_block<0b0100'0111'1111'1111, ResultType::only_a>,
    hash_block<0b0100'1111'1111'1111, ResultType::only_a>,
    hash_block<0b0101'1111'1111'1111, ResultType::only_a>,
    hash_block<0b0111'1111'1111'1111, ResultType::only_a>,
};

} // namespace md5
