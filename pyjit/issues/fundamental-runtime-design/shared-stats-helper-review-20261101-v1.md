# Independent shared-stats helper review

2026-11-01. **ACCEPT exact host readiness for one separately activated bounded
GCC computed/debug/stats baseline.** Root alone may activate the native scope.
No target, compiler, census, hardware counter, workload or timing ran in this
review. No frozen helper was changed. Regex implementation remains paused;
the six consumers are a small shared observation set, not a native-interface
roadmap or performance qualification.

Frozen directory:
`build/worktrees/vm-compiler-backend/build/shared-attribution-gcc-stats-v1`.

| Input | SHA256 |
|---|---|
| preparation-capsule.json | `32afb06637d12b86fa15cc50df812b22fb118fd72e58ada885b485e64d6fa134` |
| plan.json | `974097c852c66d9e92a8d08da49864bc80ed82790b70339fb78239bf6c034a87` |
| launch-held.json | `346e4a0d590c362ded86141f5919f5e486780edaa1f6c9a1656b419cf5dc8298` |
| launch.py | `936e07025904153c53018953c874e3e328b2781ba37eee909a05b396f7163ce2` |
| run_census.py | `63753496dbbc3cfc88b04a4faca48b234bf3eb38669fc264d7348ca97561df6d` |
| census_target.py | `a463c5f36dbde829a1024a1acc43095a964445aaa945f5da4eaec859f4f2ec2e` |
| helper-diff.patch | `fbb6e145ce3d1fe0a5f863ce0c1ca8e955c548b87520fbdeaeb4edc7d9a3426a` |

## Exact scope and process exposure

The literal ten ordered rows are one fresh GCC build, 20 existing focused
methods, six current PyStats methods, one canonical stale row, then coroutines,
generators, regex_compile, json_loads, unpickle and Richards counter rows.
Row ceilings are 900/300/300/180 seconds and six times 240 seconds, subject to
the common remaining-time clamp. Their sum is not a promise that all can run at
their individual ceilings; the inclusive batch window is authoritative.

The build is `CC=gcc --with-computed-gotos --with-pydebug --enable-pystats
CFLAGS=-O1 -g`, with GIL/mimalloc, frame/leaf-frame pointers and zero automatic
variable initialization. No PGO, LTO, JIT, tail backend, sanitizer, second image
or elapsed metric is admitted. Config and linked precise-source/inline audit
stamps remain required. Compiler-selected tool discovery is deferred to the
activated build and must match the pinned selection; this review hashes files
without querying the compiler.

The 20 focus and six stats case IDs exist in the bound source, with no expected
skips. I independently rebuilt all ten original stats child strings from the
literal template/method source, compiled them with the host parser and matched
their exact hashes to the plan. Stats children retain their original flags and
typed script-helper return object. Their one raw handler-coverage dump is
copied before TemporaryDirectory cleanup and must witness coverage version 2,
computed mode 0, actual handler IDs/counts and incoming pairs.

The census has exactly 18 workload processes plus 12 existing version/stats
probe processes: each consumer invokes warmup/low/high plus two probes. Its
validator requires exactly five actual invocation records per consumer and
ties each of the last three original argv to its actual dump journal row.
There are also ten original stats-test children, the one stale child containing
six functions times 100, focused-test children and the test harness/build
processes. The 30-process census subtotal is **not** a total native-process
claim for the whole batch. Two focus call sites include the existing managed
call-root child and the staged finalizer-OOM children; their actual scheduled,
start/end and returned records remain required, without inventing a template
count as execution credit.

## Provenance and raw retention

Independent rehashing passed all 4,193 mixed source/helper inputs, 97 driver
inputs, all capsule artifact bindings and all 19 helper hashes/modes. The 4,174
runtime/build inputs match actual qualified e9 source bytes. The six explicit
PRIMARY Windows-script CRLF/LF differences are recorded rather than hidden or
changed. The managed worktree is clean at `417a9443a599e879c76c34c13ae76943bd6f6b3e`;
the new owned build directory is empty. Nine inherited helper bodies are
byte-identical; the six narrow inherited deltas were read alongside the new
launch/census/host-control code.

The exact primary driver `0a777191...` and its independent retention review are
pinned. Every census uses its original workload body, arguments, loop count and
probe count. A target-side prefix records actual executable/CAPI, debug/stats
configuration, OFF/no-extras/N0, hash-seed behavior, pre/post module origins
and mappings within those existing processes. It does not replace collecting
calls or add a kernel execution. Observer startup/shutdown contributes to the
raw software counts and is disclosed; this image has no elapsed credit.

Each consumer gets a private stats directory. Three raw UTF-8 dump texts and
original-byte hashes are journaled before parsing/deletion, and report metadata
binds journal hash/path/count. Independent replay reconstructs parsed counters
and the complete normalized result from the raw low/high union, including
negative site/pair/entry differences. Missing keys are not fabricated as
positive attribution. Failed/ambiguous originals and pre-wait stdout/stderr
files remain evidence. Actual image/module origins must resolve to source,
helper or frozen product hashes; returned PC text alone is not stable site
identity across processes.

## Authority, serialization and failure behavior

The accepted existing row supervisor, worker, state/module contracts, stale
oracle and gate bodies are reused rather than replaced by a new executor.
`supervisor.ROOT` is explicitly rebound to the literal row cwd, avoiding the
earlier declared/actual-directory hazard. Shared serialization, the machine's
two-slot gate and the exact build-image lock serialize all rows and post-gate
verification. Every row must terminate, show no live image PID under lock,
verify artifacts and remain inside the deadline before the next row starts.

The authoritative [current work breakdown](../../design/runtime-work-breakdown.md)
gives this attribution baseline a **new inclusive 45-minute activation-relative
window**. The old 04:18:11 nursery-adoption deadline is not inherited. The
launcher takes the earlier activated deadline and its own start-plus-45-minutes;
the unchanged row supervisor reserves 210 seconds and clamps all outer
ceilings downward. Positive timeout and verification bounds remain enforced.
First unexpected build/audit/source/child/census/verification failure stops the
batch, preserves failure/current cleanup receipts and gives no pass credit.
There is no retry, rebuild, replacement consumer or added process authority.

Readiness is false and activation absent. An activation must supply exact source,
helper and driver review hashes and the exact manifest. The reviewed helper
alone cannot grant itself native authority. Root's actual terminal confirmation
and a later independent outcome audit remain necessary.

## Review findings and host replay

No blocking finding remains. A preview concern about the observer's own module
origin was withdrawn after inspecting the mixed manifest correctly: all 19
helpers are absolute keys in `plan.inputs`, so `ROOT / absolute_key` retains
the pinned helper path. I independently rehashed that entry and replayed the
unchanged source-backed synthetic fixture in a reviewer-owned copy. Its pinned
`__main__` origin is accepted, while an unbound origin is rejected. No origin
filter or validation weakening was needed. The deadline query was likewise
resolved by the explicit current work breakdown, rather than importing stale
scope wording.

All seven frozen host control methods independently passed. They cover altered
source/configuration, held activation, mode/coverage/handler witnesses, raw and
normalized counter mutations, extra process records, stale roster/state/root
guards and partial stdout/stderr survival on mocked timeout/nonzero exit.
The control bodies were inspected first; possible target subprocess calls are
mocked. Tools were file-hashed, never run for discovery by this review.

Reviewer evidence is under `build/shared-attribution-review-20261101-v1`:
`host-controls.log`, `binding-check.json`, `source-roster.json`, and the unchanged
`origin_guard_followup.py` copy with its JSON/log. This is readiness evidence,
not observed native correctness or an allocation/death census. The unpinned
root count remains typed report-only, fallback is forbidden, and the proposed
cheap-death eligibility census is explicitly absent. Software counts still do
not establish boxing dominance, adjacent-word fusion hotness or a winning
interpreter/collector architecture.
