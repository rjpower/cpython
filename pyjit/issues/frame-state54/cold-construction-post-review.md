# Cold public construction: independent source/native review

Source/native GO for the correctness extraction, not selected admission or a
performance claim. Matching clean source is
`61015a9452d9069646fb5fede9ff963896833b89` in
`BASE/worktrees/frame54-cold-construction`. Its diff from selected51 contains
only Objects/genobject.c, Modules/_testcapi/frame.c and
Lib/test/test_capi/test_frame.py. The source ownership review is in
[cold-construction-post-source-review.md](cold-construction-post-source-review.md).
Both `cold-construction-initial-{release,debug}-build.log` finish with117
modules checked,79 shared and zero failed imports. Reviewer operations were
source reads, nm/readelf/objdump and non-running GDB type information. No
inferior, generator, compiler, runtime test or timing was run by this reviewer.

| Matching ELF | SHA256 | Build ID |
| --- | --- | --- |
| BASE/frame54-cold-construction-release/python |2cce200e41cef134c84f98af5693d8430545f91e158f39b9763a8f7636877d37 |6db3409d8371774d9bbac983531e01bad60ed4fa |
| BASE/frame54-cold-construction-debug/python |773b4f299a6fb963ea7f1b1ff2bb5f9034d2e3f1d0b0cc27562114520802b0d5 |d7678d48e42fbfb00c0c9111a6a16df5a8e18e4e |

## Actual layout and preserved hot bodies

GDB confirms both GIL roles retain fixed header112 bytes, sizeof120 including
the declared trailing home and homes at112. There is no NONE/tag, new field,
layout, common-copy, hot publication or generated-handler source change.

The following normalized instruction sequences match selected51 in BOTH
release and debug. Comparison removes alignment nops and address/RIP/branch
relocations while retaining instructions, immediates, registers, memory
operands and called symbol names. This is not literal byte identity or a
claim about executed frequency. Counts are static non-padding instructions.

| Body | Release selected/candidate count | Debug selected/candidate count | Candidate release text bytes |
| --- | ---: | ---: | ---: |
| CALL_PY_EXACT_ARGS |224 /224 |423 /423 |973 |
| SEND |577 /577 |997 /997 |2955 |
| SEND_GEN |166 /166 |203 /203 |743 |
| YIELD_VALUE |101 /101 |383 /383 |453 |
| RETURN_VALUE |121 /121 |396 /396 |546 |
| RETURN_GENERATOR |213 /213 |450 /450 |1002 |
| normal _Py_MakeCoro |87 /87 |167 /167 |340 |

Exact CALL at `0x37a460` retains its80-byte rsp local reserve, the original
packed slot-zero/kwargs default0x0fff0000 at `0x37a662`, and the original
raw real-recipient store at `0x37a6a9`. It does not acquire R/R2 encoding or
S/S2 variant work. The normal _Py_MakeCoro path at `0x170850` remains340
bytes release and725 bytes debug, unchanged in both instruction sequence
and size. This cold extraction does not route ordinary creation through the
public constructor helper.

## Actual cold owner and origin commit

Release gen_new_with_qualname begins `0x1709b0`, size820 bytes versus baseline
669. It retains the same four saved GPRs (rbx,r12,r13,r14) and16-byte rsp
local reserve. Its static non-padding sequence grows157 to199 instructions.
Debug grows1697 to1897 text bytes (331 to382 non-padding instructions), adds
a saved r15 and has a24-byte rsp local reserve. These are cold code/stack
changes, not overhead on the unchanged normal _Py_MakeCoro path.

The release subtype allocation is at `0x1709ef`. Coroutine-origin creation
at `0x170c12` precedes source mutation and frame move. A NULL origin calls
GC_Del at `0x170c2a`, then consumes the stolen input f on the failure route
starting `0x170c30`; it never invokes generator teardown over uninitialized
data. Successful origin rejoins before source mutation.

The source frame receives raw IR_NO_DST at `0x170a1c`, before the copied
header and moved homes. Thus the first permitted None input follows baseline
SetReturnValue's discard-before-index contract; there is no invented slot-zero
owner. Homes move and source homes clear at `0x170ae0`–`0x170afb`.
CREATED is set at `0x170afd`; f's owning reference is acquired before
frame_obj store `0x170b15`, and GENERATOR ownership is set at `0x170b1c`.
Weakrefs, exception linkage, origin/finalizer and async flags are initialized
at `0x170b23`–`0x170b37`, followed by owning names. Association is published
at `0x170b81`, GC tracking follows, then f's stolen input owner is released
at `0x170bc8` onward. The source frame lock covers the nonfallible commit;
these GIL ELFs do not establish free-threaded machine ordering.

PyCoro_New and PyAsyncGen_New are25-byte tail wrappers to the common helper,
versus144/54 bytes in selected51. No second origin allocation or later
subtype initialization remains. Direct f_locals ownership and escaped-frame
takeover remain baseline protocols. The three direct tests cover association,
first input, namespace lifetime, coroutine origin and async-generator hooks/
asend/close, as source-reviewed earlier.

Root owns execution of the focused/new tests, failure injection and full
qualification on this actual head. Original R2 evidence must not be relabeled
as this extraction. Exact native placement and alignment differ from
selected51 despite the preserved hot sequences; this review does not infer
elapsed equivalence, full-suite success, free-threaded qualification or selected
admission. The correction is justified by the reproduced baseline C API
association/first-input defects, independently of rejected performance lanes.
