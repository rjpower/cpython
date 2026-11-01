# Reference-counted successor: source candidate

2026-11-01. This isolated source preparation follows restoration commit
`1440f8c6fd1` on `boil/rc-successor`. The coordinator's grant began at
03:36:37 UTC and ends at 05:07 UTC. Work began after the independent span
review; the checkpoint and first closure edits were made at approximately
03:42–03:44. No compiler, target interpreter, workload, counter probe or timing
ran. The primary runtime and every tracing candidate remain unchanged.

## Fixed ownership and retained compiler

Reference counting and the cyclic collector now own memory. Historical owning
`Py_GC` branches were mechanically resolved to false in 69 C/header files;
unrelated preprocessor choices retain their original bodies. `gc_live.c`, its
Unix/Windows build entries, `--with-gc`, and the generated configuration macro
were removed. Supplying `Py_GC` externally causes an explicit public-header
error rather than disabling reference operations. There is no owning collector
selector. Generic platform/free-threading code was preserved; the first supported
configuration to qualify is Linux, GIL, mimalloc, computed dispatch and historical
boxed shapes (`--with-shapes`), with no configuration sweep.

The restored register compiler, CFG/liveness/compaction, load/store/copy/truth
passes, feedback, call-return-local optimization, and NaN-boxed owned/borrowed
stack references remain. Their ownership now reaches real INCREF/DECREF and
`_Py_Dealloc`. The historical diagnostic verifier is retained; it neither owns
nor frees objects. Its registered native roots do not acquire references or
keep objects alive, and their API documentation now states that obligation.
Current tracing-only effect metadata was not copied into this generator.

Later public PyValue, inline containers, value-native slots and header/unboxed
shapes are deferred. Containers and native APIs use the restored pointer-based
ownership model. JIT groundwork survives; a functioning JIT is not supplied by
this source change. The original fallback decision records these tradeoffs.

## Artifact isolation

The successor uses SOABI prefix `boilrc1-`, cache tag `cpython-rc1-316`, and
bytecode magic 64401. Both debug and release SOABI forms are distinct from
current tracing and historical RC/upstream artifacts. POSIX/Cygwin automatic
extension discovery no longer accepts generic untagged or upstream abi3 names;
extensions must be rebuilt for this source ABI. The extension-finder test now
checks that contract. This does not make deliberately renamed/explicitly loaded
foreign binaries safe. Windows native suffix/toolchain isolation remains a
separate qualification obligation; the Windows source lists preserve CRLF and
no longer compile the removed owner.

Historical `test.support.Py_GC` is a constant false solely to preserve existing
test imports, and `requires_refcounting` returns its test directly. It cannot
select a runtime mode. Seventeen tests exclusively for the deleted owning
collector were removed. Four new focused methods check immediate finalizer and
weakref release with cyclic collection disabled, reentrant container replacement,
actual ownership-count changes, and the cache/extension namespace. These tests
have not run on the target.

## Host evidence and limitations

Artifacts are under `build/rc-closure/` in this worktree:

- `owner-projection.json` binds all 69 original and projected source hashes;
  `project_owner.py` validates supported conditions before any write.
- `projection-proof.json` replays that projection against the exact restoration
  commit. Fifty of the 83 changed tracked paths are exactly mechanical output;
  `policy-delta.patch` isolates the remaining 33 paths for source review. New
  tests and issue notes are additional paths. The proof honors the repository's
  explicit CRLF policy for the two Windows XML files.
- The existing 26 source-generator methods pass. Regenerated IR handlers and
  dispatch tables exactly match the candidate. Existing compiler/generator
  bodies were not changed. `host-checks.json` binds the source checks and logs.
- The generated configure script passes shell syntax checking. Its removed
  option/define and both SOABI edits match `configure.ac`; Windows XML parses.
  Owning live-collector references are absent from core/build paths, apart from
  the intentional external-macro rejection.

Canonical autoconf is unavailable on this host; the repository regeneration
script requires a container. The coordinator authorized a precise generated
counterpart edit for source feedback. Canonical configure/autoheader regeneration
is still owed before landing. No configure/compiler probe was run.

The host is Python 3.14; the restored support module already uses Python 3.16
`lazy import` syntax. Its full AST cannot be checked by this host. The changed
metadata statements and the three changed/new ordinary test modules compile;
the support module's complete grammar and behavior remain native gates.

Preparation corrections remain recorded: a projection-created trailing blank
line; one blank-line difference between direct handler projection and actual
generator output; and the host parser's inherited lazy-import limitation. The
first raw-Git projection proof is also retained: it showed artificial whole-file
Windows differences until checkout CRLF policy was applied in the proof. No
runtime guard was relaxed. Closure-only diff checking against `1440f8c6fd1`
passes; this does not erase the two documented historical EOF warnings in the
earlier core-restoration commit.

## Next gate, held

Independent exact source review comes first. A separately reviewed, activated
fresh debug build should then run `test_rc_runtime`, existing RC/cycle/weakref/
finalization controls, compiler/interpreter/frame/coroutine controls and native
ownership cases. It must use the restored API and current actual configuration,
not fabricate present tracing root counters that this ancestor never exported.
Debug refcounts, stackref-debug, import ABI/cache refusal, reinitialization and
subinterpreter/native-extension qualification remain outstanding. Any first
unexpected native failure stops the batch and remains evidence.

This candidate is a prepared fallback, not a promotion or a parity result.
The user's GC deadline remains **2026-11-01 15:14:44 UTC**. If tracing fails
the declared all-29 geometric-mean elapsed parity decision, the coordinator can
promote a qualified RC successor without trying to reverse the tracing tree by
merging an ancestor branch.
