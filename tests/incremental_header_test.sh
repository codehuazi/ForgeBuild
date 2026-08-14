#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 2 ]]; then
    echo "usage: incremental_header_test.sh <forge> <source-dir>"
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
# 第一次编译已经生成 depfile，并由 ForgeBuild
# 将动态头文件依赖写入 DepsLog。
#
# 等待 1 秒后更新 a.hpp，确保它的 mtime
# 明确晚于第一次构建生成的 a.o 和 main.o。
#
sleep 1

touch a.hpp


echo "=== incremental header build ==="


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
# a.hpp 是 depfile 中记录的动态依赖：
#
#   a.hpp -> a.o
#       |
#       +----> main.o
#
# 两个目标变 Dirty 后继续传播到最终 link：
#
#   a.o -----+
#            |
#   b.o -----+--> app
#            |
#   main.o --+
#
# 因此最终 BuildPlan 应只有：
#
#   Compile a.cpp
#   Compile main.cpp
#   Link app
#
if ! grep -Fq \
    "planned edge count: 3" \
    incremental.log
then
    echo "expected incremental header build to contain 3 planned edges"
    exit 1
fi


#
# a.o 必须因为动态头文件 a.hpp 更新而重编。
#
if ! grep -Fq \
    "[dirty] a.cpp --compile--> a.o" \
    incremental.log
then
    echo "expected a.o compile edge to be dirty"
    exit 1
fi


if ! grep -Fq \
    "dynamic dependency newer than output: a.hpp -> a.o" \
    incremental.log
then
    echo "expected a.hpp dynamic dependency reason for a.o"
    exit 1
fi


#
# main.cpp 同样 include a.hpp，因此 main.o 也必须重编。
#
if ! grep -Fq \
    "[dirty] main.cpp --compile--> main.o" \
    incremental.log
then
    echo "expected main.o compile edge to be dirty"
    exit 1
fi


if ! grep -Fq \
    "dynamic dependency newer than output: a.hpp -> main.o" \
    incremental.log
then
    echo "expected a.hpp dynamic dependency reason for main.o"
    exit 1
fi


#
# b.cpp / b.o 与 a.hpp 无关，不应该进入 BuildPlan。
#
if grep -Fq \
    "[dirty] b.cpp --compile--> b.o" \
    incremental.log
then
    echo "b.cpp should not be rebuilt"
    exit 1
fi


#
# a.o 或 main.o 变化后，最终 link 必须重新执行。
#
if ! grep -Fq \
    "[dirty] a.o + b.o + main.o --link--> app" \
    incremental.log
then
    echo "expected app link edge to be dirty"
    exit 1
fi


#
# Link Dirty 必须来自上游目标传播。
#
if ! grep -Fq \
    "upstream output is dirty:" \
    incremental.log
then
    echo "expected dirty propagation to app"
    exit 1
fi


APP_OUTPUT="$(./app)"


if [[ "$APP_OUTPUT" != "30" ]]; then
    echo "unexpected incremental app output: $APP_OUTPUT"
    exit 1
fi


echo "incremental header checks passed"