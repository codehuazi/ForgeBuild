#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 2 ]]; then
    echo "usage: incremental_missing_input_test.sh <forge> <source-dir>"
    exit 1
fi


FORGE="$1"
SOURCE_DIR="$2"

TEST_DIR="$(mktemp -d)"


cleanup()
{
    rm -rf "$TEST_DIR"
}


trap cleanup EXIT


cp \
    "$SOURCE_DIR/a.cpp" \
    "$SOURCE_DIR/a.hpp" \
    "$SOURCE_DIR/b.cpp" \
    "$SOURCE_DIR/b.hpp" \
    "$SOURCE_DIR/main.cpp" \
    "$SOURCE_DIR/build.forge" \
    "$TEST_DIR/"


cd "$TEST_DIR"


echo "=== baseline build ==="

if ! "$FORGE" \
    -j 3 \
    build.forge \
    > baseline.log 2>&1
then
    cat baseline.log
    exit 1
fi


if ! grep -Fq \
    "planned edge count: 4" \
    baseline.log
then
    cat baseline.log
    echo "expected baseline build to contain 4 planned edges"
    exit 1
fi


if [[ "$(./app)" != "30" ]]; then
    echo "unexpected baseline app output"
    exit 1
fi


rm a.cpp


echo "=== missing explicit input build ==="

set +e

"$FORGE" \
    --explain \
    -j 3 \
    build.forge \
    > missing_input.log 2>&1

STATUS=$?

set -e


cat missing_input.log


if [[ $STATUS -eq 0 ]]; then
    echo "build should fail when explicit input is missing"
    exit 1
fi


if ! grep -Fq \
    "planned edge count: 2" \
    missing_input.log
then
    echo "expected compile edge and downstream link edge in build plan"
    exit 1
fi


if ! grep -Fq \
    "[dirty] a.cpp --compile--> a.o" \
    missing_input.log
then
    echo "expected a.cpp compile edge to be dirty"
    exit 1
fi


if ! grep -Fq \
    "input missing: a.cpp" \
    missing_input.log
then
    echo "expected explicit missing-input dirty reason"
    exit 1
fi


if ! grep -Fq \
    "upstream output is dirty: a.o" \
    missing_input.log
then
    echo "expected dirty propagation from a.o to app"
    exit 1
fi


if ! grep -Fq \
    "build failed" \
    missing_input.log
then
    echo "expected overall build failure"
    exit 1
fi


echo "incremental missing-input checks passed"