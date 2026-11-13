# Option 1 independent post-review

Status: corrected handwritten/generated source and release native contract
reviewed, with no unresolved source/native blocker. Performance and final
qualification remain open. This reviewer ran only source searches, existing
ELF `nm`/`objdump`, non-running GDB type inspection and SHA256 reads; no
compiler, inferior, runtime tests or timing was launched.

Reviewed source `612fb4e9d170532947c007f0ccaaa04aff623329` in
`BASE/worktrees/frame54-recipient`, against selected51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. Root identified the matching
release/debug builds via `recipient-assert-fix-{release,debug}-build.log`.
Release ELF SHA256 is
`65dce4fc69233672172f32a25a389c123d7d7d0d514ff1aeb69464fa9901d71a`;
debug is `6d57a1405f774ee3e5d6c2c338bac5865d3dddcc897ee2525fb726575670a72e`.
Both native type layouts retain112-byte headers/sizeof120, return_slot104,
owner108, flag byte109; debug LLTrace fills the remaining five flag bits.

## Source and generated products

The [handwritten review](option1-post-source.md) found the complete descriptor
port: normal/copy/sentinel/trampoline/call/resume producers, result publication,
debugger discard, generator yield-from numeric subtraction and remote asyncio
numeric decoding. No remaining raw descriptor is used as a physical index.
Redirect publication rereads the descriptor after callbacks; local replacement
installs before closing the actual old owner; empty has a debug NULL assertion
and preserves the held result owner. Named locals always require replacement.

Generated inspection found a real blocker in initial `b2ad1d146c9`:
`_PyFrame_SetReturnSlot` was absent from analyzer.NON_ESCAPING_FUNCTIONS, so the
new setter made _SAVE_RETURN_SLOT escaping. Exact CALL published an internal
wrapped native frame pointer to window[0], and property paths stored then
cleared it as if it were a normal home. Metadata also acquired ESCAPES flags.
The issue was reported immediately, then fixed by root in `daab07a6985` with
the actual pure helper certification. Corrected generation removes those
publications; IR/opcode metadata now has no diff from selected51. Exact entry
again keeps new_frame as an internal transfer until frame switch. The latest
commit also fixes a whitespace-sensitive preexisting generated-source test.
The old blocker is resolved, not waived as harmless extra work.

## Actual native work and added cost

Release disassembly confirms removal of the old-home read on the proved-empty
result route, plus real recurring work to record and decode that proof. These
are mechanism findings, not a dynamic instruction/elapsed result.

- **Ordinary return:** selected `RETURN_VALUE` loads old recipient at
  `0x381422`, installs at426, then tests NULL at42a. Candidate
  `0x3816a1–0x3816b7` copies/masks descriptor, forms the home, tests bit12 and
  stores directly on empty. There is no old-home read on that route. The
  replace branch at `0x381780` loads old, installs new, then classifies/retires.
  Candidate redirect calls at `0x38170f` and reloads descriptor at714; it does
  not retain an authoritative descriptor across callback-capable clearing.
- **Exact CALL producer:** candidate `0x37aa3a` loads caller ir_words;56 compares
  destination with prefix nlocalsplus;59–62 set/move/shift/or the class bit;
  64 writes the descriptor. Selected `0x37a8a1–a9` just decodes/masks/stores
  the destination. There is no new helper call on the warm thread/frontier
  route. Child initialization also gained real default-recipient encoding:
  `0x37aa02–1c` reads child nlocalsplus and sets/shifts/stores bit12, splitting
  the previous packed constant return_slot/pending_kwargs write. Count this
  entry cost as well as the caller's continuation producer.
- **SEND producer/publication:** candidate `0x3775a9–bf` loads caller words,
  compares prefix boundary and constructs the descriptor. `0x377611–627`
  masks/forms the child home, tests its class and stores directly on empty;
  only replacement takes the old-home branch at `0x377727`. Selected's
  direct old-home read is at `0x3774c7`. There is real added proof-recording
  work on every admitted resume even when the replaced home was already NULL.
- **YIELD:** candidate `0x344770–790` reads prefix boundary and encodes the next
  input recipient. Publishing to the caller uses mask/class test at
  `0x3447e6–fc`; its empty branch has no old-home read. The selected old-home
  read is at `0x343ad0`. Replacement remains out of line at `0x344878`.
- **Generator construction:** candidate `0x34689d–8cc` encodes the initial send
  destination after moving homes, and caller result publication now has the
  same mask/class branch. It still moves and NULLs every physical home.
  No heap allocation or frame clear work is removed by this lane.

The returned named-local DUP, frame clear/pop, generator completion adapter,
monitoring and value representation stay present. Empty-proof publication
trades an old-home load/NULL branch for masks/address construction/class branch;
it is not an unconditional instruction reduction. Producer work can exceed the
saved publication work even when the pointer read disappears.

| Release handler | Selected bytes | Candidate bytes | Selected/candidate C-stack reservation |
| --- | ---: | ---: | --- |
| CALL_PY_EXACT_ARGS |973 |1021 |80 /80 |
| RETURN_VALUE |546 |547 |16 /16 |
| SEND_GEN |743 |766 |16 /16 |
| YIELD_VALUE |453 |501 |16 /16 |
| RETURN_GENERATOR |1002 |1070 |16 /16 |

Sizes are `nm -S` complete symbol text, including cold branches/padding;
reservations are prologue `sub rsp` amounts, excluding saved rbp. Code size
and unchanged reservation do not measure dynamic work or placement effects.
Root's reached-route census, release/debug semantic/remote checks and balanced
unchanged-workload comparisons remain the admission evidence. This independent
review does not claim those campaigns passed or that the lane is faster.
