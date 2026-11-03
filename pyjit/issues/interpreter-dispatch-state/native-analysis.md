# Slot-base prototype: author native analysis

2026-11-01, bounded host-only analysis begun at 00:20 UTC. This is analysis by
the prototype author, not independent acceptance. No compiler, target, workload,
counter or timing invocation was made. Source `3ddc437679ebbb9f8629c1670655c58776075dfc`
and its isolated worktree remain unchanged.

The intended outgoing slot-cache transport disappears on all three nominated
normal transitions. The matched candidate also adds repeated frame/IP memory
traffic at the sampled common entries. Both observations matter: removing the
specific pair does not establish a net reduction in executed work or elapsed
time. No elapsed result for this prototype exists in this analysis.

## Images and scope

The frozen emission receipt is
`build/worktrees/dispatch-slot-base/build/dispatch-slot-base-feedback/emission.result.json`.
Its eighteen disassemblies include the complete evaluator for each image and
eight entry prefixes per image. Each image has one evaluator symbol and no
separate evaluator partition in the retained symbol inventory. The full texts
are preserved; branch following below uses those texts, rather than assuming
an entry prefix contains its whole handler.

| Image | Native SHA-256 | Evaluator bytes |
| --- | --- | ---: |
| Accepted e9 parent | `289cf16e3baf69e7cbc364a9b40a46f713e6604ae1fa8e6bf94afad7857f586f` | 142,471 |
| Slot-base 3dd candidate | `d3d9843ee1a6d441f22d753d96d1d3224c9fa0799f14503f4d71f3fd792ef035` | 157,884 |

I rehashed the images and all retained emission captures. Both worktrees are
clean at their bound revisions. The Makefiles match after replacing only their
source/build paths; pyconfig.h matches exactly. The compiler-selected toolchain
records match, and their current files and resolved paths match their hashes.
The 10.82% evaluator text growth is diagnostic; it is neither a performance
verdict nor evidence of an instruction-cache cause.

## Three outgoing transitions

| Consumer | Parent transport before dispatch | Candidate continuation |
| --- | --- | --- |
| YIELD_VALUE | `457acb` loads resumed frame base; `457ad2` stores it to `-0x628(%rbp)`; dispatch `457af0` | `44fe7b` stores resumed frame to `-0x630`; `44fe85` loads its IP; `44fe98` stores next IP to `-0x628`; dispatch `44fea3`. There is no resumed-base cache load/store here. |
| RETURN_VALUE, ordinary caller | After FrameClearAndPop, `452bf3` loads resumed base and `452bf8` stores its cache; dispatch `452c19` | No-injection branch `451b9b → 465453` calls FrameClearAndPop at `46545a`, rejoins `451bb9`, and reaches dispatch `451be4`. It stores next IP at `451bc1`, reloads that IP at `451bc8`, and stores resumed frame at `451bcf`; the base-cache pair is absent. |
| RETURN_GENERATOR | `45c95c` loads resumed base and `45c96f` stores its cache before dispatch `45c9b3` | Normal tail `451dbf → 451e16` retains frame/IP publication and tag conversion checks. It stores next IP at `451dd3` and resumed frame at `451e0b`, with no outgoing base-cache pair. |

For YIELD, the base-frame bookkeeping branch `44fe35 → 4653c3` rejoins at
`44fe3b`; its parent counterpart `457aa3 → 4622b9` rejoins at `457aa9`.
Neither bypasses the described outgoing tail. The caller's base load used to
publish the yielded value (`44fdd5`) remains necessary work; it is not the
removed cache transport.

RETURN_VALUE's generator-resumer branch is also inspected, not inferred from
the ordinary return. Candidate `451b74 → 466bcf` computes the continuation,
publishes the result at `466c44`, clears the outgoing frame at `466c6e`, and
branches from `466c73` to `451bc8` or `451bb9`. The parent branch
`452bbd → 463127` reloads/stores the resumed-base cache at `4631c6/4631cb`
before its corresponding dispatch joins. The candidate lacks that pair.
Its optional injection branch `466c5a → 473571` rejoins `466c60`, preserving
the same clearing and dispatch continuation.

RETURN_GENERATOR's slot copy uses the outgoing frame base at `451cb7`; result
publication uses the caller base at `451d47`. Those uses remain. Base-frame
bookkeeping `451d68 → 46541c` rejoins `451d6e`. Chunk reclamation
`451d8b → 4653f7` either rejoins `451d91` or calls PopChunk and rejoins
`451dbf`. Float/long conversion branches `46542a`, `466f86` and `46e019`
rejoin result publication at `451d47`; the later long conversion branch
`466e2e` rejoins final dispatch at `451dff`. Allocation failure
`451c0b → 470513` advances IP and enters the common error path `44f3a0`.
These are static edges, with no claim that generator objects take the numeric
conversion branches during an actual execution.

## Receiver dependencies and the six common control roles

At these parent entries frame is in r14, IP in rbx, and slots comes from the
C-stack cache `-0x628`. At these candidate entries frame and IP instead come
from C-stack locations `-0x630` and `-0x628`. A candidate receiver generally
loads the frame, then its base at offset 0x10, then accesses a slot. That adds
a frame-to-base dependency compared with loading the parent's cached base.
Some handlers retain the derived base in a register within their body; this
is not a claim that every individual operation adds a reload.

| Common role | Parent / candidate entry | Observed fast and fallback edges |
| --- | --- | --- |
| LOAD_FAST_BORROW | `45ddca` / `455836` | Both reach dispatch directly. Parent loads one cached base and uses resident frame/IP. Candidate loads frame at `45583a`, IP at `455841`, `455848`, `455857`, base at `455853`, and stores next IP at `455887`. The copied slot value and destination store remain. |
| STORE_FAST | `457543` / `451101` | Both preserve the local/nonlocal clear branch. Parent `457586 → 457593` and candidate `451169 → 451176` join their dispatch tails. Candidate reloads frame three times and IP twice before its optional clear, then stores next IP at `451180`; parent uses resident frame/IP and one cached-base load. |
| UNPACK_SEQUENCE_TUPLE | `457c9e` / `4507c8` | Candidate nonempty exact-tuple copy loops at `450861`, dispatches at `45088c`; zero-count branch `4611dd` rejoins `450877`. Parent loop is `457d1d`, zero-count `45ea0a`, dispatch `457d44`. Candidate guard misses go through `460cb0 → 450630`; length mismatch also reaches `450630`. |
| UNPACK_SEQUENCE_LIST | `457f8e` / `4505a8` | Candidate exact-length branch `45061d → 469320` reaches copy loop `469337`, dispatch `469360`; zero-count `469362` rejoins `46934b`. Parent loop `45800c` and zero-count `45ea16` reach `458031`. Candidate guard/length misses converge on the same generic unpack path as the tuple. |
| FOR_ITER_RANGE | `452aab` / `458451` | Candidate small-int path dispatches at `45853a`; exhausted branch `467095` dispatches at `4670b2`. BoxInt64 branch `46821e` rejoins `4584e7` on success, or publishes IP and enters `44f3a0` on failure. Parent equivalents are `452b77`, `46383a`, `464c2c`. Type miss `453dd0` enters the retained generic iterator path. |
| YIELD_VALUE | `457a35` / `44fd6e` | Outgoing pair removal and retained caller publication are described above. The candidate entry and tail also contain repeated frame/IP loads and stores. |

Generic unpack is followed past the specialized guard: candidate `450630`
loads the current frame base at `450660`, then calls
`_PyEval_UnpackIterableValues` at `4506df`. Its failure edge
`4506e6 → 46d59f` clears the source and enters `44f3a0`. Its boxed-input arm
`465b38` retains the precise-root window, calls the same helper and rejoins
`4506e4`. The specialization edge `461384` calls the existing specializer,
restores the current IP and redispatches at `4613b7`. Parent generic unpack
starts at `45792c`; guard misses go through `45e9bb` to that same body.

Generic FOR_ITER at `453dd0` calls `_PyForIter_VirtualIteratorNext` at
`453e50`. Success/value paths reach `460e73`; exhausted paths publish the
continuation and dispatch at `453e8e`; error `46ef2e` preserves the iterator
state and enters `44f3a0`. Specialization `461176` calls the existing
specializer and redispatches. These paths retain local frame-base derivation
and existing error/IP publication, not an incoming slot-cache dependency.
This is a bounded inspection of named continuations, not a whole-evaluator
CFG or root-correctness proof.

## Interpretation and retained evidence

The source hypothesis successfully removed the nominated transport pairs.
The stronger hope that eliminating the persistent cache would preserve or
improve common GCC frame/IP residency is contradicted by these sampled native
entries. The introduced memory traffic is an observed cost candidate. Its
elapsed importance, dynamic handler frequency, GCC allocation-pass cause,
cache effects and branch effects remain unmeasured. A source lifetime change
can alter generated allocation; this record does not identify why GCC chose
these locations.

The coordinator and independent reviewer can assess the separately frozen
correctness and native evidence, then decide whether the planned elapsed
comparison is useful. Neither these text counts nor the added instructions
replace that performance decision. The old thirteen-handler operand candidate
remains separately rejected on its actual measured elapsed results.

Author artifacts are in `build/interpreter-dispatch-state-native-author/`:
`capture_paths.py` and `path-captures.json` bind 45 windows extracted from the
whole saved disassemblies; `paths.py` and `paths.json` bind matched image/config
identities and eighteen explicit segment lists. Windows stop after 65 saved
lines and can contain unrelated following blocks. Segment lists contain
conditional alternatives and one listed copy of loop bodies, not dynamic
traces. Their static instruction totals carry no frequency or speed claim.
The capsule binds this note, both scripts, derived captures and original full
evaluator texts. Raw images and frozen source/helper/evidence files were only
read.
