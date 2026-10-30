# Main-interpreter-only admission checkpoint

Source-only implementation from safety `9e223040a2070c75f88be10366e0f0a7b35f6ed3`
on managed branch `boil/main-only`. This implements the coordinator's explicit
first-milestone boundary, not a workaround for a failed qualification result.
No native build or target execution has run at this checkpoint.

## Runtime cut

`Python/pylifecycle.c` clears the configured creator's output immediately and
rejects an initialized runtime with a main interpreter before disabling GIL
checks, changing the current thread state or reaching interpreter allocation.
It returns `PyStatus` with `subinterpreters are not supported`. The legacy
`Py_NewInterpreter` wrapper returns NULL rather than exiting on failure.

`Python/pystate.c` checks main-interpreter membership under `HEAD_LOCK`, before
advancing `next_id`, allocating or publishing a state. Checking the supplied
thread state would be wrong: the XI route deliberately passes through creation
with no current thread state. `PyInterpreterState_New` now returns NULL on
failure rather than exiting; no attached state or Python exception is required.
The internal main bootstrap remains available. Thread-state creation,
PyGILState attachment and startup XI metadata initialization are unchanged.

Full creation rejects before entering the allocator's audit path. The bare
allocator retains its existing attached-caller audit attempt before its locked
admission check; an audit-hook error still propagates normally. No head lock is
held during Python audit handling. This is rejection before interpreter-state
allocation, not a claim that arbitrary audit hooks allocate no Python objects.

The existing XI and `_interpreters.create` error conversion restores the saved
thread and supplies chained `InterpreterError`; no Python runtime file changes
or process fallback are needed. InterpreterPoolExecutor can be constructed,
but its first worker initialization fails and breaks submitted work. Ordinary
thread and process executors are separate APIs.

## Focused validation, not yet executed

The existing `test_capi.test_misc` module has a new five-method
`MainInterpreterOnlyTests` class. Its private `_testinternalcapi` seam covers
configured, legacy and bare public APIs plus the internal allocator, each with
and without a current thread state. It verifies error status/message and cleared
output arguments, unchanged attachment/main/head/next_id/GIL-check state, and
no accidental Python exception. Unexpected successful states are cleaned up
before reporting the named failure.

Other methods check both Python creation layers and their exception context,
the existing four-origin `create_interpreter` seam, actual interpreter-pool
worker rejection, and ordinary Python/native-thread callbacks using the
existing `_testcapi._test_thread_state` helper. No existing unsafe creation
helper is selected by these five methods, so none was changed opportunistically.

Only host Python3.14 AST parsing of the modified test file and `git diff --check`
have run. No generated products need regeneration. API/reference changes are
limited to the C subinterpreter, concurrent.interpreters and interpreter-pool
documentation. Review this source checkpoint before committing/building.

After review, reuse the accepted workstack `build_once.py` recipe with only
the owned root/HEAD/output bindings changed: fresh owned GCC release O3 then
O1/Py_DEBUG builds, frame pointers/zero-init, shared GIL, nonstats/no JIT,
make-j4, existing global two-slot admission/build locks/timeouts and
source/config/compiler/product freeze before targets and after cleanup. No
old168 native-equivalence claim is valid for these two changed runtime files.

The first bounded target should be exactly the new five-method class once in
release/off and debug/actual-kill, using the latest auxiliary-aware wrapper,
actual state getters, writable disposable `/tmp` cache, N0 variables omitted
and normal lock paths. Capture the eight C subcases and four existing-origin
subcases as subcases, not additional unittest methods. Stop on unexpected
failure. Ordinary thread-state tests that do not create interpreters can follow
under an explicit selection. No whole `test_misc` or broad suite grant is
implied: it still contains upstream subinterpreter-success expectations.

## Honest future unsupported-test classification

Current conventions conflate module availability with creation support:
`test_interpreters/test_api.py` imports `_interpreters` with
`import_helper.import_module`, while `test_capi/test_misc.py`'s local
`requires_subinterpreters` checks only whether that module imported. Both stay
unchanged here. Keeping `_interpreters` for main/current/list inspection means
neither convention identifies the new capability correctly.

Before broad qualification, use one explicit creation-capability predicate
with the existing `unittest.skipUnless`/`requires_subinterpreters` convention,
then classify only tests requiring successful subinterpreter creation. Do not
probe capability by attempting creation, remove `_interpreters` wholesale,
skip a mixed module or map success tests to subprocesses. Main inspection,
normal threads, rejection tests and unrelated extension tests remain eligible.
The declaration/interface and exact test ledger need separate review; this
checkpoint adds no capability flag or broad skips. In particular, existing
helpers that immediately dereference/end a successful new interpreter must be
classified or made failure-safe before they are selected. Historical results
and pending Queue2b owner evidence remain unchanged and distinct.

Shutdown, repeated embedding teardown, fork policy, same-interpreter buffer
release and finalizer delivery are outside this increment. Rejecting creation
does not solve any of those resource-lifetime obligations.

## Capability and classification successor

The reviewed runtime/docs checkpoint is committed as683a407eb15; its normal
thread control now uses `threading_helper.join_thread`. The coordinator then
approved reusing `sys.implementation.supports_isolated_interpreters=False`
instead of adding a new constant. `support.requires_subinterpreters` reads
that existing boolean directly. It neither probes creation nor hides module
imports; genuine import-availability guards remain separate.

The finite source classification uses that decorator for success-creating
methods or unconditional creation fixtures, not whole mixed modules. The
existing import-only local predicates are replaced. Mixed C-API configuration
and interpreter-ID classes keep configuration validation and ID conversion;
main/current/initial-list inspection, pure cross-interpreter data conversion,
same-interpreter queue/channel tests and normal threads remain eligible.
The seven success-only auto-injected C tests remain collected with visible
skips. Interpreter-pool worker cases are marked through their existing mixin;
eligible import/constructor tests move to InterpreterPoolAvailabilityTests.

The original root_stats_dump ID now checks the main/fork records alone, with
all original main collection bounds, two-process grouping, fork interpreter ID
and fork collection bounds retained. A separately decorated subinterpreter
method calls the same helper with the original additional-interpreter branch
and record assertions enabled. Neither branch fabricates journal records.

The explicit affected/preserved source-ID inventory is retained at
`build/main-only-source/capability/final-id-inventory.json`, alongside the
classification diff and initial reviewed scope. Its counts are source
declarations, not executed tests, inherited/generated test discovery or skips.
Both generated root-stats child scripts parse with host Python; changed test
files parse and the extracted capability decorator passes False/True controls.
No native assertion or compatibility result follows from those host checks.

Native feedback is narrowed to **one fresh O1/Py_DEBUG cohort** first, reusing
the accepted workstack build helper/guards. The first five rejection/thread
methods will run in actual kill/N0 only after source/build binding review,
then explicit eligible main/thread checks once. Isolated O3, ASan and full
runs wait for a coherent GC union; historical qualified results and unsupported
test eligibility remain distinct. No new lifecycle support is implied.
