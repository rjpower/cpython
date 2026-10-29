# Pre-I linear string concatenation: design readiness

Verdict: **REVISE before S1**. A private append epoch is a coherent route
to linear local concatenation without refcounts or ropes. The storage
audit is useful and leaves its unproved contracts explicit. The compiler
certificate and its owner are not yet specified enough to implement.
This review identifies no new defect requiring a FIX_FIRST change to the
current copying implementation. It grants no native implementation,
layout, build, probe, test or measurement authorization.

Reviewed primary `dfef38ab6b557067efd085e3b001372122f63daf` and:

- `pre-I-linear-string-concat.md`, SHA256
  `1616f77fe3ce3317f44f5c4ca07d3404d192a40433982d38bb54b3c91cdf47be`;
- `pre-I-string-storage-audit.md`, SHA256
  `73fae302bfa39a9901eff583ffd8242981faa3dc6b520a3ccc9ed40723c23a52`.

Applicable AGENTS instructions and the CPython AI policy were read. The
relevant Unicode, compiler, code, frame, IR and thread sources remain
unchanged from `9dd24bf42c4`. Evidence is source inspection and Git
metadata only. No native/build/test/probe execution or source edit occurred.
Other working changes, including README/PROCESS/WORKLOG, were preserved.

## Blocking before S1

### 1. Define the certificate algorithm and runtime epoch transitions

Parent section 3 and audit section 6 specify a desired invariant and final
verifier, but not the finite rules that establish it. S1 already implements
certificate derivation, verification and owner retirement; those decisions
cannot wait for S2 capacity storage or S3 observer closure.

The available compiler facts are precise but different from ownership.
`Include/internal/pycore_regcfg.h:38–50,58–80` separates popped reads,
preserved inputs, implicit exception-live registers, outputs, folded dst,
jump-only outputs and exception edges. Copy propagation only folds a
single consumed read and records the reader
(`Python/regcfg_passes.c:269–286`); store elimination can move a local write
into its producer (`:306–313,395–407`). Neither establishes that a private
value has no preserved alias. The pass driver compacts after those
transforms (`Python/regcfg.c:1405–1444`), and assembly adds real synthetic
jump words (`Python/assemble.c:830–887`).

The revision needs a small, explicit proof specification:

- State the abstract facts for locals and temporaries, their entry values,
  transfer rules and conservative joins, including loop fixed points.
  Define whether a strong copy always seals or can transfer permission;
  do not leave that to a later optimization. A branch with an escaped
  alias on only one predecessor must never regain the old epoch at a join.
- Define which reads are consumed by an approved append and how another
  ordinary or implicit use invalidates that permission. Cover aliases
  surviving an exception, overwritten locals, `LOAD_FAST_AND_CLEAR`,
  comprehension save/restore, self-append and a second private RHS.
- Define success, guard failure and exception-edge transitions separately.
  `Python/bytecodes.c:952–996` clears the specialized target after the
  append call, including failure; generic arithmetic and a folded store
  have different failure timing. State which old epoch retires and when
  the fresh result can acquire permission on each route.
- Give the final-word admission algorithm and payload schema: bounded
  word/local indices, canonical sites/operands, expected consumption and
  exceptional successors, and rejection conditions. Verification must
  check the emitted words/slots/exception metadata rather than compare
  two unchecked copies of compiler annotations. Unknown sites decline.
  No proof may rely on the current numeric handler order.
- Define a minimal owner API/state machine for the optional thread bitmap:
  enter/retire an activation, seal a local/all locals, overwrite a slot,
  and grant a fresh result. Explain why an old bit cannot apply to a new
  value or a reused frame address. Allocation failure must leave an
  unambiguous disabled state; a same-address/frame test is insufficient.

S1 controls should corrupt one implicit use, join, exceptional successor,
physical slot and synthetic-jump mapping and require the final verifier
to reject each named defect. Runtime shadow controls must witness stale
permission at the relevant transition, including frame-address reuse;
green copying output alone cannot establish the proof. The parent already
requires negative controls; this makes their first-rung oracle concrete.

### 2. Choose one immutable certificate owner before designing S1 code

Audit section 6 leaves a dedicated code pointer and an eager execute-owned
payload as alternatives. They change different layouts, construction
costs, failure paths and lifetimes. S1 cannot quietly choose one while
implementing the verifier.

Recommend one **dedicated optional code-owned pure-data pointer**, passed
through the private constructor and installed before CREATE, preserving
lazy execution-copy allocation. This also matches the coordinator's design
direction for the revision; H3/code-layout review is still required.
Do not implement `co_extra`, eager execution storage or dual backing as a
fallback. `_co_exec` starts NULL before CREATE
(`Objects/codeobject.c:459–485`); `Python/ir.c:247–316` currently allocates
words/feedback lazily. Post-return attachment is already post-publication.

Specify exactly when the constructor consumes/frees an optional plan,
initialize the field safely on every failure path, and pass NULL on
uncertified constructors, replace, marshal, static and frozen paths.
The instruction-sequence assembler (`Python/compile.c:1947`) is not trusted
merely because it shares compiler functions; it must earn the same proof.
Absence must not later upgrade from a supplied flag or copied certificate.

The payload survives DESTROY-watch resurrection and is freed only at actual
deallocation. `_PyCode_Destroy` notifies watchers and destroys extras
(`Objects/codeobject.c:2366–2375`); that is not its lifetime endpoint.
Account for the added field in every code object and the actual payload
in `code_sizeof` (`:2688`), and enumerate `_PyCode_DEF`/deepfreeze/static
initializers and remote/debug offset consumers before layout approval.
The proof payload contains no object pointers or callbacks and needs no
GC traversal. The owner-frame identity and bitmap lifetime still require
the separate H1/ROOTS interface review.

## Storage and semantics that hold, with their existing later gates

| Area | Source assessment and boundary |
|---|---|
| Freshness and exact types | `PyUnicode_Append` copies nonempty strings, aliases on empty shortcuts and clears its output on error (`unicodeobject.c:11708–11783`). The specializer only checks a folded local's identity (`specialize.c:2411`); this is no uniqueness proof. The proposed fresh-private-helper rule, exact types, no constructor adoption and seal-before-reflected/subclass fallback are necessary. |
| Trailer arithmetic/layout | `H + (C+1)*kind + sizeof(Py_ssize_t)` contains the movable footer after logical `L+1` units when `0 <= L <= C`. `memcpy` handles alignment; checked arithmetic must include every addition/product. ASCII uses `PyASCIIObject`, other compact forms `PyCompactUnicodeObject` (`unicodeobject.h:100–173`); ASCII-to-Latin-1 must allocate even though kind stays 1. No legacy/subclass storage is admitted. |
| Producer closure | Dynamic state assignment at `unicodeobject.c:1287–1291` and subtype assignment at `:14288–14295` must explicitly zero a named new bit. Common static initialization (`pycore_runtime_init.h:191`) and both deepfreeze aggregates (`deepfreeze.py:225,238`) must remain ordinary. The audit correctly distinguishes character copies from unchanged-result aliases and preserves storage metadata after sealing. |
| Writes/resize/ownership | `_PyUnicode_ResizeCompact` reallocates and preserves header bits (`unicodeobject.c:1077–1137`), so flagged storage must be excluded. Public resize already copies (`:1681–1725`). Writer writable adoption records logical length as capacity (`pycore_unicodeobject.h:127`); it must stay ordinary-only. Read-only adoption (`unicode_writer.c:267–270`) is safe because prepare copies before writing and readonly finish returns unchanged (`:563–565`); mutable narrowing/truncation (`:579–600`) remains ordinary-only. |
| Immutable observations | Hash/UTF-8 caches, interning, DATA pointers, iterator retention and identity-returning constructors are publication hazards, not permission sources. The permanent storage bit/footer must survive seal/hash/cache/intern/immortal transitions; private conditions must not be imposed on sealed storage consistency. Public character writers' existing ownership precondition (`unicodeobject.c:1316`) cannot be weakened by the bit. |
| Accounting and physical reads | `__sizeof__` currently charges logical extent (`unicodeobject.c:13810`) and needs full reserved request plus separate UTF-8 accounting. Collector live bytes charge allocator blocks (`gc_trace.c:2143`), a different quantity. Inline DATA access stays direct; remote reads already copy a header-sized prefix beyond short logical data (`object_reading.c:51–61`). The audit appropriately narrows the parent's overly broad physical-read claim. ABI offsets, bit positions, alignment and debug/ASan extent remain S2 qualification obligations, not established results. |

The audit establishes a credible S2 candidate, not representation approval.
Its permanent bit, all three header layouts, ordinary-only writable paths,
footer relocation/terminator integrity, seal/cache/intern/free behavior and
requested-size checks belong to a focused Unicode/C-ABI prerequisite.
They need not be implemented to define a mutation-disabled S1 proof.

Likewise, S3's observer/foreign-entry inventory is explicitly deferred;
its incompleteness is not an extra pre-S1 mutation blocker invented here.
Before S4 it must close real entry paths, not only family flags: frame
materialization/proxy/get-var access (`frame.c:23`, `frameobject.c:275,2471`),
custom eval-frame entry (`pycore_ceval.h:118–124`), actual pending callbacks
and stop/drop paths (`ceval_gil.c:1248`), direct detach/suspension
(`pystate.c:2149–2166`) and collector destroy callbacks
(`gc_live.c:385–425`). Empty polls may preserve an epoch. Custom allocators,
reftracers and unresolved collecting/opaque effects must decline the feature.

Growth must retain rooted original operands and reload object/DATA addresses
after collection. No allocation, callback or safepoint can occur during the
in-place length/terminator/footer transaction. Spare-reserve failure may
retry ordinary exact allocation only after handling its speculative error;
that retry must preserve the executing path's binding/exception outcome and
counts as fallback, not linear-growth evidence. These are already sound
requirements, whose ROOTS/transaction controls remain S2–S4 work.

## Smallest next reviewable slice

Revise a **pre-S1 proof/owner design**, without changing Unicode storage:

1. Compiler/string owner defines the transfer/join/exception algorithm,
   pure-data plan, final-slot/word verifier and falsifying fixtures.
2. H3/code-layout owner reviews the single pre-CREATE code-owned interface,
   constructor/static consumers, NULL/failure/resurrection lifetime and
   field/payload accounting. Existing generated-handler ownership stays
   with H3; no checker/CAPI grants are implied.
3. H1/ROOTS review the bounded activation/slot retirement API and shadow
   state lifetime. Name the exact shared hunks before assignment. This
   does not grant the full observer, collector or thread files.

Then review mutation-disabled S1 for a development checkpoint. Do not land
permanent production barrier overhead merely to retain unused machinery;
the audit's zero-production-cost/shadow boundary and later matched release
overhead experiment are appropriate. S2 capacity/ABI and S3 observer closure
remain separately owned prerequisites to enabling S4. The final geometric
copy bound, interrupted epochs, retained published slack, RSS/pauses and
non-concatenating workload overhead are appropriate pre-JIT acceptance
criteria. No arbitrary operation/count threshold or refcount resurrection
is needed.
