#include "bitmanip.h"
#include <random>

#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#warnings"
#include <doctest/doctest.h>
#pragma clang diagnostic pop

static std::minstd_rand rng(1234);

// Ensure that the wrappers arounds x86-64 instructions are usable as constant
// expressions.
static_assert(bextr_u32(0, 0, 0) == 0);
static_assert(bextr_u64(0, 0, 0) == 0);
static_assert(bzhi_u32(0, 0) == 0);
static_assert(bzhi_u64(0, 0) == 0);
static_assert(pdep_u32(0, 0) == 0);
static_assert(pdep_u64(0, 0) == 0);
static_assert(pext_u32(0, 0) == 0);
static_assert(pext_u64(0, 0) == 0);
static_assert(crc32_u8(0, 0) == 0);
static_assert(crc32_u16(0, 0) == 0);
static_assert(crc32_u32(0, 0) == 0);
static_assert(crc32_u64(0, 0) == 0);

#ifdef __BMI2__

TEST_CASE("bextr_fallback<uint32_t> matches _bextr_u32")
{
    SUBCASE("for index and count zero")
    {
        CHECK(bextr_fallback<uint32_t>(0x12345678, 0, 0) == _bextr_u32(0x12345678, 0, 0));
    }

    SUBCASE("for index equal to the bit width")
    {
        CHECK(bextr_fallback<uint32_t>(0x12345678, 32, 0) ==
              _bextr_u32(0x12345678, 32, 0));
    }

    SUBCASE("for count equal to the bit width")
    {
        CHECK(bextr_fallback<uint32_t>(0x12345678, 0, 32) ==
              _bextr_u32(0x12345678, 0, 32));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> value_dist;
        std::uniform_int_distribution<unsigned int> index_dist(0, 32);
        std::uniform_int_distribution<unsigned int> count_dist(0, 32);

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t value = value_dist(rng);
            unsigned int index = index_dist(rng);
            unsigned int count = count_dist(rng);
            CHECK(bextr_fallback<uint32_t>(value, index, count) ==
                  _bextr_u32(value, index, count));
        }
    }
}

TEST_CASE("bextr_fallback<uint64_t> matches _bextr_u64")
{
    SUBCASE("for index and count zero")
    {
        CHECK(bextr_fallback<uint64_t>(0x123456789abcdef0, 0, 0) ==
              _bextr_u64(0x123456789abcdef0, 0, 0));
    }

    SUBCASE("for index equal to the bit width")
    {
        CHECK(bextr_fallback<uint64_t>(0x123456789abcdef0, 64, 0) ==
              _bextr_u64(0x123456789abcdef0, 64, 0));
    }

    SUBCASE("for count equal to the bit width")
    {
        CHECK(bextr_fallback<uint64_t>(0x123456789abcdef0, 0, 64) ==
              _bextr_u64(0x123456789abcdef0, 0, 64));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint64_t> value_dist;
        std::uniform_int_distribution<unsigned int> index_dist(0, 64);
        std::uniform_int_distribution<unsigned int> count_dist(0, 64);

        for (size_t i = 0; i < 10'000; ++i) {
            uint64_t value = value_dist(rng);
            unsigned int index = index_dist(rng);
            unsigned int count = count_dist(rng);
            CHECK(bextr_fallback<uint64_t>(value, index, count) ==
                  _bextr_u64(value, index, count));
        }
    }
}

TEST_CASE("bzhi_fallback<uint32_t> matches _bzhi_u32")
{
    SUBCASE("for index zero")
    {
        CHECK(bzhi_fallback<uint32_t>(0x12345678, 0) == _bzhi_u32(0x12345678, 0));
    }

    SUBCASE("for index equal to the bit width")
    {
        CHECK(bzhi_fallback<uint32_t>(0x12345678, 32) == _bzhi_u32(0x12345678, 32));
    }

    SUBCASE("for index greater than the bit width")
    {
        CHECK(bzhi_fallback<uint32_t>(0x12345678, 40) == _bzhi_u32(0x12345678, 40));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> value_dist;
        std::uniform_int_distribution<unsigned int> index_dist(0, 32);

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t value = value_dist(rng);
            unsigned int index = index_dist(rng);
            CHECK(bzhi_fallback<uint32_t>(value, index) == _bzhi_u32(value, index));
        }
    }
}

TEST_CASE("bzhi_fallback<uint64_t> matches _bzhi_u64")
{
    SUBCASE("for index zero")
    {
        CHECK(bzhi_fallback<uint64_t>(0x123456789abcdef0, 0) ==
              _bzhi_u64(0x123456789abcdef0, 0));
    }

    SUBCASE("for index equal to the bit width")
    {
        CHECK(bzhi_fallback<uint64_t>(0x123456789abcdef0, 64) ==
              _bzhi_u64(0x123456789abcdef0, 64));
    }

    SUBCASE("for index greater than the bit width")
    {
        CHECK(bzhi_fallback<uint64_t>(0x123456789abcdef0, 72) ==
              _bzhi_u64(0x123456789abcdef0, 72));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint64_t> value_dist;
        std::uniform_int_distribution<unsigned int> index_dist(0, 64);

        for (size_t i = 0; i < 10'000; ++i) {
            uint64_t value = value_dist(rng);
            unsigned int index = index_dist(rng);
            CHECK(bzhi_fallback<uint64_t>(value, index) == _bzhi_u64(value, index));
        }
    }
}

TEST_CASE("pdep_fallback<uint32_t> matches _pdep_u32")
{
    SUBCASE("for zero mask")
    {
        CHECK(pdep_fallback<uint32_t>(0x00000000, 0) == _pdep_u32(0x00000000, 0));
        CHECK(pdep_fallback<uint32_t>(0x12345678, 0) == _pdep_u32(0x12345678, 0));
        CHECK(pdep_fallback<uint32_t>(0xffffffff, 0) == _pdep_u32(0xffffffff, 0));
    }

    SUBCASE("for full mask")
    {
        CHECK(pdep_fallback<uint32_t>(0x00000000, 0xffffffff) ==
              _pdep_u32(0x00000000, 0xffffffff));
        CHECK(pdep_fallback<uint32_t>(0x12345678, 0xffffffff) ==
              _pdep_u32(0x12345678, 0xffffffff));
        CHECK(pdep_fallback<uint32_t>(0xffffffff, 0xffffffff) ==
              _pdep_u32(0xffffffff, 0xffffffff));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t value = dist(rng);
            uint32_t mask = dist(rng);
            CHECK(pdep_fallback<uint32_t>(value, mask) == _pdep_u32(value, mask));
        }
    }
}

TEST_CASE("pdep_fallback<uint64_t> matches _pdep_u64")
{
    SUBCASE("for zero mask")
    {
        CHECK(pdep_fallback<uint64_t>(0x0000000000000000, 0) ==
              _pdep_u64(0x0000000000000000, 0));
        CHECK(pdep_fallback<uint64_t>(0x123456789abcdef0, 0) ==
              _pdep_u64(0x123456789abcdef0, 0));
        CHECK(pdep_fallback<uint64_t>(0xffffffffffffffff, 0) ==
              _pdep_u64(0xffffffffffffffff, 0));
    }

    SUBCASE("for full mask")
    {
        CHECK(pdep_fallback<uint64_t>(0x0000000000000000, 0xffffffffffffffff) ==
              _pdep_u64(0x0000000000000000, 0xffffffffffffffff));
        CHECK(pdep_fallback<uint64_t>(0x123456789abcdef0, 0xffffffffffffffff) ==
              _pdep_u64(0x123456789abcdef0, 0xffffffffffffffff));
        CHECK(pdep_fallback<uint64_t>(0xffffffffffffffff, 0xffffffffffffffff) ==
              _pdep_u64(0xffffffffffffffff, 0xffffffffffffffff));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint64_t> dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint64_t value = dist(rng);
            uint64_t mask = dist(rng);
            CHECK(pdep_fallback<uint64_t>(value, mask) == _pdep_u64(value, mask));
        }
    }
}

TEST_CASE("pext_fallback<uint32_t> matches _pext_u32")
{
    SUBCASE("for zero mask")
    {
        CHECK(pext_fallback<uint32_t>(0x00000000, 0) == _pext_u32(0x00000000, 0));
        CHECK(pext_fallback<uint32_t>(0x12345678, 0) == _pext_u32(0x12345678, 0));
        CHECK(pext_fallback<uint32_t>(0xffffffff, 0) == _pext_u32(0xffffffff, 0));
    }

    SUBCASE("for full mask")
    {
        CHECK(pext_fallback<uint32_t>(0x00000000, 0xffffffff) ==
              _pext_u32(0x00000000, 0xffffffff));
        CHECK(pext_fallback<uint32_t>(0x12345678, 0xffffffff) ==
              _pext_u32(0x12345678, 0xffffffff));
        CHECK(pext_fallback<uint32_t>(0xffffffff, 0xffffffff) ==
              _pext_u32(0xffffffff, 0xffffffff));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t value = dist(rng);
            uint32_t mask = dist(rng);
            CHECK(pext_fallback<uint32_t>(value, mask) == _pext_u32(value, mask));
        }
    }
}

TEST_CASE("pext_fallback<uint64_t> matches _pext_u64")
{
    SUBCASE("for zero mask")
    {
        CHECK(pext_fallback<uint64_t>(0x0000000000000000, 0) ==
              _pext_u64(0x0000000000000000, 0));
        CHECK(pext_fallback<uint64_t>(0x123456789abcdef0, 0) ==
              _pext_u64(0x123456789abcdef0, 0));
        CHECK(pext_fallback<uint64_t>(0xffffffffffffffff, 0) ==
              _pext_u64(0xffffffffffffffff, 0));
    }

    SUBCASE("for full mask")
    {
        CHECK(pext_fallback<uint64_t>(0x0000000000000000, 0xffffffffffffffff) ==
              _pext_u64(0x0000000000000000, 0xffffffffffffffff));
        CHECK(pext_fallback<uint64_t>(0x123456789abcdef0, 0xffffffffffffffff) ==
              _pext_u64(0x123456789abcdef0, 0xffffffffffffffff));
        CHECK(pext_fallback<uint64_t>(0xffffffffffffffff, 0xffffffffffffffff) ==
              _pext_u64(0xffffffffffffffff, 0xffffffffffffffff));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint64_t> dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint64_t value = dist(rng);
            uint64_t mask = dist(rng);
            CHECK(pext_fallback<uint64_t>(value, mask) == _pext_u64(value, mask));
        }
    }
}

#endif // __BMI2__

#ifdef __SSE4_2__

TEST_CASE("crc32_fallback<uint8_t> matches _mm_crc32_u8")
{
    SUBCASE("for zero crc")
    {
        CHECK(_mm_crc32_u8(0, 0) == crc32_fallback<uint8_t>(0, 0));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> crc_dist;
        std::uniform_int_distribution<uint8_t> value_dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t crc = crc_dist(rng);
            uint8_t value = value_dist(rng);
            CHECK(crc32_fallback<uint8_t>(crc, value) == _mm_crc32_u8(crc, value));
        }
    }
}

TEST_CASE("crc32_fallback<uint16_t> matches _mm_crc32_u16")
{
    SUBCASE("for zero crc")
    {
        CHECK(_mm_crc32_u16(0, 0) == crc32_fallback<uint16_t>(0, 0));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> crc_dist;
        std::uniform_int_distribution<uint16_t> value_dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t crc = crc_dist(rng);
            uint16_t value = value_dist(rng);
            CHECK(crc32_fallback<uint16_t>(crc, value) == _mm_crc32_u16(crc, value));
        }
    }
}

TEST_CASE("crc32_fallback<uint32_t> matches _mm_crc32_u32")
{
    SUBCASE("for zero crc")
    {
        CHECK(_mm_crc32_u32(0, 0) == crc32_fallback<uint32_t>(0, 0));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> crc_dist;
        std::uniform_int_distribution<uint32_t> value_dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t crc = crc_dist(rng);
            uint32_t value = value_dist(rng);
            CHECK(crc32_fallback<uint32_t>(crc, value) == _mm_crc32_u32(crc, value));
        }
    }
}

TEST_CASE("crc32_u64 matches _mm_crc32_u64")
{
    SUBCASE("for zero crc")
    {
        CHECK(_mm_crc32_u64(0, 0) == crc32_u64(0, 0));
    }

    SUBCASE("for random inputs")
    {
        std::uniform_int_distribution<uint32_t> crc_dist;
        std::uniform_int_distribution<uint64_t> value_dist;

        for (size_t i = 0; i < 10'000; ++i) {
            uint32_t crc = crc_dist(rng);
            uint64_t value = value_dist(rng);
            CHECK(crc32_u64(crc, value) == _mm_crc32_u64(crc, value));
        }
    }
}

#endif // __SSE4_2__
