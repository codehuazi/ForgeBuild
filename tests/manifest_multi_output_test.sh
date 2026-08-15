#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
    echo "usage: manifest_multi_output_test.sh <forge>"
    exit 1
fi


FORGE="$1"

TEST_DIR="$(mktemp -d)"


cleanup()
{
    rm -rf "$TEST_DIR"
}


trap cleanup EXIT


cd "$TEST_DIR"


printf '%s\n' \
    "seed" \
    > seed.txt


cat > build.forge <<'EOF'
rule generate
 command = printf 'cpp\n' > generated.cpp && printf 'hpp\n' > generated.hpp

rule consume
 command = cat $in > result.txt

build generated.cpp generated.hpp: generate seed.txt
build result.txt: consume generated.cpp generated.hpp
EOF


echo "=== first multi-output build ==="


"$FORGE" \
    --explain \
    -j 2 \
    build.forge \
    > first.log 2>&1


cat first.log


if ! grep -Fq \
    "planned edge count: 2" \
    first.log
then
    echo "expected two edges in first build"
    exit 1
fi


if [[ ! -f generated.cpp
    || ! -f generated.hpp
    || ! -f result.txt ]]
then
    echo "multi-output build did not generate all outputs"
    exit 1
fi


if [[ "$(cat result.txt)" \
    != $'cpp\nhpp' ]]
then
    echo "unexpected result.txt content"
    exit 1
fi


echo "=== clean multi-output build ==="


"$FORGE" \
    -j 2 \
    build.forge \
    > clean.log 2>&1


if ! grep -Fq \
    "planned edge count: 0" \
    clean.log
then
    cat clean.log
    echo "expected clean second build"
    exit 1
fi


rm generated.hpp


echo "=== missing one multi-output artifact ==="


"$FORGE" \
    --explain \
    -j 2 \
    build.forge \
    > rebuild.log 2>&1


cat rebuild.log


if ! grep -Fq \
    "planned edge count: 2" \
    rebuild.log
then
    echo "expected producer and consumer to rebuild"
    exit 1
fi


if ! grep -Fq \
    "output missing: generated.hpp" \
    rebuild.log
then
    echo "expected missing generated.hpp dirty reason"
    exit 1
fi


if ! grep -Fq \
    "upstream output is dirty: generated.hpp" \
    rebuild.log
then
    echo "expected dirty propagation through generated.hpp"
    exit 1
fi


if [[ ! -f generated.cpp
    || ! -f generated.hpp
    || ! -f result.txt ]]
then
    echo "multi-output rebuild did not restore all outputs"
    exit 1
fi


echo "manifest multi-output checks passed"