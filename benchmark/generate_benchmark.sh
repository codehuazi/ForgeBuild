#!/usr/bin/env bash

set -euo pipefail


ROOT="benchmark"

SRC_DIR="${ROOT}/src"
INCLUDE_DIR="${ROOT}/include"
OUT_DIR="${ROOT}/out"
MANIFEST="${ROOT}/build.forge"


mkdir -p \
    "${SRC_DIR}" \
    "${INCLUDE_DIR}" \
    "${OUT_DIR}"


cat > "${INCLUDE_DIR}/common.hpp" <<'EOF'
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
EOF


for i in $(seq -w 0 30)
do
    number=$((10#$i))


    cat > "${SRC_DIR}/file_${i}.cpp" <<EOF
#include "common.hpp"

#include <cstdint>


std::uint64_t bench_value_${i}()
{
    constexpr auto values =
        forge_bench::make_table<1024>(
            ${number} + 1
        );


    return forge_bench::fold(
        values
    );
}
EOF
done


cat > "${SRC_DIR}/main.cpp" <<'EOF'
#include <cstdint>
#include <iostream>


EOF


for i in $(seq -w 0 30)
do
    echo \
        "std::uint64_t bench_value_${i}();" \
        >> "${SRC_DIR}/main.cpp"
done


cat >> "${SRC_DIR}/main.cpp" <<'EOF'


int main()
{
    std::uint64_t result = 0;


EOF


for i in $(seq -w 0 30)
do
    echo \
        "    result ^= bench_value_${i}();" \
        >> "${SRC_DIR}/main.cpp"
done


cat >> "${SRC_DIR}/main.cpp" <<'EOF'


    std::cout
        << result
        << '\n';


    return 0;
}
EOF


cat > "${MANIFEST}" <<'EOF'
rule compile
 command = g++ -std=c++20 -O2 -Ibenchmark/include -MMD -MF $out.d -c $in -o $out
 depfile = $out.d

rule link
 command = g++ $in -o $out

EOF


for i in $(seq -w 0 30)
do
    echo \
        "build benchmark/out/file_${i}.o: compile benchmark/src/file_${i}.cpp" \
        >> "${MANIFEST}"
done


echo \
    "build benchmark/out/main.o: compile benchmark/src/main.cpp" \
    >> "${MANIFEST}"


printf \
    "build benchmark/out/app: link" \
    >> "${MANIFEST}"


for i in $(seq -w 0 30)
do
    printf \
        " benchmark/out/file_%s.o" \
        "${i}" \
        >> "${MANIFEST}"
done


printf \
    " benchmark/out/main.o\n" \
    >> "${MANIFEST}"


echo "benchmark generated:"
echo "  compile edges: 32"
echo "  link edges:    1"
echo "  total edges:   33"
