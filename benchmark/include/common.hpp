#pragma once

#include <array>
#include <cstddef>
#include <cstdint>


namespace forge_bench
{


template<std::size_t N>
constexpr std::array<std::uint64_t, N> make_table(
    std::uint64_t seed
)
{
    std::array<std::uint64_t, N> values{};

    std::uint64_t value =
        seed;


    for(std::size_t i = 0;
        i < N;
        ++i)
    {
        value ^= value << 13;
        value ^= value >> 7;
        value ^= value << 17;

        values[i] =
            value
            + static_cast<std::uint64_t>(i);
    }


    return values;
}


template<std::size_t N>
constexpr std::uint64_t fold(
    const std::array<std::uint64_t, N>& values
)
{
    std::uint64_t result =
        14695981039346656037ULL;


    for(const auto value :
        values)
    {
        result ^= value;
        result *= 1099511628211ULL;
    }


    return result;
}


} // namespace forge_bench
// common header fan-out test
