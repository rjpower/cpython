# Independent merged source review

2026-11-09 UTC. Reviewed the integration checkout
`build/worktrees/execution-boundaries-20261108` after operand commit
`f2c5ac3052b` and source-only merge/regeneration of the boolean-boundary and
Python-discard increments. No native build or timing by this reviewer.

No source-merge blocker found. This is not native qualification or final
performance admission. Coordinator combined release/debug builds, focused
runtime cases, stats checks and generated-product checks remain required.

## Shared generator and handler domain

The merged descriptor has static local/temp fields, boolean boundary mask and
postcall discard property. Family applicability remains explicit. Window formats
are allowed for call refinements only when no unsupported operand-mode override
is present; real invalid operand modes still reject.

A host review driver on the actual merged source passed these checks:

- Exactly six execution variants occupy IDs 319–324.
- Every alias shares its base instruction format and complete conservative
  contract, has the canonical specialized shadow and family base, and receives
  SPECIALIZED/IR_ONLY metadata without PRISTINE or ADAPTIVE flags.
- Generated handlers use canonical opcode and stats names. The shape variant
  retains static ownership operations; all three bool variants omit dynamic
  NOP traversal and retain one initial PC save. Exact discard sets the recipient
  tag; general discard passes the constant flag to the guarded runner.
- Pinned number checks and `git diff HEAD --check` pass.

The integrated adaptive-disassembly shadow repair remains present. It displays
canonical specializations while physical get_ir retains variant identities;
fused and structural IR handlers keep their previous mapping rules.

## Specialization, feedback and instrumentation

The conflict resolution preserved all three independent selector branches:
shape mode, bool edge descriptor, and exact/general discarded result. The saved
feedback row and incoming comparison paths normalize all six aliases; operand
normalization was not lost while adding bool/call cases. Existing fused handler
identities are unchanged. Fixed opcode arrays still receive canonical opcodes.

Boolean skip selection remains disabled for debug and free-threaded builds;
pristine/live post-NOP successors must advertise the generated SAVES_IP fact.
Instrumentation invalidates the three dependent skip handlers before target
patches. Discard selection is free-threaded-disabled and excludes generator
caller/callee code, non-temporary destinations and mismatched/lineless POPs.
Its live return guard preserves instrumented POP fallback. The two mechanisms
do not silently share their distinct no-callback/possible-finalizer PC proofs.

## General-child code identity at commit

The code object passed to `call_python_recipient` in the consuming fallback is
trusted actual child code, not a stale unowned lookup. The fallback independently
pins PyFunction_GET_CODE, passes that explicit code to
`eval_frame_push_and_init_code` or its EX adapter, and the provider initializes
the child with that same code. `initialize_locals` also takes it explicitly;
callbacks cannot make the provider reselect mutable function.__code__. The EX
adapter forwards the same argument. Post-binding executable fixup likewise uses
that code, still pinned until cleanup after commit.

Thus sync-code flags at recipient commit describe the child's actual executable.
Overlap entry separately retains post-preparation identity/resource checks and
its no-callback commit interval. No new code reload or call guard is needed.
Native completion keeps an ordinary numeric recipient; only committed Python
children receive the tag. Exact entry retains its version/shape/resource guards
and the existing no-callback gap before recipient publication/frame switch.

## Review limits

The pending-f_lineno tag cancellation and CALL-PC completion policy remain as
reviewed in `discard-completion-code-review.md`. Supported observation fallback
must be proved by native cases. Incidental cross-parent writes during a result
finalizer do not justify a new hot continuation check without a reconstruction
contract or evidence of a new safety defect.

Operand's first elapsed screen did not improve logging, despite its verified
22-instruction local path reduction. Keep that mechanism provisional and assess
net combined timing and required controls. Static handler inheritance or a lower
POPs/NOPs execution count is not an elapsed-speedup claim. A fresh warmed census
must distinguish retained canonical words, selected physical aliases and actual
entered handlers before crediting the nine general-call sites.

## Cache-metadata repair review

Reviewed the narrow follow-up in the boundary worktree after the full-suite
import-table pin failure: `pycore_magic_number.h` adds private magic 64411 and
its history entry; `test_import` updates its expected magic and matching table
pin. This enforces the existing handler-table policy even though execution-only
aliases are not emitted in canonical words. No handler, observer, GC or lifetime
behavior is changed by the repair.

Independently recalculated the digest using the exact test algorithm, including
removed/range policy lines: `c96c524dc507b07af93fd7f7a97e36b5e07752c0b85b96790e6f4981d741ff40`.
It matches the new 64411 pair. The test's checks remain intact and the existing
cache-tag namespace remains unchanged. Source disposition: no blocker; root
must rebuild and rerun native import/cache gates after updating the header.
The separately reported GC-verifier result is not fixed or explained away by
this cache-metadata change; matched control passes do not replace a full retry.

## Magic-header build dependency review

The cache-v2 mismatch is explained by the inspected build graph: Python/import.c
is the sole C/header source directly including pycore_magic_number.h, and it
exports PYC_MAGIC_NUMBER_TOKEN through both the magic accessor and `_imp` module
initialization. Makefile.pre.in's existing Python/import.o DTrace prerequisite
omits that private header, which is also absent from PYTHON_HEADERS. The release
import.o timestamp predates the new header (16:28 versus 16:40 local time).

Source-only disposition: approve adding
`$(srcdir)/Include/internal/pycore_magic_number.h` to the existing
`Python/import.o: $(srcdir)/Include/pydtrace.h` prerequisite rule. This focused
edge matches the actual include and avoids making every Python object depend on
the private magic header. Makefile regeneration must carry it into each build;
ordinary make should then rebuild import.o and relink before native magic/import
checks. No source edit, make invocation or build was performed by this reviewer.
Retain the failed cached receipt. The header/test 64411 change remains correct;
reverting the assertion would hide a stale binary rather than repair the build.

## Debug full-suite profiling failure: initial source investigation

The retained final-debug failure is the pure-Python ProfileTest inherited by
`test_profiling`, calling `max(*([0],), **dict(key=int))`. Its stats contain the
outer exec C call but neither max nor a visible `<string>` module frame. This
suggests checking exec-body entry instrumentation before assuming a missing
native max completion event. No source defect is confirmed by this observation.

The statement uses CALL_FUNCTION_EX/CALL_EX_NON_PY_GENERAL, outside both new
Python-discard selectors. The instrumented EX handler still passes
CALL_INSTRUMENTED through its unchanged runner; native completion emits CALL
before invocation and C_RETURN/C_RAISE afterward. Its destination is numeric and
postcall_discard is false. SAVES_IP has no profile-instrumentation mask or
truncating consumer; it is used by the bool selector, debug dispatch checks and
stats. No direct new-alias or transaction-flag cause was found in source.

Recommended coordinator controls: match the fresh individual inherited
ProfileTest.test_calling_conventions and full test_profiling module on golden
and candidate debug, then repeat all six original calling conventions with fresh
Profile objects in one process. If reproducible, record raw profile events in a
wrapper around the unchanged dispatcher and compare string exec with a known
precompiled unique code object; capture its executing IR afterward. Missing
module PY_START and max C_CALL points at entry instrumentation; module entry
without max C_CALL points at EX monitoring; an actual C_CALL without a stats row
points at Profile bookkeeping. Check RESUME/legacy tracing restoration only if
those observations support it. No native job, source edit or test expectation
change was performed here; do not classify this failure as fixed by source review.
