#include "common.hpp"

#include <cstdint>


std::uint64_t bench_value_29()
{
    constexpr auto values =
        forge_bench::make_table<1024>(
            29 + 1
        );


    return forge_bench::fold(
        values
    );
}
