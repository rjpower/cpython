# Select a managed-values method and invoke it

Status: qualified, measured and rejected. Both 117/79 builds and all 227
focused methods pass per role after baseline-checked fixture corrections.
Independent native review and actual Delta/ray first-hit probes pass. The
stable hot-eight screen improves comprehensions 3.5% but loses DeltaBlue 3.2%
and logging 22.9%. No all29/upstream/full campaign follows; see the
[trial report](report.md). The source packet starts independently from
selected57 `0dcba75bb3df40c32137db453b001f7e8f101de0`; reuse the reviewed
[prepared-call machinery](prepared-call62.md) where it supplies this contract,
without making a losing executable a prerequisite.

## First complete consumer

Admit a two-word `LOAD_ATTR` method selection followed immediately by positional
`CALL` with zero explicit arguments. The receiver is a local; the ATTR result A
is the distinct scratch CALL base B, and its second result is B+1. The active
prefix is specifically the existing `LOAD_ATTR_METHOD_WITH_VALUES`, and the
consumer is the existing supplied-self `CALL_PY_EXACT_ARGS`. Do not add a
NO_SELF member, defaults binder, generic descriptor consumer or third site.

First optimize thread-owned callers. A heap caller leaves the active head for
its mapped ordinary METHOD handler before acquiring either result; the ordinary
pair then performs its existing argument transfer. This branch is charged to
the complete path. Neither receiver ownership
nor the callee's strong function reference is removed.

The specific removals on an admitted hit are the METHOD-to-CALL handler transfer
and transient callable-home store and reload. One explicit NULL clear stays,
replacing the old store/load/clear sequence. The receiver's owning
B+1 store stays: with a thread caller it is already the final overlapping callee
argument home. The method DUP stays and transfers to `f_funcobj`. Both selected
method outputs already have canonical TEMP writers; no generic normalization
or destination-classification saving is claimed.

## Source-derived composition

The prefix authority is the existing `canonical_output_sources` walk in
`Tools/cases_generator/ir_generator.py:6621`, applied to the actual unchanged
guard/producer parts in `Python/bytecodes.c:4079–4097`. It checks the guarded
class-value DUP, TAKE of the current receiver, complete flat guard bodies and
canonical owning outputs. Type/version and managed-inline-values guards remain
the existing emitted parts. A missing or changed receipt disables the member;
its name is not permission.

Add one bounded generator input-source description, `CallInputBinding`, for a
current scalar CALL input: role callable, actual owning Local/token from the
prefix, original materialization home B, canonical fact and first consuming
operation. This is compiler/emitter state, with no runtime descriptor or frame
field. The ordinary binding remains its original owning home. Only this complete
source recipe may supply the native binding.

The binding has three consumers in the shared CALL emitter:

1. Input acquisition reads the current bound VALUE rather than window[0]. The
   existing pointer-space guard uses that VALUE, rather than an address-based
   guard on the deliberately empty window[0]. Existing function-type/version,
   arity, PEP523, stack-space and recursion guards stay in their original order.
   There is no redundant second callable type/version guard.
2. Every precommit CALL miss materializes the native owner exactly once into B
   using the existing canonical fresh publication primitive, consumes the native
   receipt, and transfers to ordinary interior CALL. It does not close/reacquire
   the method, repeat ATTR, or continue with stale C aliases. Heap callers have
   already left before acquisition and do not need this materialization exit.
3. Replace the raw `window_base[0] = PyValue_NULL` in
   `_INIT_CALL_PY_EXACT_ARGS` with a shared callable-transfer marker. Its ordinary
   home binding emits that same clear; its native binding consumes the current
   owner receipt after the same `_PyFrame_Initialize(..., callable, ...)` has
   adopted it, without writing B. INPUTS_DEAD/WINDOW_TAKEN and the existing
   `_SAVE_RETURN_SLOT`/`_PUSH_FRAME` remain the same operations.

This marker changes where the existing owner comes from, not who owns it after
initialization. It must have an ordinary lowering for every existing consumer;
do not copy the initializer/provider body, substitute C strings after generation,
or make all CALLs accept a new borrowed callee ABI. Generate the two bodies from
the same parsed parts and compare the ordinary receipt/body to its original
lowering. The composed receipt may differ only by the declared callable-source
substitution and its materialization/transfer actions.

Prefix and inner CALL still have separate lexical/two-pass emitter states. A
small explicit handoff passes the one actual Local/owner receipt between them;
it must not copy unrelated native-owner sets or home graphs. At first publication,
fallback or transfer, all aliases of that receipt cease being consumable. Address
escape, shadowing, an intervening flush/reload/callback or an unrecognized use
rejects composition. Keep active-head normal/error/owner/completion certificates
UNKNOWN unless independently emitted from the complete composition. Union actual
effects and `runs_code`; keep original ATTR operand geometry and semantic base.

## Admission and physical state

Extend the existing final-fixed-point nomination in `Objects/codeobject.c`, not
the frontend alone. Validate the original method bit, local receiver, distinct
scratch outputs B/B+1, zero explicit CALL count, exact adjacency and matching
roles. CALL's independent interior entry remains legal under its ordinary
prestate. Constructor, replace and marshal take this same path.

Both outputs require positive NO_HOME and no MAY_OWNER before ATTR. No physical
NULL proof or whole-image completeness is required. Immediately after the actual
prefix acquires its canonical owning method and receiver, clear B with a direct
write-only `IR_SLOT(prefix_A) = PyValue_NULL`, then publish the receiver to B+1.
These actions precede the inner CALL prologue and `call_transaction_begin`, so
no debug window reader, child initialization, callback or later frame scan can
see stale non-value bits in B. Prefix guard misses occur before either acquisition
or clear; CALL misses materialize the method into the now-NULL B exactly once.
This is an explicit physical transition under owner-free overwrite authority,
not incoming readability or NULL inferred from logical absence. B+1 uses the
existing write-only canonical publication; the local receiver's acquisition is
still a DUP through the existing TAKE lowering.

The existing semantic image proves CALL consumption and later overlap/clear
geometry. Do not relax those constraints or enable scratch receiver/output
aliasing. Do not add a separate execution variant or per-hit fact lookup to omit
the clear for already-NULL sites in this first packet.

### Why the explicit clear is the useful first contract

The retained `resident-selected57-deltablue.json` supplies actual compiler words:

| Code | Method/CALL words | Callable home | Retained monitored visits |
| --- | --- | --- | ---: |
| Plan.execute | 4/5, CALL discard with zero explicit args | 4 | 364,800 |
| EqualityConstraint.execute | 1/2, zero explicit args | 1 | 306,000 |
| EqualityConstraint.execute | 4/5, zero explicit args | 2 | 306,000 |

These are structural opportunity upper bounds, not admitted or unmonitored hit
counts. Source replay establishes that a strict incoming-NULL requirement is
unnecessarily restrictive for these actual images. Equality's first pair follows
only RESUME and starts with positive NULL scratch facts. Plan's loop returns from
a discard CALL consuming homes 4/5: CALL has UNKNOWN normal transfer; selected57
`semantic_leaf_step` (`Objects/codeobject.c:2668`) preserves an existing SEM_NULL
only for PHYSICAL_PRESERVE, clearing it even for certified PHYSICAL_SET_NULL.
Its separate CallAbsence consumer retains NO_HOME. The loop fixed point therefore
cannot keep positive SEM_NULL at home 4. Equality's second callable home was the
first CALL's consumed self home and loses its NULL fact by the same rule. The
clear contract avoids expanding that older physical-proof consumer just to admit
this optimization. It still requires the positive final NO_HOME/no-owner facts;
the implementation's common replay nominee test must establish those for each site.

Physical NULL is needed before later ordinary readers, not as a condition on a
write-only acquisition interval. `_PyGC_VisitFrameStack` (`Python/gc.c:483`) uses
the initialized NULL-control scan for ordinary lifecycle states; fallback copies
can use the same control (`Python/frame.c:166`), and PopFrame asserts physical
NULL for departing cells. A stale owner-free header word is not safe scan input.
No callback or scan is allowed during the short prefix's acquisition-to-clear
interval. The one clear restores this physical invariant before any potential
reader; it does not grant any new normal/error/home certificate.

## Owners, callbacks and exits

| Point | Method owner | Receiver owner | Exit |
| --- | --- | --- | --- |
| Before prefix guards | none acquired | original local binding | Ordinary LOAD_ATTR on untouched inputs |
| Prefix complete | one canonical native owner | one snapshot in B+1 | No Python callback/poll/release permitted before materialization or child commit |
| CALL guard miss | moved once to B | B+1 unchanged | Ordinary CALL once at its original PC |
| Exact initialization | transferred to child `f_funcobj` | existing B+1 overlap becomes child argument | Same initializer, recipient and frame-entry protocol |
| Child code, result, error or observation | child/ordinary protocol | child/ordinary protocol | Existing CALL source/error/result/retirement paths |

There is no C-only owner across arbitrary Python. The admitted prefix only reads
guards and acquires owners. Its following guards are reads; stack reservation
and initialization must retain their actual no-Python/no-poll/nonfailing-after-
guards contract, including allocator/reference-tracer hooks. Before any supported
effect can occur, either the complete original CALL window has been materialized
or the complete child owns the method and receiver. Do not add EXTERNAL staging
or rely on a callee-saved register as a GC root. An unsupported new effect in that
interval must fail generation rather than inherit this permission.

The original ATTR and CALL PCs remain distinct. ATTR misses run ordinary ATTR.
After completing the prefix, inner CALL prologue attributes its guards to CALL;
any path that can observe or fail first materializes its complete original CALL
prestate. No native owner may be lost through a direct deopt/error label. Local,
scratch and discarded CALL results, child recursion/error, callbacks and redirects
then use the unchanged ordinary completion path.

## Two-site adaptive state and writer closure

The current prepared writer restores a head to immutable LOAD and carries its
certificate only when specializing the interior CALL. That is insufficient for
an adaptive ATTR prefix. Extend its generated mapping rather than adding another
raw writer or recovering nomination from adjacency on a hit.

Use three private states initially:

- certified generic-ATTR pending alias, whose dispatch entry is exactly ordinary
  LOAD_ATTR;
- certified METHOD_WITH_VALUES pending alias, whose dispatch entry is exactly
  ordinary LOAD_ATTR_METHOD_WITH_VALUES;
- active METHOD_WITH_VALUES + CALL_PY_EXACT_ARGS head.

Generate each state's ordinary-prefix handler, extent, semantic base/twin and
supported prefix/consumer transition. These are a finite set, not a Cartesian
product of all ATTR and CALL variants. All are private execution-only IDs; public
images contain ordinary words. No extra per-site payload or third cache is added.

On specialization at either covered site, `_PyIR_ReplaceHandler` captures the
existing certificate and the underlying prefix/consumer, defuses the head through
its generated ordinary-prefix mapping, applies the requested write, then selects
the supported active or pending state. METHOD-to-generic restoration preserves
nomination through the generic pending alias; unsupported ATTR specializations
drop it. CALL changes can retain the certified METHOD pending alias while waiting
for a supported exact member. Loss of a certificate cannot be repaired by guessing
adjacency. Declining uncommon adaptive transitions is acceptable and must be
counted as coverage, not silently generalized.

Instrumentation writes at either site revoke nomination and restore that site's
actual ordinary prefix, then install the requested wrapper. Wrapper saved identity
must never contain a private head. Preserve the existing FT specialization CAS;
nomination/composition remains GIL-only. Audit specialization success/backoff,
generic reset, line/instruction wrappers, displaced-opcode restoration, cold exec
initialization and every direct handler store. A raw mutable writer outside this
closure blocks admission. Debug LLTrace must tail-dispatch to the mapped ordinary
prefix before acquisition, preserving both semantic instruction displays.

## Ordered implementation and qualification

1. Extend and test the generated prefix mappings and two-site writer transitions,
   retaining ordinary prepared-LOAD behavior and public denial. Capture actual
   eligible per-home counts before claiming coverage from nominal pairs.
2. Implement the single typed callable binding, shared transfer marker, prefix
   owner capture and materializing miss exit. Add the active composition only
   after its actual emitted receipt and source-negative controls pass.
3. Root generates and builds both roles, then checks the complete native path,
   admission and runtime controls before the unchanged balanced hot-eight screen.

Production boundaries: `ir_generator.py` and `ir_metadata_generator.py` plus
pinned handler IDs; `bytecodes.c` for the shared callable-transfer marker;
`Objects/codeobject.c` for final per-home nomination; `Python/ir.c` for generated
two-site restoration/selection; existing specialization and instrumentation use
that central writer. `pycore_ir.h` exports only the necessary private mapping or
binding interfaces. No benchmark, allocator ABI, frame-layout or provider rewrite.

Source controls mutate guards, DUP/TAKE, canonical declaration, transfer marker,
CALL body, alias/address uses and all emitter exits. They must reject unsupported
composition while preserving ordinary analysis. Assert complete ordinary body
and receipt equivalence in both boxing modes, except the explicitly substituted
input. Include private mapping/effect truth and raw-writer census controls.

Runtime controls cover all public image routes, real managed-values Python methods,
thread and heap callers, B/B+1 live/unknown negatives and NO_HOME-only positives, independent
interior entry, receiver/local aliasing, method shadowing/materialized dict,
type mutation and function-code/default replacement, PEP523 and tracing changes,
stack/recursion misses, exact-result modes, errors and sole-owner lifetimes.
Enable LINE/INSTRUCTION and trace at either site after warmup; restore/deopt each
site in both orders and verify lookup/provider/acquisition occurs once. Use real
callable errors, finalizers and GC after materialization/child commit, not an
unsupported allocator-Python callback.

Native acceptance is the whole SELECT-to-child path: one method acquisition,
one receiver snapshot/store, unchanged ATTR and CALL guards, direct acquired
method-to-`f_funcobj`, no callable B publication/reload, exactly one owner-free B
clear before any reader, and no intermediate dispatch.
Charge next-word/feedback decoding, the heap fallback branch, all extra spills,
text growth and changes to ordinary CALL/ATTR handlers. There must be no hidden
lookup/descriptor/helper hop, duplicate provider or cleanup. Record actual
unmonitored admitted hits separately from census counts. Root decides selection;
native work removal alone is not a performance result.

The source packet enables only the method family. Final-local prepared handlers
are not emitted, their private numbers 370–374 remain reserved UNKNOWN entries,
and their cold nomination and candidate-specific fixtures are absent. Generic
composition infrastructure is reused without inheriting those native bodies.
Focused evidence is retained under `register-semantics59/method-call-source-host`;
the first CWriter scope assertion and its correction remain in separate logs.
