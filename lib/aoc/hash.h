#pragma once

#include <aoc/bitmanip.h>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

class CrcHasher {
private:
    template <std::integral T>
    static void update_crc(const std::byte *&p, uint64_t &crc) noexcept
    {
        T u;
        std::memcpy(&u, p, sizeof(T));
        crc = crc32_u64(crc, u);
        p += sizeof(T);
    }

    static size_t hash_bytes(const std::byte *p, size_t n) noexcept
    {
        uint64_t result = 0;

        for (; n >= 8; n -= 8)
            update_crc<uint64_t>(p, result);

        if (n) {
            alignas(8) std::array<std::byte, 8> tail{};
            for (size_t j = 0; n--; ++j)
                tail[j] = *p++;
            result = crc32_u64(result, std::bit_cast<uint64_t>(tail));
        }

        return result;
    }

    template <size_t N>
    static size_t hash_bytes_fixed(const std::byte *p) noexcept
    {
        uint64_t result = 0;
        for (size_t i = 0; i < N / 8; ++i)
            update_crc<uint64_t>(p, result);

        constexpr size_t rest = N % 8;
        if constexpr (rest & 4)
            update_crc<uint32_t>(p, result);
        if constexpr (rest & 2)
            update_crc<uint16_t>(p, result);
        if constexpr (rest & 1)
            update_crc<uint8_t>(p, result);

        return result;
    }

public:
    static size_t operator()(std::integral auto value) noexcept
    {
        static_assert(sizeof(value) <= 8);
        return crc32_u64(0, static_cast<uint64_t>(value));
    }

    static size_t operator()(const float value) noexcept
    {
        return crc32_u64(0, std::bit_cast<uint32_t>(value));
    }

    static size_t operator()(const double value) noexcept
    {
        return crc32_u64(0, std::bit_cast<uint64_t>(value));
    }

    template <typename T, size_t Extent>
    static size_t operator()(std::span<T, Extent> s) noexcept
    {
        static_assert(std::has_unique_object_representations_v<T>,
                      "Cannot hash type: it has non-unique object representations "
                      "(possibly padding?)");

        auto bytes = std::as_bytes(s);
        if constexpr (decltype(bytes)::extent == std::dynamic_extent) {
            return hash_bytes(bytes.data(), bytes.size());
        } else {
            return hash_bytes_fixed<decltype(bytes)::extent>(bytes.data());
        }
    }

    template <std::convertible_to<std::string_view> T>
    static size_t operator()(T &&s) noexcept
    {
        std::string_view sv = s;
        return hash_bytes(std::bit_cast<std::byte *>(sv.data()), sv.size());
    }

    template <typename T>
    static size_t operator()(const T &value) noexcept
        requires(!std::integral<T>)
    {
        static_assert(std::has_unique_object_representations_v<T>,
                      "Cannot hash type: it has non-unique object representations "
                      "(possibly padding?)");

        return hash_bytes_fixed<sizeof(T)>(std::bit_cast<std::byte *>(&value));
    }
};
