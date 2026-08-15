#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
    echo "usage: parser_error_test.sh <forge>"
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


#
# Case 1：
# 未知语句必须报出准确行号。
#
cat > typo.forge <<'EOF'
rule compile
 command = true
commnad = false
EOF


set +e

"$FORGE" \
    typo.forge \
    > typo.log 2>&1

STATUS=$?

set -e


if [[ $STATUS -eq 0 ]]; then
    cat typo.log
    echo "unknown statement should fail"
    exit 1
fi


if ! grep -Fq \
    "typo.forge:3: unknown statement: commnad = false" \
    typo.log
then
    cat typo.log
    echo "expected line-aware unknown-statement error"
    exit 1
fi


#
# Case 2：
# build 语句结束后，不能再修改上一个 Rule。
#
cat > invalid_state.forge <<'EOF'
rule compile
 command = true
build out.txt: compile in.txt
command = false
EOF


set +e

"$FORGE" \
    invalid_state.forge \
    > invalid_state.log 2>&1

STATUS=$?

set -e


if [[ $STATUS -eq 0 ]]; then
    cat invalid_state.log
    echo "invalid parser state should fail"
    exit 1
fi


if ! grep -Fq \
    "invalid_state.forge:4: command must follow a rule" \
    invalid_state.log
then
    cat invalid_state.log
    echo "expected parser-state error"
    exit 1
fi


#
# Case 3：
# build 使用不存在的 Rule。
#
cat > unknown_rule.forge <<'EOF'
build out.txt: missing input.txt
EOF


set +e

"$FORGE" \
    unknown_rule.forge \
    > unknown_rule.log 2>&1

STATUS=$?

set -e


if [[ $STATUS -eq 0 ]]; then
    cat unknown_rule.log
    echo "unknown rule should fail"
    exit 1
fi


if ! grep -Fq \
    "unknown_rule.forge:1: unknown rule: missing" \
    unknown_rule.log
then
    cat unknown_rule.log
    echo "expected unknown-rule error"
    exit 1
fi


echo "parser error checks passed"