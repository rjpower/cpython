# C: complete conditional leaf prototype implementation brief

**Implement one complete conditional family with callback-free retirement.**
Use the approved [nonfinal-release scope](leaf-no-callback-scope-review.md),
not a general ordinary-completion/debugger redesign. The implementation is held
for independent Sol review. This brief does not authorize a native job or commit.

Runtime file ownership is exclusively in
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/deltablue-leaf-prototype`
at starting commit `986c65ee5ab`. That managed worktree is clean and already
contains canonical class-scalar lookup. Classmethod work continues independently
in the integration worktree. Canonical documentation ownership is limited to
this file and `progress-leaf-contract.md`; no design/status changes.

## Files and interfaces

| File in isolated runtime worktree | Owned change |
| --- | --- |
| `Include/internal/pycore_ir.h` | Scalar leaf-path/region structs and limits; optional `_PyCodeExec.leaf` pointer. No public ABI or serialized feedback changes. |
| `Python/ir.c` | Whole-body certificate and optional record allocation in `make_exec`; static exec initialization, clear and sizeof accounting. |
| `Python/ceval.c` | Shared raw guarded readers/comparison; entry admission; structured leaf execution; aggregate nonfinal-release preflight; terminal no-callback completion. Place definitions before generated-handler inclusion. |
| `Python/bytecodes.c` | Ordinary readers/comparisons use shared raw primitives; add leaf terminal marker only to plain exact-call entry after checked prefix. Retain ordinary publication/retirement. |
| `Tools/cases_generator/ir_generator.py` | Marker emitter and positive source/format/prefix/terminal-owner validation. Prevent accidental bound-method enablement through initializer reuse. |
| `Tools/cases_generator/analyzer.py` | Root-approved additional ownership: audited raw probes/scalar comparison helper effects and the checked callback-free conditional marker. No general exemption. |
| `Modules/_testinternalcapi.c` | Small code-specific certificate/hit/miss inspection API, returning ordinary Python integers/tuples; no optimization controls or fake forced hits. |
| `Lib/test/test_ir_leaf_calls.py` (new) | Discoverable runtime witnesses, using repository unittest conventions. |
| `Lib/test/test_ir_leaf_codegen.py` (new) | Producer rejection/terminal ownership tests via `test_generated_cases` parser/generator helpers. |
| `Lib/test/test_ir_attr_cache_codegen.py` | Update affected source-shape expectations to the shared probes while preserving guard/acquire/publish/retire assertions. |

If a mandatory check requires a new owner file or semantic expansion, report it
before extending scope. Generated C/header outputs are coordinator-owned; worker
does not regenerate them. Worker code changes must identify exactly which
products root must regenerate from the changed producers.

Use internal static primitives in `ceval.c`, with typed local borrowed outputs:

```
PyValueBorrow ir_probe_global_value(PyObject *globals, PyObject *builtins,
                                    int live_handler, const _PyFbGlobal *);
PyValueBorrow ir_probe_shape_value(PyValueBorrow owner, const _PyFbAttr *);
PyValueBorrow ir_probe_class_value(PyValueBorrow owner, const _PyFbAttr *,
                                  bool check_metaclass);
int ir_probe_inline_compare(int mask, PyValueBorrow left, PyValueBorrow right);
```

These return MISS without mutation, exception changes, allocation, owner release,
specialization or generic lookup/operator dispatch. Value probes return borrowed
NULL for MISS (Python None is non-NULL); comparison returns -1/0/1 for
MISS/false/true. No output parameter introduces address-taken handler locals.
Ordinary handlers and leaf
execution call the same guard/read implementation; do not merely duplicate its
guard sequence in a new helper. A live-handler/typed-entry dispatcher belongs
only in the attribute/global reader, not in an opcode evaluator. Start with
LOAD_GLOBAL_MODULE/BUILTIN, LOAD_ATTR_SHAPE and canonical
LOAD_ATTR_CLASS/WITH_METACLASS_CHECK. Preserve all current ordinary coverage.
The inline comparison primitive shares actual integer-scaled/float comparison
rules with normal comparison operations. Only leaf admission rejects mixed
numeric, bool, heap numeric and user operators initially. Ordinary comparison
admission and boxed/general paths stay unchanged; share scalar computation only.

Probe factoring must preserve positive canonical-source validation: exact probe
assignment with typed metadata, owner and guard-selection arguments, then direct
`IR_CANONICAL_HOME_DUP` and checked receiver retirement without intervening
effects. Keep strict suffix validation. Host real-source tests follow that
structure; wrong probe/arguments/guard selection or an intervening effect must
reject. No function-name-only exemption or broad source-shape waiver.

Existing checker rules forbid `PyValueBorrow` arrays/fields. Keep predicate
operands and intermediate path results in scalar C locals. Bounded path metadata
contains only integer indices/tags; retirement arrays contain actual moved
`PyValue` owners. Alias tables may contain raw object identities under unchanged
GIL-held roots. Use `PyValue_RefcountOnObject`, canonical duplication and normal
CLOSE/CLEAR; no bit casts, encoding literals, checker allowlist expansion or
new persistent borrowed value representation.

## Certificate and execution representation

One region is `select(optional compare(path, path), path, path)`. Each of four
paths has an argument/constant/global root plus zero to two homogeneous guarded
attribute steps. The absent predicate returns its first arm. Constant/argument/
getter returns are degenerate instances of this representation; direct scalar
comparisons can use canonical True/False literal roots. There is one fixed
execution shape, with bounded attribute-step iteration only. No eval-node
recursion, arbitrary opcode loop, pattern-name table or generated per-function C.

Budget: at most 32 final immutable words, one comparison/branch, two terminal
returns, eight consumed call-window homes and two attribute steps per path.
Decline over-budget programs. Derive argument roots from bound argument slots;
derive folded constant roots from `_PyCode_CONSTSLOTS`/CO_FAST_CONST as well as
ordinary immutable constant loads. Read current `co_consts` values, including
mortal replacements. Do not assume constants are immortal or that every root
has an explicit LOAD_FAST/LOAD_CONST word after compaction.

Certification in `ir.c` uses immutable `_PyCode_InstrHandler/Oparg`, field roles,
effects, feedback initializers and branch target encoding. A bounded temporary
definition table can map at most 32 definitions/current slot bindings; do not
allocate an IR_MAX_SLOTS-sized table on every call. Track folded local argument/
constant sources and COPY/load forms, dominance and physical slot reuse across
both arms. Every executable word must be accounted for, with all paths returning
the described value. Reject unreachable extra words, dead potentially raising
reads, loops, unsupported joins, calls/writes/allocations, handlers, cells/freevars,
variadics, kw-only requirements and unknown prefixes. Require entry RESUME oparg
zero. Broad effects are rejection evidence; admitted generic reads/comparisons
still need their explicit guarded-runtime obligations.

The optional leaf record owns no Python object or back-reference. It stores
source word/argument/name/constant indices, comparison mask/sense and bounded
path data. Allocate only for accepted code after classification. Allocation
failure may decline cleanly without a pending exception. Initialize/account/free
it in `make_exec`, `_PyIR_InitCleanup`, `_PyCode_ExecSizeOf` and
`_PyCode_ClearExec`; handle the existing no-feedback early return. Code replacement
gets a fresh record. Separate scalar diagnostic counters from immutable syntax
fields in that optional allocation, avoiding per-code global counter storage.

## Entry, nonfinal retirement and terminal hook

Admit only plain CALL_PY_EXACT_ARGS, a thread-owned caller, normal captured
function object/value providers, valid checked version, current selected code,
all arguments supplied and matching split-self arity. Preserve stack-space and
recursion guards. Reject pending caller redirect and interpreter finalization
that invalidates immortal home invariants. No wrapper expansion or generic
binder belongs in this hit.

Initially GIL-only and non-Emscripten; WITH_DTRACE may conservatively decline.
Reject trace/profile/PEP523 and tracing recursion, all global monitoring activity,
selected code `_co_monitoring != NULL`, stale entry instrumentation and pending
eval work. Require the live entry RESUME to be plain and full eval_breaker equal
to its code instrumentation version; recheck before mutation. Debug initially
declines unless LLTrace absence is established without generic lookup. Do not
claim debug hit coverage while taking that conservative decline.

Once admission passes, probe predicate and only its selected result path, then
acquire the canonical result while every original root remains live. Enumerate
all <=8 window releases plus old LOCAL **or** discarded acquired result. Aggregate
count-owning releases by object identity, not equality; require post-acquisition
`Py_REFCNT(object) > total_releases(object)` for every group. Overlapping temporary
destinations count their old owner only in the window. Nonoverlapping temporaries
must be NULL. On any preflight/admission miss, close only the acquired result
duplicate and preserve original homes. Its original protected source makes that
close nonfinal. No temporary pins may force the test to pass.

After commit there is no fallible operation: set numeric return recipient/real
next PC, detach/NULL the window, perform nonfinal closes, hold result through
retirement, publish via `_PyFrame_SetReturnValue` under pending_redirect==0, and
return DONE/next. Old LOCAL/discard release is preflight-covered; function survival
prevents code/globals destruction cascades. Deferred RC is not supported by this
GIL proof. Remote optimized stacks may omit the elided callee. Exact historical
finalizer timings/debug transcripts are not gates.

Introduce a generator-recognized terminal marker after the exact checked prefix
and before frame initialization. Prefer a dedicated plain-exact initializer
wrapper/marker over enabling shared bound-method initialization by accident.
Positive generator validation checks PEP523/version/arity/stack/recursion guards,
untouched physical window/destination and no prior effect. A two-word
`{next,status}` outcome has MISS and DONE: MISS runs the existing initializer;
DONE reloads slots and dispatches next without output flush, delayed window move,
`_SAVE_RETURN_SLOT`, `_PUSH_FRAME`, recursion decrement or fictitious child.
No fallback/error branch exists after commit. The ordinary path stays unchanged.

## Discovery, checks and scope cut

Add tests to real modules `test.test_ir_leaf_calls` and
`test.test_ir_leaf_codegen`; use `test.support.import_helper`, `reset_code`,
specialization threshold and the existing generated-case harness. Runtime tests
inspect certificate/hit counters so successful fallbacks cannot impersonate hits.
Use stable long-lived function/receiver roots for positive nonfinal cases.

Cover both conditional arms; constant/argument degeneration; complete-body
side-effect/extra-word/exception rejection; `code.replace` and mortal constants;
same-code functions with distinct globals and existing-key rebinding; descriptor,
metaclass/provider/type/shape replacement; custom vectorcall, wrong/default/
keyword arity; callback comparison fallback once; local-only/PY_RETURN-only/
LINE/BRANCH monitoring, trace/profile, epoch/pending work; recursion/stack guard;
aliased window+LOCAL+result, function also argument, overlap and discard. Sole-owner
function/receiver/finalizer and refcount==release-multiplicity cases must MISS;
the ordinary path must run once. Include codegen negatives for missing/reordered
guards, changed format, bound-method reuse and nonterminal stale-owner output.

Root will regenerate products and run release/debug compile, value API checker,
new modules and existing `test_ir_frame_calls`, `test_ir_attr_cache`,
`test_ir_attr_cache_codegen`, `test_ir_canonical_home`, `test_ir_discard_completion`,
`test_ir_semantic_completion`, `test_call`, `test_monitoring`, `test_sys_settrace`,
`test_sys_setprofile`, `test_capi.test_function` as applicable. Worker may run
host syntax/generator validation only through configured-cache
`uv run --no-project python -B`; no regeneration/native build/test or commit.
The strict `Tools/build/check_value_api.py` remains intact.

After independent review/root greenlight, implementation budget is 40 minutes.
First finish a hooked, correct single conditional family and its negative
witnesses; cut optional compare-return spelling and extra read providers before
cutting complete-body validation, shared semantics, admission, aliases or terminal
ownership. If those minimums do not fit, report an incomplete prototype explicitly;
do not present unhooked metadata as structural progress. Root owns all integration,
native evidence and measurement. No changes to benchmark kernels or name-based
admission, no commits, and no edits in the integration worktree.
