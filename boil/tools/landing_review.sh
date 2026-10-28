#!/bin/sh
# The landing review of PROCESS.md section 1 step 4: a reviewer reads what
# was built (the as-built record, the gate lines and the diff of the rung),
# not the design.  Runs codex read-only over a checkout of the rung's head;
# falls back to a cold claude session like review.sh.  Writes
# <workdoc dir>/reviews/<name>-landing.md in the checkout and prints the path.
#
# usage: boil/tools/landing_review.sh CHECKOUT BASE HEAD WORKDOC.md
#   CHECKOUT  a worktree at HEAD (the reviewer reads files there)
#   BASE      the integration head the rung is measured against
#   WORKDOC   the rung's work doc, path relative to the checkout
# Environment: REVIEW_LIMIT (seconds for codex, default 2400), REVIEWER=claude.
set -eu
checkout=$1; base=$2; head=$3; doc=$4
cd "$checkout"
name=$(basename "$doc" .md)
outdir=$(dirname "$doc")/reviews
mkdir -p "$outdir"
out="$outdir/$name-landing.md"
prompt="You are the landing reviewer on a CPython performance fork (register IR, NaN-boxed PyValue, tracing collector with precise and conservative roots, hidden classes). A rung of work is about to merge. Review what was built, not the design. Read in full: $doc (its design and its as-built record), boil/PROCESS.md (section 1 step 4 says what you are for), and the diff of the rung: run 'git diff --stat $base $head' then 'git diff $base $head' and read every hunk that touches C, the handler definitions (Python/bytecodes.c), the generators under Tools/cases_generator, and the tests. Context: boil/design/06-architecture-v2.md sections 1 to 6 and the design note the work doc cites. Check: (1) every deviation from the design is named in the as-built record with its reason; (2) every gate line names the commit it was produced at and no code commit follows that commit without a rerun or a stated reason the line stands; (3) every hazard found was traced to a cause with evidence, not dismissed; (4) invariants the design relies on are kept by the code (roots across collecting calls, no PyObject* fields holding inline values, shape and version rules, no reference counting); (5) the tests assert what the design promised, including negative cases; (6) measurements are recorded against the before records with attribution of anything the rung did not design; (7) no checker-appeasing form or interim hack without a named owner and retirement. Be concrete: cite files and lines of the diff and sections of the record, and say how to fix each finding. Output markdown with these headings: Summary (three lines), Blocking findings (numbered; each with the problem, where, and the fix), Non-blocking findings, Questions for the coordinator, Verdict (one of: land, fix first)."
reviewer=codex
if [ "${REVIEWER:-codex}" = codex ]; then
    timeout -k 30 "${REVIEW_LIMIT:-2400}" codex exec -m gpt-6.1-sol -c model_reasoning_effort=xhigh \
        --sandbox read-only -C "$checkout" -o "$out" "$prompt" >/dev/null 2>&1 \
        && [ -s "$out" ] || { echo "codex landing review did not finish; falling back to claude" >&2; reviewer=claude; }
fi
if [ "$reviewer" = claude ]; then
    rm -f "$out"
    claude -p --model opus --effort xhigh \
        --allowedTools 'Read,Grep,Glob,Bash(rg *),Bash(git log *),Bash(git show *),Bash(git diff *)' \
        "$prompt" > "$out" 2>/dev/null && [ -s "$out" ] || { echo "landing review failed for $doc" >&2; exit 1; }
fi
printf '\n---\nLanding reviewer: %s, %s; base %s, head %s\n' "$reviewer" "$(date '+%Y-%m-%d %H:%M %Z')" "$base" "$head" >> "$out"
echo "$out"
