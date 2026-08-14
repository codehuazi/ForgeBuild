#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 2 ]]; then
    echo "usage: incremental_source_test.sh <forge> <source-dir>"
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
# ForgeBuild 当前使用文件时间戳进行基础 Dirty 判断。
#
# 等待 1 秒再更新 a.cpp，保证 a.cpp 的 mtime
# 明确晚于第一次构建生成的 a.o。
#
sleep 1

touch a.cpp


echo "=== incremental source build ==="


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
# 修改 a.cpp 后：
#
#   a.cpp -> a.o
#              |
#              v
#             app
#
# 因此 BuildPlan 应只有：
#
#   1 Compile
#   1 Link
#
if ! grep -Fq \
    "planned edge count: 2" \
    incremental.log
then
    echo "expected incremental build to contain 2 planned edges"
    exit 1
fi


if ! grep -Fq \
    "[dirty] a.cpp --compile--> a.o" \
    incremental.log
then
    echo "expected a.o compile edge to be dirty"
    exit 1
fi


if ! grep -Fq \
    "input newer than output: a.cpp -> a.o" \
    incremental.log
then
    echo "expected a.cpp timestamp dirty reason"
    exit 1
fi


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
# 无关 Translation Unit 不应该重新进入 BuildPlan。
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
# a.cpp 已经是显式 Input。
# 它不应该再被 DepsLog 重复解释为动态依赖。
#
if grep -Fq \
    "dynamic dependency newer than output: a.cpp -> a.o" \
    incremental.log
then
    echo "explicit source should not be reported as a dynamic dependency"
    exit 1
fi


APP_OUTPUT="$(./app)"


if [[ "$APP_OUTPUT" != "30" ]]; then
    echo "unexpected incremental app output: $APP_OUTPUT"
    exit 1
fi


echo "incremental source checks passed"