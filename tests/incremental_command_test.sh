#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 2 ]]; then
    echo "usage: incremental_command_test.sh <forge> <source-dir>"
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


cat baseline.log


if ! grep -Fq \
    "planned edge count: 4" \
    baseline.log
then
    echo "expected baseline build to contain 4 planned edges"
    exit 1
fi


APP_OUTPUT="$(./app)"


if [[ "$APP_OUTPUT" != "30" ]]; then
    echo "unexpected baseline app output: $APP_OUTPUT"
    exit 1
fi


#
# 先确认当前构建确实完全 clean。
#
# 这样后面重新构建时，可以明确证明 Dirty
# 是由 command change 引起，而不是残留的文件状态。
#
echo "=== clean build ==="


if ! "$FORGE" \
    --explain \
    -j 3 \
    build.forge \
    > clean.log 2>&1
then
    cat clean.log
    exit 1
fi


cat clean.log


if ! grep -Fq \
    "planned edge count: 0" \
    clean.log
then
    echo "expected build to be clean before command change"
    exit 1
fi


#
# 只修改 a.o 对应的编译命令：
#
#   原命令：
#     g++ -std=c++20 ...
#
#   新命令：
#     g++ -std=c++20 -O2 ...
#
# b.o 和 main.o 继续使用原 compile 规则。
#
# 因此只有 a.o 应因为 command hash 改变而直接 Dirty。
#
cat > build.forge <<'EOF'
rule compile
 command = g++ -std=c++20 -MMD -MF $out.d -c $in -o $out
 depfile = $out.d

rule compile_a_optimized
 command = g++ -std=c++20 -O2 -MMD -MF $out.d -c $in -o $out
 depfile = $out.d

rule link
 command = g++ $in -o $out

build a.o: compile_a_optimized a.cpp
build b.o: compile b.cpp
build main.o: compile main.cpp
build app: link a.o b.o main.o
EOF


echo "=== incremental command build ==="


if ! "$FORGE" \
    --explain \
    -j 3 \
    build.forge \
    > incremental.log 2>&1
then
    cat incremental.log
    exit 1
fi


cat incremental.log


#
# 只有：
#
#   a.cpp -> a.o
#   a.o + b.o + main.o -> app
#
# 应进入 BuildPlan。
#
if ! grep -Fq \
    "planned edge count: 2" \
    incremental.log
then
    echo "expected command change build to contain 2 planned edges"
    exit 1
fi


#
# a.o 必须重新编译。
#
if ! grep -Fq \
    "[dirty] a.cpp --compile_a_optimized--> a.o" \
    incremental.log
then
    echo "expected a.o compile edge to be dirty"
    exit 1
fi


#
# 最关键断言：
# Dirty 原因必须来自 BuildLog 中保存的 command hash。
#
if ! grep -Fq \
    "command changed for output: a.o" \
    incremental.log
then
    echo "expected command change reason for a.o"
    exit 1
fi


#
# b.cpp 和 main.cpp 的编译命令没有改变，
# 因此不应该重新编译。
#
if grep -Fq \
    "[dirty] b.cpp --compile--> b.o" \
    incremental.log
then
    echo "b.cpp should not be rebuilt"
    exit 1
fi


if grep -Fq \
    "[dirty] main.cpp --compile--> main.o" \
    incremental.log
then
    echo "main.cpp should not be rebuilt"
    exit 1
fi


#
# app 必须因为新的 a.o 而重新链接。
#
if ! grep -Fq \
    "[dirty] a.o + b.o + main.o --link--> app" \
    incremental.log
then
    echo "expected app link edge to be dirty"
    exit 1
fi


if ! grep -Fq \
    "upstream output is dirty: a.o" \
    incremental.log
then
    echo "expected dirty propagation from a.o to app"
    exit 1
fi


#
# 这个测试没有修改任何源文件或头文件，
# 所以 a.o 不应该因为时间戳或动态依赖变化而 Dirty。
#
if grep -Fq \
    "input newer than output: a.cpp -> a.o" \
    incremental.log
then
    echo "a.o should not be dirty because of source timestamp"
    exit 1
fi


if grep -Fq \
    "dynamic dependency newer than output:" \
    incremental.log
then
    echo "build should not be dirty because of dynamic dependencies"
    exit 1
fi


APP_OUTPUT="$(./app)"


if [[ "$APP_OUTPUT" != "30" ]]; then
    echo "unexpected command-change app output: $APP_OUTPUT"
    exit 1
fi


echo "incremental command checks passed"