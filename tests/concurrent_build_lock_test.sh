#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
    echo "usage: concurrent_build_lock_test.sh <forge>"
    exit 1
fi


FORGE="$1"

TEST_DIR="$(mktemp -d)"

FIRST_PID=""


cleanup()
{
    if [[ -n "$FIRST_PID" ]] \
        && kill -0 "$FIRST_PID" 2>/dev/null
    then
        kill "$FIRST_PID" 2>/dev/null || true
        wait "$FIRST_PID" 2>/dev/null || true
    fi


    rm -rf "$TEST_DIR"
}


trap cleanup EXIT


cd "$TEST_DIR"


printf '%s\n' \
    "hello" \
    > input.txt


cat > build.forge <<'EOF'
rule slow_copy
 command = touch started.flag && sleep 2 && cp $in $out

build output.txt: slow_copy input.txt
EOF


echo "=== start first ForgeBuild ==="


"$FORGE" \
    -j 1 \
    build.forge \
    > first.log 2>&1 &


FIRST_PID=$!


#
# started.flag 由真正的构建命令产生。
# 看到它以后，可以确定第一个 Forge 已经进入执行阶段，
# 因而一定已经持有 .forge_lock。
#
for _ in $(seq 1 100)
do
    if [[ -f started.flag ]]; then
        break
    fi


    sleep 0.05
done


if [[ ! -f started.flag ]]; then
    cat first.log

    echo "first ForgeBuild did not start in time"

    exit 1
fi


echo "=== start second ForgeBuild ==="


set +e


"$FORGE" \
    build.forge \
    > second.log 2>&1


SECOND_STATUS=$?


set -e


cat second.log


if [[ $SECOND_STATUS -eq 0 ]]; then
    echo "second concurrent ForgeBuild should fail"

    exit 1
fi


if ! grep -Fq \
    "another ForgeBuild process is already using this build directory" \
    second.log
then
    echo "expected build-directory lock error"

    exit 1
fi


#
# 第二个 Forge 必须在碰持久化状态之前退出。
# 此时第一个 Forge 仍在 sleep，所以 marker 应继续存在。
#
if [[ ! -e .forge_in_progress ]]; then
    echo "active build marker was unexpectedly removed"

    exit 1
fi


echo "=== wait for first ForgeBuild ==="


if ! wait "$FIRST_PID"
then
    cat first.log

    echo "first ForgeBuild unexpectedly failed"

    exit 1
fi


FIRST_PID=""


if [[ "$(cat output.txt)" != "hello" ]]; then
    echo "unexpected output from first ForgeBuild"

    exit 1
fi


if [[ -e .forge_in_progress ]]; then
    echo "build marker should be removed after successful build"

    exit 1
fi


#
# .forge_lock 文件本身应该继续存在，
# 但此时内核锁已经因为 RAII 释放。
#
if [[ ! -e .forge_lock ]]; then
    echo "expected persistent .forge_lock file"

    exit 1
fi


echo "=== run ForgeBuild after lock release ==="


"$FORGE" \
    build.forge \
    > third.log 2>&1


cat third.log


if ! grep -Fq \
    "planned edge count: 0" \
    third.log
then
    echo "expected clean build after lock release"

    exit 1
fi


echo "concurrent build lock checks passed"