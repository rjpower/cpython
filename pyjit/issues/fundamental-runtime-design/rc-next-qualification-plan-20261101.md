# RC next compatibility tranche

2026-11-01. **Design only; no execution or collector-selection authority.**
Use clean `d7df12b1d81722927e71d9734c70eeafdccfee73` and its existing Linux/GIL/
GCC/computed/mimalloc/boxed-shapes debug and release images. The accepted
39-method cohort passed on each image. Canonical Autoconf now reproduces that
same source exactly; it requires no replacement build. Preserve those receipts.

The smallest useful next step is C-extension ownership and interpreter lifecycle,
not another repetition of the 39 cases. Freeze these two serial rows:

| Row | Exact selection | Method invocations | Target-program processes |
| --- | --- | ---: | ---: |
| Existing debug | 48 shared IDs plus two debug refcount-abort IDs | 50 | 19 |
| Existing release | Same 48 shared IDs | 48 | 17 |

The authoritative literal IDs and AST/source hashes are
`build/rc-next-qualification-plan-20261101/roster.json`, SHA256
`56eca960b39a15368ced921b412770d91e09c9c81810d1fba6e6eb55d846f306`.
All 50 methods exist in the frozen source; none overlaps the passed 39 IDs.
The shared 48 comprise:

| Module / classes | Methods | Purpose |
| --- | ---: | --- |
| `test_capi.test_object`: GetConstantTest, ClearWeakRefsNoCallbacksTest | 4 | Owned/borrowed constants and suppressed weakref callbacks |
| `test_capi.test_list.CAPITest` | 8 | New-reference getters, stealing/replacing setters, reentrant deletion, clear/extend |
| `test_capi.test_tuple.CAPITest` | 8 | Owned construction/stealing, get/set, resize and failure paths |
| `test_capi.test_dict.CAPITest` | 6 | Strong-reference getters, setdefault/pop/iteration/update |
| `test_capi.test_frame.FrameTest` | 4 | Materialized locals, generator ownership and native frame construction |
| `test_capi.test_weakref.CAPIWeakrefTest` | 4 | Strong referent acquisition, death and callback delivery through C APIs |
| `test_capi.test_module.TestModFromSlotsAndSpec` | 4 | Native module GC hooks, exec/create and invalid repeated ownership slots |
| `test_capi.test_slots`: TypeSlotsTests, ModuleSlotsTests | 4 | Limited/full C-API heap types, nested slots and bases |
| `test_embed.EmbeddingTests` | 6 | Subinterpreter state, init/finalize loops, inittab, native module init, static parser/structseq reset |

The six embedding IDs are `test_subinterps_distinct_state`,
`test_repeated_init_and_inittab`, `test_create_module_from_initfunc`,
`test_run_main_loop`, `test_finalize_structseq` and
`test_getargs_reset_static_parser`. Each calls the existing `_testembed` once;
its repeated interpreter lifecycles occur inside that C process. Both existing
`Programs/_testembed` binaries are present and SHA-bound in the roster. Missing
or changed binaries must fail preflight, not trigger compilation or a skip.

The debug-only extras are `test_capi.test_object.CAPITest.test_negative_refcount`
and `test_decref_freed_object`: one deliberate abort child each, requiring the
existing exact negative-refcount stderr assertion and nonzero return. The source
exports these APIs under `Py_REF_DEBUG`. Do not select them in release and then
claim their skips as coverage. Expect zero selected-method skips on this declared
Linux configuration; missing APIs or unexpected skips stop the batch.

Process accounting is **36 target programs**, not 98 or the number of logical
subinterpreters: two ordinary drivers plus eighteen module workers, two platform
auxiliaries, twelve embedded C programs, and two expected-abort Python children.
The platform calls are source-explicit in `libregrtest/utils.py:671` when using
the existing verbose `--python` worker route. With these exact selections there
are no other test-body process launches. Host/gate wrappers are separate. Bind
actual scheduling/returns/PIDs and preserve the source-derived distinction;
helper review must reject an unobserved child or an extra target invocation.

Reuse the RC gate, build locks, source/product/config/tool validation and worker
result observer. Use the same own stdlib/extensions and ABI namespace; the
limited-API extensions here are the locally compiled RC ones, not upstream abi3
binaries. Request diagnostic OFF with `PYTHON_GCVERIFY=0`, seed0, no stress or
resource sweep. Retain only the real historical count/holes observations; do not
invent tracing mode/rootstats APIs or qualify the diagnostic checker.

Two concrete helper deltas are necessary before a native grant. First, capture
the selected `test_embed.run_embedded_interpreter` calls with their unchanged
argv/assertions and pre-wait stdout/stderr spools; its current PIPE/communicate
path can lose partial output on an outer kill. All six selected calls use no
stdin. Reuse the existing accepted stream-spool pattern, not a new executor.
Second, extend the existing owned-process cleanup check to both `python` and
`Programs/_testembed`; the old Python-only idle check is insufficient. Expected
abort auxiliaries need their own exact negative result records. Review these
narrow adapters and timeout controls before activation; no test/runtime edits.

Propose one **30-minute inclusive** native window, both machine slots and global
serialization, two image locks, 600-second ceiling per row downward-clamped to
remaining time minus the existing 210-second cleanup reserve. Debug runs first;
stop on the first unexpected failure. No automatic rerun, alternate allocator,
new build, extra verifier row or selection change. Pass requires all 98 selected
invocations, the exact process ledger, expected abort evidence, unchanged source/
products, actual import/ABI origins and all Python/embedding processes idle
before the deadline. Failures remain reviewable; no assertion/skip relaxation.

This is the next compatibility tranche, not complete integration qualification.
It deliberately exposes extension/embedding ownership errors before a fallback
is adopted. Whole-stdlib coverage, debug reference-leak runs, broader import/
extension and embedding configurations, ported third-party extension rebuilds,
Windows/other platforms and free-threaded support remain separate obligations.
In particular, the RC ABI isolation intentionally rejects unsafe old binaries;
passing locally rebuilt C-API tests does not promise their compatibility. No
qualification result is inferred from this source-only plan, and the GC cutoff
and reference-counting fallback decision remain the coordinator's responsibility.
