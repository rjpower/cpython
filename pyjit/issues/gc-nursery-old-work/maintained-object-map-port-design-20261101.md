# Object-map inventory on the maintained collector

2026-11-01, 05:39 UTC. **Design only; proposed next source attempt.** The span
candidate remains rejected on actual elapsed results; this proposal neither
changes nor retimes it. The old c43 source review is useful evidence, not
acceptance of an unimplemented port. No runtime patch or native command ran.

**Select the port.** Start from clean `417a9443a599e879c76c34c13ae76943bd6f6b3e`
(`build/worktrees/vm-compiler-backend`), whose non-`boil` tree is identical to
qualified runtime `e9e10d5b503eaf219797a5db44766eeab6fad4dc`. Port only c43's
`live_map_area`, page callback, object-route/block-visit split and cold
keep-everything cleanup. Those four production hunks pass `git apply --check`
without changing the maintained tree. Keep its classifier byte-for-byte:
c43's classifier differs only by 47cc's unrelated inventory fault injection.
The mimalloc heap/segment implementations are identical. The 17 added
`obmalloc.c` lines retire 47cc diagnostic-ledger entries; none belongs here.
Do not import nursery adoption, cadence, age/ledger changes, span allocation,
new registries or collector options. The old non-mimalloc shim, module insertion
and NurseryTests placement need explicit baseline anchors, not a cherry-pick.

**Work removed and retained.** Visit the same protected domain/thread/abandoned
OBJECT and GC pages through the existing allocator's page API. Enumerate only
committed initialized-start bitmap words; validate debug offset, canonical
stride, bounds and overflow before headers. A huge singleton tests one start,
not its payload. MEM discovery retains its separate page route. This removes
mimalloc's repeated free-list/free-map reconstruction and block-visitor layer;
it preserves page visiting, map loads, per-object classification, full-major
scheduling, old scanning, roots, death records, lifecycle and individual free.
It is an inventory optimization, not integrated allocator ownership. The old
12.1377% inventory/classification umbrella is not removable time; its explicit
allocator leaf was only 1.1473% of sampled coroutine CPU. Neither is a current
gain forecast. Inspect emitted calls before claiming callback removal.

**Close the missing failure proof.** Add one debug-only cold core test boundary,
not 47cc's general fault selector. It performs a forced manual major with
automatic GC still disabled and returns scalar evidence/error status. The
extension calls the exported boundary; it must not reference hidden
`_PyGC_Collect` directly. A one-shot context nominates only normal production
death inventory. After the first complete page that actually clears a MARKED
object through the unchanged classifier, abort that real walk. Independent
allocator **true-block** callbacks count MARKED and SHALLOW immediately before
and after the existing cold clear, under the same head lock and domain; they
check publication/header extent and never call the map iterator or allocate.
Require a positive cleared prefix, positive remaining MARKED/SHALLOW, both post
counts zero, a consumed trigger and actual rejected inventory. An incomplete
counter is unavailable, never zero; diagnostic trouble still runs cleanup.
The outer failure must discard the partial death array and dispatch no deaths.
Then require one fault-free major and intact payload, followed by fixed
reclamation checks. Do not fabricate flags or substitute local oracle faults.

Use a fixed bounded opaque extension-stack array of 4,096 initialized empty
lists, normally untracked before the call, to exercise the existing conservative
SHALLOW/readiness contract across multiple pages. No managed container may
precisely upgrade those references. Preserve the array through collection,
then restore ordinary tracking and release its owners on every error prefix.
A missing remaining-mark/SHALLOW witness fails closed; no growth or retries.
Exact root classification, forced-collection errors and this fixture receive
critical source review. No causal ablation image is required to interpret the
direct positive-before/zero-after cleanup observation.

**Shortest executable sequence, requiring separate grants.** Allow at most
30 minutes for the focused port/test source, then 10 minutes critical review.
Reanchor c43's four address-oracle methods/six children in a standalone baseline
class: ordinary publication/raw/free equality, its three local negative faults,
huge singleton and exited-thread ownership. Keep missing keeper/abandoned-page
witnesses as failures. Add the one debug cleanup case. In one fresh debug/O3
feedback batch (45-minute inclusive ceiling), run these and the existing five
`MinimalNurseryTests` plus focused frame/lifecycle/OOM controls. Those existing
minor tests retain their explicit experimental enable/restore; ordinary new
tests and benchmarks remain OFF/N0 with young admission disabled. Reuse the
current build/run/gate/cleanup executor and preserve first failures.

If correctness and native inspection pass, predeclare one unchanged four-row,
three-arm pilot (candidate/qualified parent/upstream; coroutines, generators,
comprehensions, Richards), two rotated rounds of five samples plus 12 origin
warmups. Require coroutine candidate/parent at most 0.99 and four-row GM at most
1.0 in **each** round; inconsistent/borderline results earn no extra samples.
Report raw vectors, spread, RSS and controls; explain any control loss over 2%.
Use the existing fixed protocol as a separately frozen successor, not edit the
span pilot. A promising result alone advances to the fixed all-29 parity
decision. Every grant clamps to **15:14:44 UTC**; old 04:18 authority is expired.

Read-only applicability evidence and the initially overbroad allocator-equality
assumption are retained in `build/object-map-port-design-20261101/receipt.json`.
