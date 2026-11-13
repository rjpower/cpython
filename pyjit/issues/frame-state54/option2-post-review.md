# Option 2 independent source and native review

Reviewed `frame54-feedback` at `7ea40d5d67c` against selected51. Source
review and read-only `nm`, `objdump`, and GDB type inspection only; no inferior,
build, test, benchmark, or timing was run by this reviewer. Root reports both
roles built117 modules and passed578 focused tests per role. Performance
admission remains root's decision.

No semantic source blocker found. The actual executing image immediately
precedes ir_words and is pinned by the independent executable owner. The
ordinary image, static init cleanup and evaluator exit trampoline have this
prefix; the embedded base sentinel does not. All handwritten ir_feedback
field accesses were removed. The hot accessor derives directly from ir_words,
not mutable function code or a longer executing-code decode. Existing refresh
and copy retain image authority; no owner or GC edge changes.

## Actual release geometry and code

GDB `ptype /o struct _PyInterpreterFrame` on
`BASE/frame54-feedback-release/python` gives ir_words80, return_pc88,
return_slot96, owner100 and owned_slots104. The fixed header is104 bytes,
down from112; sizeof112 includes the one declared trailing home. No claim is
made about debug/free-threaded alignment from this GIL release result.

`nm -S` gives the following complete handler text sizes, in bytes:

| Handler | Golden51 | Feedback candidate | Stack reservation in inspected code |
| --- | ---: | ---: | --- |
| BINARY_OP_ADD_INT |642 |642 |48 bytes, unchanged |
| LOAD_ATTR_SLOT |1240 |1240 |64 bytes, unchanged |
| LOAD_GLOBAL_MODULE |347 |347 |32 bytes, unchanged |
| LOAD_GLOBAL_BUILTIN |363 |379 |32 bytes, unchanged |
| CALL_PY_EXACT_ARGS |973 |974 |80 bytes, unchanged |
| RETURN_GENERATOR |1002 |1018 |16 bytes, unchanged |

Equal text size does not establish equal dynamic work. Representative actual
feedback dependency changes are:

- LOAD_GLOBAL_MODULE: golden `0x38c1ee` adds `frame+0x58` directly to the
  feedback offset. Candidate `0x38c65a` loads `frame+0x50` (ir_words), then
  `0x38c65e` adds `*(ir_words-0x18)` (entries). One additional dependent load
  precedes the entry read; there is no helper or code-owner lookup.
- LOAD_GLOBAL_BUILTIN has the same change at candidate `0x38c7ba/0x38c7c2`
  versus golden `0x38c34e`. Its32-byte stack reservation stays unchanged.
- LOAD_ATTR_SLOT: candidate `0x367fa3/0x367fbc` obtains words then entries;
  golden `0x367b88` adds the cached feedback pointer. Candidate reuses the
  retained words pointer for nlocalsplus at `0x368002`; golden separately loads
  words at `0x367bcb`. The dependency changes even though later CSE means this
  is not simply one more total load on every successful route.
- CALL_PY_EXACT_ARGS adds the words-to-entries load at candidate
  `0x37ab16/0x37ab21` versus golden `0x37a6d6`. Native frame reservation changes
  header subtraction112→104 and semantic home count subtraction14→13. Setup
  no longer loads/stores the cached entries word; the emitted scalar/vector
  arrangement changes, so source field removal is not a literal fixed count of
  removed machine instructions. Its80-byte stack reservation remains.
- ADD_INT's feedback read is on the guard-miss/deopt route: candidate
  `0x356020/0x35602e` follows words then entries; golden reads frame+0x58.
  Successful integer arithmetic still reads words for the local boundary.
  Do not charge every successful ADD a feedback-load penalty it does not run.

RETURN_GENERATOR moves one fewer header word: golden source offsets88/96 are
combined into the two-word feedback/return-PC copy at `0x3468b7..0x3468cb`;
candidate copies only return_pc from offset88 at `0x345eee/0x345ef3`.
Destination homes shift from generator+184 to+176. The full physical-home
move/NULL loop remains; no allocation or value-owner operation disappears.
Its larger total text includes layout/alignment changes and is not evidence
that copying more homes occurs. The inspected stack reservation and ordinary
call/overflow save-and-restore structure show no new spill frame; exact register
assignment and instruction placement differ.

This verifies the advertised header/cache removal and the real recurring
feedback dependency. It does not attribute elapsed time or cycles to either.
Root's reported initial coroutine screen loss needs confirmation; no code-size
or geometry argument overrides the measured workload gate.
