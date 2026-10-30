# H2 first source slice: inert scalar effects

Independently reviewed source checkpoint from `5a45f9bb6a73`. No native build, target
test, runtime map activation or performance claim. No persisted frame/code
layout, clearing, register reuse or constructor acceptance changes.

This is the coordinator-approved split before the full final-word CFG and
physical-liveness producer. It supplies DSL-generated scalar declarations
and an allocation-free status-returning decoder in the existing IR module.
`OK` means only scalar declared input slots/new output span were decoded;
it is **not** a valid code/CFG/map verdict. `UNSUPPORTED` carries explicit
exception/control/frame/window/stack-shape requirements and no usable sets.
`INVALID` covers nonpristine/hole handlers and invalid physical slot/span
bounds. No runtime constructor or root reader calls the partial decoder.

Effects derive from existing analyzer properties and external stack I/O;
there is no hand-maintained opcode whitelist. Family inputs align at the
top of the family format (END_FOR differs from its instrumented twin).
Scalar output ranges are bounded through their final destination; repeated
physical input addresses are permitted and a later bitset builder unions
them. Fatal trap bodies are unresolved control, not empty fallthrough.

The frozen source has 153 pristine handlers: **27 supported, 126 unresolved**.
Supported names:

```
COPY_FREE_VARS END_FOR END_SEND IS_OP LOAD_COMMON_CONSTANT LOAD_CONST
LOAD_CONST_INLINE_FLOAT LOAD_CONST_INLINE_INT LOAD_FAST LOAD_FAST_AND_CLEAR
LOAD_FAST_BORROW LOAD_FAST_BORROW_LOAD_FAST_BORROW LOAD_FAST_LOAD_FAST
LOAD_SMALL_INT MATCH_MAPPING MATCH_SEQUENCE NOP NOT_TAKEN POP_EXCEPT
POP_ITER POP_TOP PUSH_NULL SET_FUNCTION_ATTRIBUTE STORE_DEREF STORE_FAST
STORE_FAST_LOAD_FAST STORE_FAST_STORE_FAST
```

The complete generated-name/reason inventory is retained as task evidence
`build/H2-root-effects/coverage.json`; reasons overlap. The remaining classes
include normal/exception transfer and initialized output prefixes, conditional
and unconditional branch/relaxed SEND/FOR_ITER edges, suspend/resume/call
ownership transitions, and variable windows. They need explicit generated
semantics before successful decoding, CFG construction or fixed-point maps.
The decoder deliberately does not inspect FBINIT, exception tables, local
opargs, constants or constructor normalization; those are the next full
producer's inputs, not omitted checks hidden behind this scalar status.

Added controls in existing test_generated_cases/test_ir cover scalar shape,
family alignment, output spans, aliasing, invalid slot boundaries and named
unresolved complex edges. Source-review follow-up adds actual in-domain hole,
instrumented/state-dependent specialization rejection, zero-slot NOP versus
input, and exact last input/output and two-output span boundaries. The
`_testinternalcapi` adapter reports POD decoder
results; it exposes no raw addresses or GC root fixture. Its C and target
Python tests remain uncompiled/unexecuted.

Host metadata generation passes. Four newly added generator test methods
pass when extracted unchanged with their existing analysis helpers and run
under host Python; that avoids importing this fork's `lazy import` stdlib
syntax under host Python3.14. The direct host module import failed before
tests; an initial copied-control run failed on missing newlines around DSL
markers, corrected in the new test input. That failed log is retained; the
initial import limitation is recorded here and in the tool transcript. No
target execution is inferred. The final four-method host rerun and identical
regeneration pass; source analysis confirms holes125–127 and the chosen
state-dependent handlers' lack of PRISTINE admission. `git diff --check`
passes. Native semantic assertions remain outstanding.

Independent review `reviews/H2-root-effects-source-review.md` in primary is
SOURCE_CHECKPOINT_CLEAR for this inert temporary-only slice. Coherent GCC O3
and O1 Py_DEBUG build/affected-test preparation follows; execution needs a
separate coordinator grant and resource slot. Full decoder/CFG/map generation must use final checked words,
FBINIT and exception/layout inputs, compare independent projected virtual
liveness, then eventually produce planned deduplicated full physical
bitsets/direct indices. Maps remain inert until note14's reader/span and
construction/lifetime contracts independently close. Queue2/S3 and unrelated
generations work are outside this patch.
