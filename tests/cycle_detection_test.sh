#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
    echo "usage: cycle_detection_test.sh <forge>"
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


cat > cycle.forge <<'MANIFEST'
rule copy
 command = cp $in $out

build a.txt: copy b.txt
build b.txt: copy a.txt
MANIFEST


set +e

"$FORGE" \
    cycle.forge \
    > cycle.log 2>&1

STATUS=$?

set -e


cat cycle.log


if [[ $STATUS -eq 0 ]]; then
    echo "cyclic build graph must be rejected"
    exit 1
fi


if ! grep -Fq \
    "cycle.forge: invalid build graph: cycle dependency detected" \
    cycle.log
then
    echo "expected cycle-detection error"
    exit 1
fi


echo "cycle detection checks passed"
