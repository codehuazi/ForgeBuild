#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 2 ]]; then
    echo "usage: interrupted_state_test.sh <forge> <source-dir>"
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
    echo "expected baseline build to contain 4 planned edges"
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
    echo "expected clean build before recovery test"
    exit 1
fi


#
# 模拟上一次 ForgeBuild 在持久化状态正式提交前
# 异常退出留下的 marker。
#
printf '%s\n' \
    "simulated interrupted build" \
    > .forge_in_progress


echo "=== recovery build ==="


"$FORGE" \
    --explain \
    -j 3 \
    build.forge \
    > recovery.log 2>&1


cat recovery.log


if ! grep -Fq \
    "recovery: previous build state was not committed; ignoring persisted logs" \
    recovery.log
then
    echo "expected interrupted-build recovery message"
    exit 1
fi


#
# 两份历史日志都被视为不可信，
# BuildLog 为空会使所有已有 Output 的 command
# 无法匹配，因此进行一次保守全量重建。
#
if ! grep -Fq \
    "planned edge count: 4" \
    recovery.log
then
    echo "expected conservative full rebuild during recovery"
    exit 1
fi


#
# 两份日志成功提交后 marker 应作为最终 commit point
# 被移除。
#
if [[ -e .forge_in_progress ]]; then
    echo "build state marker should be removed after successful commit"
    exit 1
fi


echo "=== post-recovery clean build ==="


"$FORGE" \
    -j 3 \
    build.forge \
    > post_recovery.log 2>&1


if ! grep -Fq \
    "planned edge count: 0" \
    post_recovery.log
then
    cat post_recovery.log
    echo "expected clean build after recovery commit"
    exit 1
fi


APP_OUTPUT="$(./app)"


if [[ "$APP_OUTPUT" != "30" ]]; then
    echo "unexpected recovered app output: $APP_OUTPUT"
    exit 1
fi


echo "interrupted state recovery checks passed"