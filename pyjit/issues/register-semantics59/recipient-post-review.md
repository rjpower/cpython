# Continuation recipient: source and native post-review

Source GO, with costs retained for root's performance gate. This review covers
G `c1da955989eefcb11023c181c5d2e712bb64e504` against selected57
`0dcba75bb3df40c32137db453b001f7e8f101de0`. It does not qualify native behavior
or recommend selection. No interpreter, host test, generator or build ran for
this review; only source inspection, `objdump`, `nm` and identity checks.

## Ownership and complete source closure

The existing 16-bit field at offset 104 now stores action plus 12-bit home.
The fixed header remains 112 bytes, with phase at 110. INVALID is zero;
DISCARD has exactly `0x3fff`; EXTERNAL is restricted to the initialized native
entry home 1. Ordinary frame/thread constructors start INVALID, created
generators explicitly bind DISCARD, and trampoline/child-call producers bind
before entry. Header copying carries the encoded field.

The producer classifies LOCAL versus SCRATCH once. Generic binding still reads
the caller's code/local partition; it does not get that fact for free.
`enter_generator` uses the no-query scratch binder only for SEND with the
actual checked entry recipe. Generic/instrumented SEND retains generic
binding. FOR binds its actual A. The declared source and all generator body
templates were ported together: generated source changes are 15 binding sites,
without changing acquisition, input vacancy, child linkage or continuation PC.

Publication loads the action after `_PyFrame_CompleteRedirect`. LOCAL reads
the displaced owner, publishes the result and only then closes the old owner.
SCRATCH/EXTERNAL publish without reading old bits. DISCARD closes the result;
noncanonical DISCARD and INVALID/reserved actions fail rather than derive an
address. Action does not supply canonicality, incoming readability, ownership,
or a home-map proof. Pending redirect still requires exact DISCARD; callback
cleanup can change the continuation before the action is read.

The code/partition checks inside the publisher are assertions, compiled out of
the audited release paths. Cold local/remote readers use the strict shared
layout decoder before addressing a home; delegated-yield readers additionally
match the immutable instruction's actual destination and distinct receiver.
Remote offsets retain their shape but advance ABI version to 2. Runtime and
generator source searches found no remaining `return_slot` field references
under Include/Python/Objects/Modules/Tools/cases_generator. The exact DSL binder
is registered as nonescaping; it grants no source error/home recipe.

Metadata, target IDs, formats and uop products are byte-unchanged from selected57;
only `Python/generated_ir_cases.c.h` changes among generated products. Thus the
new encoding is a continuation action, not a new semantic operand authority.
The new local-yield fixture needs its separate source-shape correction: an
ordinary compiled assignment yields to scratch then STORE_FAST, so that shape
cannot itself establish LOCAL recipient coverage. Frames owns that fixture;
this does not identify a production defect.

## Actual release code price

Disassembly and symbol sizes are retained in
`E59/recipient1-native-review/`. Candidate ELF:
`BASE/reg59-results-release/python`, SHA256
`51bd3187305bd3fe059f0f9e914d6edebd34eafe831ea28c63406a8f2f84cd7d`.
Selected57 control is the retained `E55/comp57-dictionary-native/python`, SHA256
`db38aeb4ab220e89d0b32a213fa69a8f51c397e8dd47736ec555c8a81a487fe2`.
Hashes were checked again after capture. Sizes below include cold branches and
padding within each symbol; they are not dynamic instruction counts.

| Handler | Selected bytes → candidate | Stack reservation, selected → candidate |
| --- | --- | --- |
| `CALL_NO_SELF_PY_EXACT_ARGS` | 871 → 903 (+32) | 64 → 64 bytes |
| `SEND_GEN` | 776 → 830 (+54) | none → none; both retain frame-pointer push/pop |
| `RETURN_VALUE_EMPTY` | 780 → 743 (−37) | 16 → 16 bytes |
| `YIELD_VALUE` | 525 → 561 (+36) | 16 → 16 bytes |

The four symbols grow by 85 bytes in total. This is a bounded sample, not total
interpreter text growth. No new helper call or larger stack reservation was
introduced on these common paths. Existing cold deallocation/redirect spills
remain; their register names and block arrangement can differ.

`CALL_NO_SELF_PY_EXACT_ARGS` binds at candidate `0x388260–0x38829b`.
The non-DISCARD binding executes 12 binding-related instructions, versus three
at selected `0x387fa0`, `0x387fb4`, `0x387fbd`: load A, check sentinel,
load code, compare `co_nlocalsplus`, choose tag with `cmov`, combine and store.
The selected sequence only loads/masks/stores the home. The candidate adds an
actual sentinel branch and code/partition read; the surrounding child/header
construction and 64-byte frame remain. Do not count classification as removed
from the complete call/return path.

`SEND_GEN` binds its parent scratch action at `0x397029–0x39703e`, with no
code/local-partition query: it reuses B+2 and sets the scratch tag. Resuming the
child still decodes that child's independently armed action at
`0x397096–0x3970dd`; the known parent scratch recipe cannot specialize this
different recipient.

`RETURN_VALUE_EMPTY` decodes at `0x398c30–0x398c71`, after frame cleanup and the
pending-redirect branch. `YIELD_VALUE` first binds its own next resume action at
`0x34e840–0x34e873`, then decodes the parent's action at
`0x34e8cf–0x34e908`. Their release publication paths contain no executable
frame-owner/code/nlocals partition classification. The price is shift/mask
and action comparisons: scratch reaches its store after three action branches,
local after four. Selected publication reaches its scratch/local decision
after sentinel, owner and partition branches. DISCARD now validates its exact
encoded sentinel; EXTERNAL and invalid actions have explicit cold branches.
The candidate's fatal branch is real code even though valid producers cannot
take it. Fewer layout loads is not evidence of fewer executed instructions or
faster whole workloads.

## Follow-ups, not blockers

Keep the shared strict cold decoder and producer-only known-scratch exception;
do not make release publication recheck layout. The native source shapes offer
no justification for adding another recipient cache or eagerly classifying all
destinations. Selection needs root's complete owner/observer/remote gates and
timing, including the charged ordinary bind. No production correctness blocker
was found in this read.

For the next grouped resident/backend review, the full native provider boundary
must be priced against upstream. The retained 108 dict.get method-provider
calls plus 162 direct LIST_APPEND helper operations per tray, times a
hypothetical 20 instructions, is neither a measured saving nor an upper
bound: value-array preparation, argument publication, provider adapters and
return normalization can cost much more. Measure the complete path at actual
boundary frequency, alongside resident and dispatch costs, without overlapping
their budgets. This is a handoff requirement, not a new micro-optimization lane
or a reason to reinstate the rejected checked-native-entry experiment.
