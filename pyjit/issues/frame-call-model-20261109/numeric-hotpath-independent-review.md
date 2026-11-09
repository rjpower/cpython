# Numeric publication: independent source and compiled review

Read-only, 2026-11-09. No runtime edits, builds, native tests or performance
captures. The first `6ef45e391aa` screen reports nbody/golden 1.05559,
float 1.02804 and deepcopy_memo 1.092. The later arena/completion image has not
been measured. These ratios do not identify publication as their cause.

Source comparison uses immutable golden `0765e9be47f` and the active source.
Read-only disassembly inspected the current release ELF, build ID
`45b7aa08949016ad4f867c728ec164a77b9218d2`, and golden ELF build ID
`61fbf1bb863cc2c72050228cc7704979a7779589`. The current ELF is integration
evidence, not the sealed image underlying the earlier timing. Recommendations
wait for the next matched broad screen and actual warmed handler/destination
census.

## 1. Put the local destination before the sink test

This is the smallest structural experiment. Current IR_STORE_DST tests
F==IR_NO_DST before IR_IS_LOCAL(F); golden tests the local class first and has
no sink. All valid local indices are below co_nlocalsplus, which is bounded by
the slot limit. IR_NO_DST is 4095 and cannot satisfy the local predicate.
The equivalent order is local, then discard, then ordinary temporary.

The local branch can retain its exact body: load the current displaced owner,
publish the new result, check the home, then IR_RETIRE_LOCAL. No destination
address is formed for the sentinel. Input/destination aliases and temporary
NULL/same-result checks are unchanged. The result remains owned until either
the selected home or discard consumes it; error paths before final storage
remain unchanged.

On an ordinary local-result route, this removes one sink conditional branch
and its supporting test work. The current SUBTRACT_FLOAT ELF has destination
extraction followed at symbol-relative +0x1d6 by mov/not/and/je for the sentinel,
before its local comparison. Golden has a direct local comparison without
that guard. Exact savings depend on GCC allocation and layout; whole-function
size is not a dynamic saving. Used temporaries still need both class tests,
and discarded results acquire a preceding local test. Inspect all three routes
after compilation, rather than counting every result as a win.

IR_STORE_TEMP_DST already uses short-circuit !IR_IS_LOCAL(F) followed by
F!=IR_NO_DST, so the local route skips its sink test today. It is not the same
regression. Keep that order and preserve its early temporary publication.

Minimal gates: local/temp/discard destinations; local destination aliasing each
input; a displaced last mortal owner reentering Python; a discarded mortal
result released once; and an operation failing before publication. Existing
reader/finalizer tests should remain unchanged. Compare resulting code and the
affected broad workloads, not only a numeric microbenchmark.

## 2. Carry a checked canonical numeric producer fact to final storage

The numeric constructor chain supplies a stronger fact than a handler name:

- PyValue_FromDouble returns inline non-NaN bits, or calls _PyValue_BoxDouble
  only for NaN. The latter asserts NaN and creates an exact float, the required
  boxed representation for that value. Failure returns NULL.
- PyValue_FromInt64 checks the inline range; _PyValue_BoxInt64 asserts the
  complement and creates an exact long outside that range.
- _PyValue_FromIntScaled and PyValue_FromInlineInt construct inline bits.

The four specialized float arithmetic bodies use the first chain on every
successful result assignment. ADD_INT/SUBTRACT_INT use the scaled/boxed pair.
These are bounded initial candidates for a checked canonical-result intrinsic
or generator fact. Integer multiplication's span-output provider and generic
user/native numeric results need their own producer audit before admission.

Current _PyValue_Publish tests object form and exact long/float types before
canonicalization. GCC can fold part of that for a known inline fast result;
it may retain classification and a cold canonicalization path because a
constructor can also return a boxed value. The reviewed current float ELF
retains pointer/type tests and a canonicalization call path. Do not credit the
whole cold path as hot instruction savings. A final canonical store can remove
only the classification work actually executed on the measured route.

The fact must survive unchanged result assignments and the complete uop chain,
not propagate from FLOAT/INT in an opcode name. Fail closed on producer
replacement, conditional raw-result assignment, alias escape, reassignment,
unknown suffix effects or a different output. Preserve allocation failure,
deopt, input retirement and displaced-local retirement. Keep the sink branch;
canonical bits do not make a discarded owner immortal.

Minimal gates: finite floats, signed zero, infinities and boxed NaN; int inline
boundaries and boxed overflow; mixed int/float deopt; local/temp/discard and
input aliasing; failure from boxed allocation where available. Add generator
mutation negatives comparable to canonical_return_input. This experiment is
larger than branch reordering and should follow its separate measurement.

## 3. Extend the checked publication protocol where a real duplicate exists

Whole-source in-memory generation found one final IR_STORE_DST for
BINARY_OP_SUBTRACT_FLOAT and ADD_INT. There is no pre-retirement
IR_STORE_TEMP_DST in those generated bodies. Calling their final store a
double publication would be incorrect.

UNARY_NEGATIVE, by contrast, emits IR_STORE_TEMP_DST before releasing its input
and IR_STORE_DST at completion. The first publication canonicalizes the C
result and makes a temporary owner visible during retirement; the second
repeats publication and the store for an ordinary temporary. The local path
intentionally waits until retirement succeeds and must still replace the
current local. A sink intentionally has no published temporary and must close
its C result exactly once on success or failure.

A useful follow-up is an explicit checked retirement/publication contract,
sharing the existing published_temporary_result mechanism used by reviewed
attribute readers. Prove the result and home are unchanged through the suffix,
input consumption, callbacks and refresh. Only then omit the final temporary
publication. Do not generalize the fact to every flush: a finalizer can redirect
or change owner state, and generic __neg__ can return arbitrary values. The
existing reader protocol supplies a model, not an automatic proof for unary
or generic arithmetic.

Likely work removed on admitted temporary routes is the final destination
classification, canonicalization predicate and duplicate home store. Local and
discard routes retain their completion behavior. Priority depends on actual
UNARY_NEGATIVE/generic-reader frequency; no evidence here makes it a cause of
the measured broad losses.

Minimal gates: custom __neg__ result, last-owner input finalizer reentry,
callback redirect/error, local replacement, temporary result and discard;
generator negatives for result reassignment, escape and suffix overlap.

## Avoided shortcuts

Do not remove local old-owner retirement because the new value is numeric:
the displaced owner may be arbitrary, including after debugger/local mutation.
Do not infer raw native-result canonicality from its C type or operation name.
No frame-owned sink bank, extra opcode aliases or runtime ownership map is
needed for these three experiments. Current canonical window moves and
tuple/list unpack already have separate proofs; repeating their optimization
cannot explain a remaining loss.

The source-level order of recommendations is therefore branch reordering,
bounded numeric producer provenance, then measured duplicate-publication cases.
The coordinator should choose only after the current integration's functional
gate and matched timings; this review makes no speedup or parity claim.

## Admitted minimal experiment after second timing

Coordinator source20 measurement is separate from this source change:
logging/upstream .97449 over two pairs; nbody/golden 1.0749, repeated over four
rounds at 1.08037. Its twelve valid PMU samples show instruction ratio1.03194,
cycles1.08001, branches1.05608 and misses1.00010. These motivate measurement;
they do not identify this macro as the cause. Warmed static destination counts
(advance6 local/33 temporary arithmetic destinations; report_energy5/15) are
site counts, not dynamic frequencies.

The implemented experiment changes only Python/ceval_macros.h IR_STORE_DST:
check IR_IS_LOCAL(F) first, then IR_NO_DST, then the ordinary temporary route.
The local, discard and temporary bodies are byte-for-byte unchanged. Existing
slot bounds ensure locals are below the 4095 sentinel, so the three categories
remain disjoint. Publication, old-local retirement, input alias handling and
last-owner callback behavior keep the same route-specific order. No change to
IR_STORE_TEMP_DST or provenance rules. No benchmark-specific selection.

Source diff review and git diff --check pass; no worker native test, build or
regeneration. B post-review and coordinator focused numeric/local/temp/discard/
reentry validation plus paired nbody measurement remain required before accepting
the experiment. This is not a performance result.
