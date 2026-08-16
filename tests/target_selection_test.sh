#!/usr/bin/env bash

set -euo pipefail


if [[ $# -ne 1 ]]; then
    echo "usage: $0 <forge-binary>" >&2
    exit 2
fi


forge_binary="$1"


forge_binary="$(
    realpath "${forge_binary}"
)"


test_directory="$(
    mktemp -d
)"


cleanup()
{
    rm -rf "${test_directory}"
}


trap cleanup EXIT


cd "${test_directory}"


printf 'alpha\n' > alpha.in
printf 'beta\n' > beta.in


cat > build.forge <<'EOF'
rule copy
 command = cp $in $out

build alpha.mid: copy alpha.in
build alpha.out: copy alpha.mid

build beta.mid: copy beta.in
build beta.out: copy beta.mid
EOF


#
# Case 1:
# 只要求 alpha.out。
#
# beta 分支虽然 Output 全部不存在，
# 但不属于 alpha.out 的依赖闭包，
# 因此绝不能执行。
#

"${forge_binary}" \
    build.forge \
    alpha.out \
    > alpha_first.log \
    2>&1


grep -Fq \
    "planned edge count: 2" \
    alpha_first.log


test -f alpha.mid
test -f alpha.out


test ! -e beta.mid
test ! -e beta.out


#
# Case 2:
# alpha 已经完全 Clean。
#
# beta 依然整个分支都是 Dirty，
# 但再次指定 alpha.out 时必须得到 0 Edge。
#

"${forge_binary}" \
    build.forge \
    alpha.out \
    > alpha_clean.log \
    2>&1


grep -Fq \
    "planned edge count: 0" \
    alpha_clean.log


grep -Fq \
    "nothing to build" \
    alpha_clean.log


test ! -e beta.mid
test ! -e beta.out


#
# Case 3:
# 同时指定两个 Target。
#
# alpha 已经 Clean，beta 仍然 Dirty，
# 所以最终只执行 beta 的两个 Edge。
#

"${forge_binary}" \
    build.forge \
    alpha.out \
    beta.out \
    > multiple_targets.log \
    2>&1


grep -Fq \
    "planned edge count: 2" \
    multiple_targets.log


test -f alpha.out
test -f beta.mid
test -f beta.out


#
# Case 4:
# 未知 Target 必须明确失败，
# 不能静默退化成全量构建。
#

if "${forge_binary}" \
    build.forge \
    missing.target \
    > unknown_target.log \
    2>&1
then
    echo \
        "unknown target unexpectedly succeeded" \
        >&2

    exit 1
fi


grep -Fq \
    "unknown target: missing.target" \
    unknown_target.log


echo "target selection checks passed"