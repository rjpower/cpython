# IR and execution-image proposal

Status: historical M0 audit and implemented M1 admission design. The common
admission increment was peer approved, integrated and passed root's focused
release/debug checks. The audit below describes selected54
`61015a9452d9069646fb5fede9ff963896833b89`, not the current implementation.
See [progress](ir-progress.md) for integration history and
[semantic phase plan](semantic-phase-plan.md) for current follow-on work.

The existing admission proofs should become one construction boundary. Do not
introduce a parallel bytecode verifier or a compiler-trust flag. Keep NULL
homes, allocation and execution behavior through this increment.

## Existing authority and gaps

- `_PyCode_Validate` in `Objects/codeobject.c` checks constructor types,
  byte-size geometry, feedback-table geometry, hidden constants, source
  records, virtual kwargs and semantic completion/observer handlers. Assembly,
  the public constructor/replace path and marshal each call it.
- Only the public constructor then calls `check_words`. That checks explicit
  register and feedback fields and normalizes inline constant handlers when
  actual replacement constants change type. Compiler and marshal skip it.
  It does not validate full input/output extents, general branch targets,
  local field domains, constant/name indices or definite assignment.
- `_PyCode_New` trusts callers; `PyCode_NewEmpty` calls it directly. Frozen
  code uses marshal. `make_exec` copies the words and allocates feedback
  without another proof. The code pin already retains that allocation.
- `_PyRegCfg_Verify` proves virtual temporary entry availability, folded-local
  lifetime, contiguous windows, interference, exceptional suffix placement
  and survival below overlapping-call windows. Its liveness transfer and
  `_PyRegCfg_Compact` deliberately retain current stack/suffix constraints.
  It cannot alone admit public encoded images.
- `pycore_ir_outcomes.h` already supplies `_PyIR_ReadsHome`,
  `_PyIR_NullableHome`, `_PyIR_DefinesHome`, per-edge `_PyIR_Outcome`,
  `_PyIR_ObservedHomes` and `_PyIR_NormalPhysicalTransfer`. The last describes
  selected NULL/transport facts and returns UNKNOWN conservatively; UNKNOWN
  is not proof of initialized values or ownership. `owner_handler_safe` is
  already an encoded-image worklist proof, bounded to eight tracked homes.
- Generated `_PyIR_Formats`, `_PyIR_HandlerBase` and `_PyIR_HandlerFlags`
  already carry operand roles, family identity, output mapping, callbacks,
  observer facts and pristine/specialized/instrumented handler distinctions.

## First executable increment

Make `_PyCode_New` the admission boundary for every ordinary code allocation.
Keep `_PyCode_Validate` for structural and semantic checks, and promote
`check_words` into its common normalization/admission path. Validate structure
before decoding and normalization before completion/source proofs. The wrapper
holds a reference to normalized bytes, passes a copied constructor to the
existing allocation body, and leaves the caller's constructor untouched.
Remove redundant validation/check_words calls from assembly, marshal and
the public constructor. `PyCode_NewEmpty` now reaches the same boundary.

Extend encoded domain validation in this increment: full frame <=4095 homes;
locals/cells within localsplus; input 4095 forbidden; destination 4095 only
for generated discard/unpack roles; window read/write extents within scratch;
multi-output destination runs within their admitted region; all encoded
branch targets within the canonical word count; referenced constant/name
indices within actual tuples; feedback kind/size/operand requirements agree.
Preserve the documented all-zero padding adapter until its external users
are inventoried; do not silently treat arbitrary CACHE-bearing code as proved
executable. No new format/magic is needed for stricter admission with unchanged
serialized roles. New descriptor schema and LOAD_POOL/RP12 remain later work.

Files: `Objects/codeobject.c`, `Python/assemble.c`, `Python/marshal.c`,
`Lib/test/test_ir.py` or a focused `test_ir_admission.py`. No shared frame ABI
or generated/header edits in this first increment. Root owns generation,
release/debug execution and source/native review.

Tests must distinguish constructor/replace from marshal admission by patching
a serialized valid code payload into an invalid operand/window/branch while
preserving record lengths; never execute malformed code. Check valid compiled
calls, loops, completion, exceptions, hidden constants and mutable inline
constants still round-trip. Retain existing semantic completion errors.

## Frozen follow-on interfaces

Use a code-owned immutable descriptor, and borrow it from `_PyCodeExec` under
the existing code pin. SiteId is uint32 canonical word index; PhaseId uint8
uses PRE=0, POST=1, ERROR=2, RETIRING=3, SUSPENDED=4, family staged IDs >=5.
Locals are separately initialized. Scratch initialized/authorized-owner sets
use interned sorted half-open uint16 ranges. Each site records fixed phase
offsets, transfer family, successors and exception/observer recipes. Do not
allocate this schema until its validator and first consumer land together.

Extend existing outcome/read/definition APIs into an explicit semantic edge
transfer contract, keeping physical NULL facts separate. Each edge needs
consumed/borrowed/duplicated/moved inputs, initialized/nullable outputs,
release obligations, call-clobber extent, callback capability and PRE/STAGED/
POST failure authority. The same generated family contract must authorize
specialization, canonical restore and instrumentation; only low16 changes.
LOAD_ATTR method output must normalize callable plus self-or-NULL. SEND/FOR
need separate success/exhaustion/error edges. Dynamic unpack needs a validated
output window plus source/capacity/produced_count prefix recipe, not one phase
per item or the eight-root retirement batch.

General definite assignment should extend the encoded worklist proof using
these transfers; entry locals/hidden constants and permitted nullable inputs
are distinct from newly produced scratch. Joins intersect initialized facts;
owner obligations use explicit edge transfers. Exceptional/observer edges
need their own certified maps. Call overlap excludes transferred windows and
keeps caller survivors below the child's entire clobber region. Do not claim
M1 full semantic-image proof until these edges and fixed sentinels are covered.

Fixed sentinels requiring bounded independent proofs are InitCleanup (two
words/two scratch homes), interpreter-entry trampoline and PyCode_NewEmpty.
There is no universal sentinel bypass. The first two need the frames lane's
typed sentinel-kind contract before `_PyCodeExec` changes.

## Gate and dependencies

Peer review must freeze the common boundary and reject any normalization that
can run before basic tuple/word/feedback checks. Root should test the first
increment before widening definite-assignment semantics, because the selected
generator intentionally elides PUSH_NULL under NULL authority. M2/M4 cannot
inherit that elision as an initialized-value proof. The frames and GC lanes
must agree on phases, prefix root enumeration and detached owners before any
map becomes authoritative. Optional pool constants and non-stack allocation
must not be bundled into admission hardening.
