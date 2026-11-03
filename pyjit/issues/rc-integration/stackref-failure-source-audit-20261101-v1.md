# Stackref full-suite failure: source audit and one upstream control

2026-11-01. **Qualification remains blocked. Do not change runtime ownership,
disable the checker, or add exclusions on this source audit alone.** The
stackref-debug full row has real target failures. Root stopped its remaining
work at08:47:32 after563 of565 modules had returned:449 PASSED,94 SKIPPED,
12 FAILED and8 RESOURCE_DENIED. The observed timeout return124 follows root's
intentional SIGTERM, not expiry of the5,400-second run ceiling. No complete
full-suite result is claimed.

Source inspected is clean RC `dce654018d5ae16ffe5e772976d9ccc2c9d1e01a`.
Evidence is the retained `rc-stackref-debug-suite-v2/affected` log/state under
`build/worktrees/rc-successor`. Source comparison uses actual upstream commit
`321925c92c9e67864cafa1566840927f6f07e90a`, available in the existing clean
checkout `/home/power/code/cpython/.worktrees/boil-checkpoint-i-upstream`.
No new checkout, compiler, target, diagnostic or source mutation occurred.

## Three distinct mechanisms

**Open tokens after abandoned threads or fork.** The reported Thread/function/
code groups fit live frames abandoned during shutdown. Actual failed fork
methods create a daemon thread sleeping in C and fork before it finishes.
`PyThreadState_Clear` deliberately leaves `current_frame` intact; its own
comment describes threads leaving the evaluator while taking the GIL at
finalization. `tstate_delete_common` subsequently calls `clear_datastack`,
which frees stack chunks without closing frame slots. Fork removal calls
`PyThreadState_Clear` then deletes the old thread states as well. The same
raw-stack abandonment and warning contract exist in the exact upstream
`Python/pystate.c`; source similarity does not prove an upstream native
checker run passes or fails.

Normal frame completion is different: RC `clear_thread_frame` calls
`_PyFrame_ClearExceptCode`, closes `f_executable`, and pops storage. Generator
clear closes locals/function; `gen_dealloc` additionally closes its executable.
`MakeHeapSafe` closes the old debug token before creating the new heap-safe
one. I found no source proof that these normal return/move paths explain
the shutdown groups. Closing abandoned frames with ordinary DECREF would
introduce callbacks during an already-partial interpreter/thread shutdown;
it is not a safe mechanical fix merely to quiet the reporter.

**Objects created by the last GC.** The actual failed embedding method
deliberately creates an asyncio Task with an unstarted coroutine inside a
finalizer run during `interpreter_clear`'s last collection. Its Loop retains
the task callback, creating another cycle. After `_PyGC_CollectNoFail`, the
current source finalizes GC, clears sys/builtins/common constants, then
reports open stackref records. A new coroutine cycle can therefore still
own valid function/code tokens at that point. The test body, embedding C
entry, collection/report ordering and relevant open-record checker logic
also exist upstream. This supports a shutdown-domain hypothesis; it does
not yet prove that every observed open record is intentional or that RC
has no ordinary lifetime defect.

**Asynchronous and remote readers of table IDs.** Several actual
`test_faulthandler` failures report `PyInterpreterState_Get` without the GIL
or current thread state. The direct source path is
`traceback.c:dump_frame -> _PyFrame_SafeGetCode -> PyStackRef_AsPyObjectBorrow
-> _Py_stackref_get_object -> PyInterpreterState_Get`. The watchdog reader
need not have Python TLS. The corresponding upstream safe getter/debug
lookup has the same requirement. Separately, remote `frames.c:212,273`
reads the executable field with `GET_MEMBER_NO_TAG` as an object address,
while this debug profile stores a table ID. The retained sampling-profiler
zero-sample assertions are consistent with that representation mismatch;
they are not proof of leaked owning references or ordinary-build sampling
failure. No async table lookup or new frame cache is proposed here.

The reporter rejects genuinely open entries; it is not reporting an invalid
or double-closed ID. The fork's borrow-outlives-close extension suppresses
only records already marked closed, so it does not by itself explain these
open records. These families must not be collapsed into one presumed
interpreter ownership bug.

## One finite upstream checker comparison, held

Reuse the existing pristine upstream checkout above. Bind its existing
`build/span-elapsed-pilot-v4/upstream-source.json` manifest from PRIMARY,
recheck exact HEAD/cleanliness, and use a **new project-owned out-of-tree
build directory**, such as `PRIMARY/build/upstream-stackref-control-20261101`.
Do not overwrite that checkout's `build/default` or `build/eval-nogcse`.

Build one Linux/x86-64/GIL/GCC computed-dispatch/mimalloc O1/debug image with
`CC=gcc --with-mimalloc --with-computed-gotos --with-pydebug` and
`CFLAGS='-O1 -g -DPy_STACKREF_DEBUG'`. Upstream has no Boil shapes/IR option;
record that explicit source/profile difference rather than passing unsupported
flags or claiming a byte-identical runtime. Freeze actual selected tools,
configuration, all-unit define, interpreter/_testembed, own extensions,
stackref symbols and real compiled call witnesses. No ASan, CLOSE_DEBUG,
verifier, compiler/backend alternative or elapsed measurement is involved.

Run exactly these unchanged standard methods once through the standard
runner, seed0/no-randomize, in one fixed selected row:

1. `test.test_embed.EmbeddingTests.test_asyncio_task_during_interpreter_clear`
2. `test.test_faulthandler.FaultHandlerTests.test_dump_traceback_later`
3. `test.test_threading.ThreadTests.test_join_daemon_thread_in_finalization`

The third method has its existing two subtests; do not call three methods
three child programs. Retain actual selected IDs/statistics, raw stdout/
stderr, child exits and own upstream image/_testembed/config/source origins.
Use the existing finite supervisor/gate with both slots, own build lock,
pre-wait streams and dual-executable cleanup; adapt observation to upstream
APIs without importing Boil's historical verifier/CAPI requirements.
Propose one30-minute inclusive future activation: build1200 seconds and
selected tests180 seconds, with210 seconds reserve per row. A predicted
standard-test failure is comparative evidence, not an automatic rerun:
the reader must preserve and classify actual standard outcomes rather than
stop at the first expected test assertion without retaining the other two.
Unexpected build/import/crash mechanics stop the control. Root/independent
helper review and a native grant are still required.

Before that grant, prove the three method bodies are unchanged against the
RC source (the embedding method adds only a `requires_refcounting` decorator
in RC), along with the embedding C entry. Keep input/requested tests fixed;
do not add a full upstream suite or optional profiler row now.

If the upstream control reproduces an identical causal signature, that
specific nondefault-checker applicability issue is established and root may
review a precise qualification amendment. It does not turn the failed RC
full suite into a pass or excuse unrelated failures. If upstream passes a
method that fails in RC, investigate the fork's corresponding runtime/reader
delta with the retained counterexample. Mixed or different signatures remain
separate unresolved findings. No ownership/checker change is justified before
this comparison distinguishes product defects from valid shutdown behavior.
