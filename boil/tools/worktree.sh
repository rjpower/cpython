#!/bin/sh
# Step worktrees and branches of boil/PROCESS.md section 2.
#
# usage: boil/tools/worktree.sh new STEP [BASE]   create .worktrees/boil-STEP on branch boil/STEP
#        boil/tools/worktree.sh build STEP        configure and build build/default and build/debug there
#        boil/tools/worktree.sh push STEP         push branch boil/STEP to origin
#        boil/tools/worktree.sh land STEP         merge boil/STEP into the integration branch (no fast-forward) and push
#        boil/tools/worktree.sh drop STEP         remove the worktree and the branch
set -eu
cmd=$1; step=$2
MAIN=/home/power/code/cpython/.worktrees/python-boil-the-ocean
INTEGRATION=weaver/python-boil-the-ocean
WT=/home/power/code/cpython/.worktrees/boil-$step
BR=boil/$step
PUSH="git -c credential.helper= -c credential.helper=!gh auth git-credential push https://github.com/rjpower/cpython.git"
case $cmd in
new)
    base=${3:-$INTEGRATION}
    git -C "$MAIN" worktree add -b "$BR" "$WT" "$base"
    echo "$WT on $BR from $base" ;;
build)
    J=${GATE_JOBS:-16}
    for v in default debug; do
        mkdir -p "$WT/build/$v"
        case $v in default) flags="" ;; debug) flags="--with-pydebug" ;; esac
        (cd "$WT/build/$v" && ../../configure -q CC=gcc $flags >/dev/null && make -s -j"$J")
    done
    echo "built $WT/build/default and $WT/build/debug" ;;
push)
    (cd "$WT" && $PUSH "$BR") ;;
land)
    cd "$MAIN"
    git merge --no-ff "$BR" -m "boil: land $step" || { echo "merge conflict; resolve in $MAIN, commit, then push" >&2; exit 1; }
    $PUSH "$INTEGRATION" ;;
drop)
    git -C "$MAIN" worktree remove --force "$WT"
    git -C "$MAIN" branch -D "$BR"
    git -C "$MAIN" worktree prune ;;
*)  echo "unknown command $cmd" >&2; exit 2 ;;
esac
