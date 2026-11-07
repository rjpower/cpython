# Integration B: independent cold source review

2026-11-06. **Accept runtime `ba37cc10058`; no blocking correctness finding.**
Reviewed against parent `8a8fdc91368` in
`python-boil-the-ocean/build/worktrees/pyvalue-rc-universal`. The first pass
read the actual uncommitted diff and adjacent source before this packet's
proposals or prior reviews. A later `git show` confirmed the committed change
was the same four-file increment. Runtime source remained untouched.

## Bounds and ownership

The shared overlap primitive is the right place for this change. Both the
common positional transaction (`Python/ceval.c:3115`) and the specialized exact
call producer (`Python/bytecodes.c:4317`, including its generated bound-method
and plain-function handlers) use it. Non-overlap and generator-owned caller
initialization retain their existing behavior.

At `Include/internal/pycore_interpframe.h:538`, let `B` be the child base,
`P` the previous frame's logical end, and `N` the child slot count. The new
boundary is `min(P - B, N)`. Arguments occupy the initial `nargs` cells;
assertions check that they fit both the previous extent and that boundary.
The debug loop checks every skipped non-argument home for NULL. Initialization
then zeros precisely the remaining child tail, retaining the ordinary constant
fill. An entirely overlapping child performs no tail stores; a larger child
initializes the newly exposed cells before publication.

Using the previous frame's logical extent is cleaner and safer than using
`datastack_top` as evidence of valid storage. A short frame can itself overlap
a larger suspended ancestor, leaving the stack high-water mark above the short
frame's own homes. That frontier describes reservation, not this caller's
NULL-home guarantee. The patch computes `P` before changing either frontier,
preserves `saved_top`, and extends `datastack_top` only when the child end
requires it. Stack-space guards remain conservative enough for the header and
the entire child slot count.

The skipped range follows held-owner occupancy, not merely future-read
liveness. `Python/regcfg.c:789` documents the ordering; `compact_op:887`
retains occupied homes until consumption and allocates later held temporaries
above earlier ones. Call windows are consecutive and above other caller-held
owners. The verifier's live-across-call check at `1441` reinforces that
contract but does not replace the occupancy proof. Caller locals and hidden
constants precede the temporary window. Thus the skipped range cannot contain
another caller owner that the former zeroing happened to hide.

The common call path completes preparation and revalidation before transferring
the callable and argument owners. No escaping operation intervenes before the
child becomes current. Defaults, empty varargs and prepared kwargs are installed
after valid child storage exists. Specialized exact calls preserve the same
transfer boundary. The revised overlap comment correctly distinguishes the
argument prefix from the NULL suffix and names the physical aliases as child
homes after transfer.

Constant filling still uses owning duplicates (`pycore_interpframe.h:259`),
including mortal replacements in hidden constant slots. Retirement clears
locals after asserting clear temporaries (`Python/frame.c:185`); exception
unwinding clears temporaries separately. Retained frames and generator creation
move every home and NULL its source (`_PyFrame_Copy`, `RETURN_GENERATOR`).
GC's full-slot visitor continues to receive valid values or NULL; stack-backed
frame objects do not independently traverse the overlapping array
(`Objects/frameobject.c:2378`). No root, copy or retirement consumer needs a
new representation for this increment.

The slow provider assertion at `Python/ceval.c:3265` now checks executable
identity only for successful status zero. Status one with `child == NULL` is
the intended adapter-decline state, so the prior NULL dereference is removed
without weakening the successful-child assertion.

## Tests, evidence and follow-up

The added call test warms small and large caller/callee combinations, reserves
extra caller temporaries through a wide untaken branch, and checks unwind and
mortal argument lifetime. The constant test selects an existing hidden `7`,
replaces it with a mortal object whose `__rsub__` runs Python, and checks both
results and stable reference count across repeated overlapping calls.

These are behavior regressions, not measurements of actual skipped words or
proof that every relative extent and specialization occurred. The variadic
targets chiefly exercise the common transaction route; generated exact and
bound-method paths were inspected statically. This is not a source blocker:
both routes share the reviewed primitive. Future targeted coverage should
establish actual extent relationships before claiming exhaustive boundary
coverage. Do not infer adapter reselection inside allocation from an ordinary
GC callback test.

The coordinator reports three successful builds with 117 modules each,
1,013 focused tests per release/debug profile (12/11 skips), and value API lint
covering 306 units with zero failing findings. These are coordinator-executed
results, not independent execution by this reviewer. This review executed only
read-only source and Git inspection, including a clean diff check. It ran no
runtime, test, generator, build, disassembly or performance command.

The `null_from` interface now expresses the caller's initialization boundary;
for overlap entry, it includes the proven valid prefix beyond arguments. This
is a clear, bounded abstraction improvement. Keep the logical-frame bound and
shared primitive; no extra validity bitmap or general liveness export follows
from this change. GCC inspection and matched timing still decide whether the
saved stores repay the new arithmetic. No performance gain or broad-suite
qualification is established here. Dependent Integration C may proceed on
correctness grounds after the coordinator's scheduled B screening.
