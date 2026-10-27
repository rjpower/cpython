#!/bin/sh
# Peer review of a design document by codex (gpt-6.1-sol, xhigh reasoning),
# read-only over the repository.  Writes <dir>/reviews/<name>-review.md and
# prints its path.
#
# usage: boil/tools/review.sh DESIGN.md [CONTEXT.md ...]
set -eu
REPO=$(cd "$(dirname "$0")/../.." && pwd)
cd "$REPO"
design=$1; shift
name=$(basename "$design" .md)
outdir=$(dirname "$design")/reviews
mkdir -p "$outdir"
out="$outdir/$name-review.md"
ctx="boil/design/06-architecture-v2.md (sections 1 to 6 are the architecture and plan), boil/PROCESS.md"
for f in "$@"; do ctx="$ctx, $f"; done
prompt="You are a peer reviewer on a CPython performance fork (register IR, NaN-boxed PyValue, tracing collector, hidden classes). Review the design document $design. Read it in full, then the context: $ctx, and any source file the document cites. Review for: (1) correctness: holes, unstated invariants, interactions with the collector (conservative C-stack roots, no reference counting), the value encoding and the C API; (2) design clarity: one mechanism per job, no dual forms, no interim hack without a named retirement step; (3) implementation risk: what will go wrong, in which file; (4) tests and measurements: what is missing to prove the change; (5) what the document leaves undecided. Be concrete: cite sections and files, propose the fix where you can, and do not pad. Output markdown with these headings: Summary (three lines), Blocking issues (numbered; each with the problem, where, and the fix), Non-blocking issues, Questions, Verdict (one of: ready, revise)."
codex exec -m gpt-6.1-sol -c model_reasoning_effort=xhigh --sandbox read-only -C "$REPO" -o "$out" "$prompt" >/dev/null 2>&1 || { echo "review failed for $design" >&2; exit 1; }
echo "$out"
