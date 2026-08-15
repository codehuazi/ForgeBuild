#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
    echo "usage: missing_output_test.sh <forge>"
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
    "input" \
    > input.txt


cat > build.forge <<'EOF'
rule generate
 command = true

build result.txt: generate input.txt
EOF


echo "=== missing declared output build ==="

set +e

"$FORGE" \
    --explain \
    build.forge \
    > missing_output.log 2>&1

STATUS=$?

set -e


cat missing_output.log


if [[ $STATUS -eq 0 ]]; then
    echo "build should fail when declared output is missing"
    exit 1
fi


if ! grep -Fq \
    "planned edge count: 1" \
    missing_output.log
then
    echo "expected one generate edge in build plan"
    exit 1
fi


if ! grep -Fq \
    "declared output missing after successful command: result.txt" \
    missing_output.log
then
    echo "expected missing-output validation error"
    exit 1
fi


if ! grep -Fq \
    "build failed" \
    missing_output.log
then
    echo "expected overall build failure"
    exit 1
fi


if [[ -e result.txt ]]; then
    echo "result.txt should not exist"
    exit 1
fi


if [[ -e .forge_log ]]; then
    echo "failed edge must not commit a successful build log"
    exit 1
fi


echo "missing-output checks passed"