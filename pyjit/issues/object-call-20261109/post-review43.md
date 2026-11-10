# Candidate 43 compiled frame-commit review

**Approve bounded timing after root's focused checks pass.** Candidate 43 removes
the unconditional native frame-construction/header-initialization boundary at
the three targeted successful overlap sites. A conditional constant-fill leaf
remains; this is an acceptable measured limit, not complete inlining of all
frame work. No third annotation is proposed or approved. This review supplies
no throughput result or promotion recommendation.

Inspected runtime `47dc23f48cf`, retained 41 `0a35a382909` and archived 42
`478e7978845`. The 43 diff adds only `Py_ALWAYS_INLINE` on `_PyFrame_Initialize`,
retaining the outer annotation. Root reports 117 checked modules on both 43
builds; its focused tests are separate executed evidence. This review ran
read-only source/ELF inspection and lightweight host parsing, with no runtime
edits, builds, native workloads or commits.

## Compiled boundary and conditional leaf

ELFs are below
`/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/`:

- **41:** `pyjit-link-wave1/object-call-20261109/gap41-perf-record/candidate-python`;
- **42:** `pyjit-link-wave1/object-call-20261109/structural42/release-python`;
- **43:** `execution-boundaries-release/python`.

`nm`, `objdump` and `size` confirm no PushOverlapping or Frame_Initialize
standalone symbol in 43. Direct inspection of the three bodies finds storage
reservation/frontier updates, header/code-owner initialization and execution
pointer installation inline. The 43 native calls that remain specific to these
steps are `_PyFrame_FillConstSlots`, size 0x70 at `0x33cb00`, at:

| Caller | Constant-count branch | Conditional leaf call |
| --- | --- | --- |
| Positional general | `0x37f7f9` load, test, jne | `0x37f98a` |
| Generic overlap | `0x34739e` compare, jne | `0x347458` |
| Keyword Python | `0x381340` compare, jne | `0x381449` |

Every call is reachable only when `co_nconstslots != 0`. Zero-constant frames
skip it. The leaf has no native push/pop, stack reservation, stack protector,
allocation or further calls. It loads the constant table and source tuple,
duplicates each canonical constant owner, and stores into the selected home.
Its bit/null checks preserve mortal-object increfs; inline/immortal constants
do not gain a count. It returns without retiring an arbitrary owner or calling
Python.

For nonzero constant counts, the native call/return, caller argument handling
and spills, and the leaf's repeated count load/test remain real costs. They
must not be called cold merely because they are conditional. Their actual
application frequency is unmeasured here. This is a bounded 112-byte ownership
loop rather than another outlined complete frame commit. Measuring 43 with this
limit is preferable to automatically extending inline policy again.

## Size, stacks and misses

| Path | 41 bytes | 42 bytes | 43 bytes | 41→42→43 stack reserve |
| --- | ---: | ---: | ---: | --- |
| Positional general | 0x3ff | 0x4f3 | 0x5bf | 0x50→0x80→0x90 |
| Generic overlap | 0x600 | 0x6be | 0x757 | 0x48→0x58→0x58 |
| Keyword Python | 0x5f8 | 0x6d3 | 0x76e | 0x80→0xa0→0xa0 |

Static rbp-relative references respectively change 42→43 as 77→92, 94→99 and
81→85. These counts cover whole bodies and cold arms, including newly inlined
work; they are not dynamic spill counts. Neither positional general nor generic
overlap has a stack protector. Keyword retains its existing one TLS load/four
exit checks; no extra protector appears.

The generic-overlap generator-owner early miss stays 13 instructions. Its owner
compare/branch is still at entry +0x1d/+0x21, before the first native push at
+0x27. The return island is now +0x200 and uses another scratch register; no
stack reservation or register save executes on this miss. The two annotations
have preserved its shrink-wrapping.

Text is 7,527,899 bytes: +312 versus 42 and +744 versus 41. Data/bss remain 975,152
and 298,104. The complete experiment therefore increases caller footprint and
stack reservations while removing the ordinary constructor boundary. Whether
that tradeoff improves cycles is the timing question, not a static conclusion.

## Guards, owners and broader consumers

The source diff changes declarations only. Compiled general/keyword bodies
retain pre-effect function/provider/version/recursion, stack/executable,
keyword count/self/name/destination and current-default guards. Conditional
empty kwargs allocation remains before callable detach. Keyword owners and
names detach before overlapping constant/header writes. Binding/default
installation and existing return/current-frame publication follow initialized
construction. No new callback, fallible step or post-detach fallback is added.
The constant leaf preserves its independent-owner duplication contract.

Both exact-call owner arms were inspected. Thread-owned entry still overlaps;
generator-owned entry still uses unchecked non-overlapping storage, copies
argument owners and NULLs their caller homes. Neither exact handler calls a
native frame initializer or the new constant leaf. They retain 339 and 500
static instructions and 42's 0x582/0x86a bytes, stack reserves 0x50/0x60 and no
canary. Register/home choices and branch placement vary, so equal counts are
not an assertion of identical code or latency.

The wider unchecked consumers also retain inline initializer/constant work,
without a substituted native frame helper:

| Consumer | 41→43 static instructions | Stack/protector observation |
| --- | ---: | --- |
| Python getitem entry | 289→292 | 0x80 unchanged; no protector |
| Property getter entry | 256→255 | 0x60 unchanged; no protector |
| Overridden getattribute entry | 303→314 | 0x70 unchanged; no protector; rbp references 34→43 |
| Generic `eval_frame_push_and_init_code` | 1,764→1,766 | 0xc8 base reservation and existing protector retained |
| Direct heap `PyFrame_New` | 238→238 | Entire normalized body unchanged; 0x68 reservation retained |

The custom-getattribute codegen growth is a control to watch, not assumed free.
The generic provider still prepares storage, initializes the child, consumes
binding and performs established partial-child cleanup. The heap initializer's
NULL fill before owner/header/constant initialization is unchanged, as is its
entire containing emitted body. No escape/generator move gained a preliminary
initializer. Existing mortal-constant and owner tests remain needed; annotation
alone does not prove runtime behavior on every branch.

## Coroutine profile and measurement gate

The corrected 41 profile identifies exact Python calls, generator creation,
await conversion, generator return and SEND_GEN as its sampled costs. These
paths must not be relabeled as positional-general fallback. RETURN_GENERATOR,
make_gen, GET_AWAITABLE, CompleteGeneratorReturn and SEND_GEN retain the same
normalized instruction lists across 41/42/43 (223, 51, 312, 443 and 170 instructions).
Exact-call codegen changes are recorded above. Neither fact predicts coroutine
recovery or rules out placement/front-end effects.

Root's release/debug focused 273-check results are still the behavior gate for 43.
After those pass, compare balanced logging/keyword timings and raw instructions
with frozen 41 and golden 39, keeping coroutine/DeltaBlue as required controls.
Inspect measured work, elapsed time and code footprint together. Preserve 42 as
the structurally incomplete first experiment and 43 as the bounded extension.
Retain 43 only on repeated application evidence, without hiding residual constant-
leaf costs or assuming source annotations constitute a win.
