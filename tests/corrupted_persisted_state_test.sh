#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 2 ]]; then
    echo "usage: corrupted_persisted_state_test.sh <forge> <source-dir>"
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


"$FORGE" \
    -j 3 \
    build.forge \
    > baseline.log 2>&1


if ! grep -Fq \
    "planned edge count: 4" \
    baseline.log
then
    cat baseline.log
    echo "expected baseline build to contain 4 edges"
    exit 1
fi


if [[ "$(head -n 1 .forge_log)" \
    != "FORGEBUILD_BUILD_LOG_V1" ]]
then
    echo "unexpected build log header"
    exit 1
fi


if [[ "$(head -n 1 .forge_deps)" \
    != "FORGEBUILD_DEPS_LOG_V1" ]]
then
    echo "unexpected deps log header"
    exit 1
fi


echo "=== clean build ==="


"$FORGE" \
    -j 3 \
    build.forge \
    > clean.log 2>&1


if ! grep -Fq \
    "planned edge count: 0" \
    clean.log
then
    cat clean.log
    echo "expected clean build before corruption"
    exit 1
fi


#
# 保留合法 V1 Header，但故意破坏 DepsLog 正文。
#
cat > .forge_deps <<'EOF'
FORGEBUILD_DEPS_LOG_V1
a.o
not-a-number
EOF


echo "=== corrupted deps log recovery ==="


"$FORGE" \
    --explain \
    -j 3 \
    build.forge \
    > corrupted.log 2>&1


cat corrupted.log


if ! grep -Fq \
    "build_log=ok, deps_log=corrupted" \
    corrupted.log
then
    echo "expected corrupted deps log recovery message"
    exit 1
fi


if ! grep -Fq \
    "planned edge count: 4" \
    corrupted.log
then
    echo "corrupted persisted state must trigger conservative rebuild"
    exit 1
fi


if [[ "$(head -n 1 .forge_deps)" \
    != "FORGEBUILD_DEPS_LOG_V1" ]]
then
    echo "deps log was not repaired after recovery build"
    exit 1
fi


echo "=== clean build after recovery ==="


"$FORGE" \
    -j 3 \
    build.forge \
    > clean_after_recovery.log 2>&1


if ! grep -Fq \
    "planned edge count: 0" \
    clean_after_recovery.log
then
    cat clean_after_recovery.log
    echo "expected clean build after state recovery"
    exit 1
fi


#
# 把 BuildLog 改成一个未来版本。
#
{
    echo "FORGEBUILD_BUILD_LOG_V999"
    tail -n +2 .forge_log
} > .forge_log.unsupported


mv \
    .forge_log.unsupported \
    .forge_log


echo "=== unsupported build-log version recovery ==="


"$FORGE" \
    --explain \
    -j 3 \
    build.forge \
    > unsupported.log 2>&1


cat unsupported.log


if ! grep -Fq \
    "build_log=unsupported-version, deps_log=ok" \
    unsupported.log
then
    echo "expected unsupported build-log recovery message"
    exit 1
fi


if ! grep -Fq \
    "planned edge count: 4" \
    unsupported.log
then
    echo "unsupported persisted state must trigger conservative rebuild"
    exit 1
fi


echo "corrupted persisted state checks passed"