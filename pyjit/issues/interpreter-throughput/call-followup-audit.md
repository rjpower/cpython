# Call path and frame-lifetime follow-up audit

Static audit, 2026-11-06. Source: runtime `7780e32e0e2`;
retained release ELF `final-identities/candidate-python`, build ID
`8c621301ec1ca453bca4c2405f64a80fa187e571`. No runtime execution,
benchmark or new build was performed for this audit.

## Retained GCC path

The follow-up's prologue observation is correct for this ELF. The symbol
`call_transaction_push_python` starts at `0x361340`. Before testing the
transaction kind at `0x361370`, GCC saves five callee-saved registers, subtracts
`0xf8` from `rsp` at `0x361350`, loads the stack guard at `0x361361`, and
stores it at `0x36136a`. The shared return path checks that guard at `0x36160c`.
The overlap helper has been inlined into this function; declined guards branch
to the general-provider path at `0x361570`, which addresses stack storage at
`rbp-0xc0`. The source has `PyValue small[16]` in that provider. This supports
the connection between a single machine function and its slow-path stack
requirements, but the disassembly alone does not prove that this array is the
only cause of the prologue, that the path is frequent, or that its removal
improves elapsed time.

## Smallest next experiment

Outline the existing consuming fallback into one private `Py_NO_INLINE` helper
called only after the overlap routine returns the existing decline status.
Keep the current overlap prepare/guard/commit in the wrapper and move the slow
body, including its argument array, without changing its conditions, status
contract, resources or ordering. This does not add a call on the successful
common path at source level. The proposed machine effect is to move the local
array and its stack-protector requirement off that path; GCC may still allocate
a frame, spill registers or introduce another call, so this must be checked in
the resulting release disassembly. Declined calls gain a call boundary and must
also be measured. A source helper split is not a proven speedup.

Before implementation, establish dynamic coverage of accepted/declined overlap
on the relevant workloads with existing statistics if possible. Review the
actual boundary for the original `-1/0/1` outcomes: no fallback after
ownership detaches; the pinned code, prepared kwargs, parent refresh and error
cleanup remain on their current paths; slow-path argument copies, releases,
child publication and cleanup keep their order. This changes no IR, operand
format, frame layout, or public provider. First run a release compile and
focused Python call/frame, errors, reentry, tracing, generator and fallback
tests; inspect both success and fallback GCC paths. Then run a matched basic
screen under the existing load and identity rules, plus a fresh cold review.
Admit only on correctness and measured workload evidence; retain neither the
split nor extra instrumentation merely because assembly is shorter.

## Design-page reconciliation

The introduction in `design/call-and-frame-lifetimes.md` correctly records the
A and B landings, but some later instructions still describe them as future
work:

- The call section says a common positional path "should" implement the
  prepare/guard/commit protocol and says instrumented calls "may initially"
  retain the old provider. At this source the path exists, and instrumented,
  free-threaded, keyword/EX and generator-owned callers decline overlap.
  The code uses an already-created executable and sufficient stack space;
  missing executable or stack space goes through the existing provider.
- The initialization section calls common-call default/variadic filling
  "proposed", says the initialization helper "may" accept the prefix, and
  says debug assertions "should" check it. Those are implemented in A/B;
  assertions cover the skipped range in debug builds. Replace future-tense
  implementation instructions with present-tense contracts while preserving
  the occupancy, observer, error and ownership proof obligations.
- The request to keep initialization independent from generic binding is a
  useful measurement history, not a current architectural requirement: the
  same overlap primitive is now used by exact and common calls. The two
  landings were measured separately; future changes should preserve that
  attribution rather than imply the paths are disjoint.

The page's broader distinctions remain valid: a future-read set is not a
validity/ownership map; ordinary thread cleanup already clears locals only;
and observers, generators, exception edges and JIT recovery need valid homes.

These documentation findings were reconciled in the current call/frame design
on 2026-11-07; the list above records what the static audit found.

## Offline annotation of the final logging record

Added 2026-11-07 after the static audit above. This is one diagnostic
five-second `cycles:u` record, not a workload-wide causal attribution or a
speedup estimate. Its retained candidate ELF has build ID
`8c621301ec1ca453bca4c2405f64a80fa187e571`, matching the earlier disassembly.
The existing whole-record self report assigns `call_transaction_push_python`
16.18% of weighted period. Offline `perf annotate` resolves 437 samples in
that function; its periods below are percentages of the function's annotated
period, not shares of runtime or instruction latencies. Sampling skid and the
small number of samples prevent treating a marked instruction as a measured
operation cost.

| Annotated address range | Source-level region | Local weighted period |
| --- | --- | ---: |
| `0x361340–0x36156f` | entry and initial overlap guards | 15.98% |
| `0x36160c–0x361630` | shared epilogue and stack-canary check | 7.83% |
| `0x361f2c–0x362123` | post-preparation function/code/default/stack revalidation | 11.47% |
| `0x3621dc–0x362316` | overlapping push and frame-header initialization | 23.50% |
| `0x362317–0x36232e` | newly exposed slot NULL loop | 10.94% |
| `0x36233f–0x3623d6` | default filling and child publication | 9.17% |
| `0x36250f–0x36255f` | empty-varargs singleton conversion and store | 8.00% |

These are selected disjoint address ranges; they do not classify every sample
or measure branch counts. The hot addresses include frame-header loads/stores
and the remaining NULL-tail loop, not just the prologue. The canary-check
instruction itself carries about 6.0% of annotated function period; the whole
shared epilogue is the 7.83% row. The empty-tuple range includes type/tag and
immortality handling around the statically known empty tuple. These observations
prioritize examining generated work but do not show that any specific check or
store caused a stall. The candidate deltablue record has only 13 annotated
samples in this function, too few for a comparable breakdown; the
comprehensions annotation reported no samples for the symbol. Avoid using
unresolved or nearest-symbol addresses to fill those gaps.

The prior cold-provider split remains a bounded candidate, now with narrower
justification: it may remove some entry/exit stack-protector work, while the
largest selected sampled region remains frame setup. A second small candidate
is the empty-varargs path: assess an existing canonical statically immortal
`PyValue` construction for the empty tuple, with explicit proof that its
identity, immortal tag, ownership and free-threaded behavior are correct. Use
one reviewed value API rather than an ad hoc bit pattern at the call site.
Inspect resulting GCC assembly and the fallback path, run focused varargs,
refcount, frame/reentry and generator tests, and compare matched workloads.
The annotated period is a prioritization signal, not an expected improvement.
Changes to the NULL-tail loop need a separate occupancy proof and must be
weighed against Integration B's existing mixed timings; the samples do not
justify changing it by themselves.

The annotation receipts are in `final-v4/perf-record/call-annotation/` under
the established results root. They retain argv, return codes, stdout, stderr,
and `DEBUGINFOD_URLS=''`. An initial request combined mutually exclusive perf
options and failed with exit 129; the corrected local-period annotations
succeeded and both attempts are preserved. The zero-status comprehensions
command still emitted the no-samples diagnostic on stderr.
