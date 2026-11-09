# Boundary and discard compiled-code review

2026-11-09 UTC. Read-only GCC release inspection of integrated `5bfd1e7e457`
(`build/execution-boundaries-release/python`) and immutable golden
`0765e9be47f` (`build/pyvalue-rc-repr-release/python`). No builds, timing or native
execution by this reviewer. Bounded symbol, per-handler objdump and exact DWARF
structure captures are retained in build evidence
`pyjit-link-wave1/execution-boundaries-20261108/boundary-compiled-review/`.

## Verified local work and costs

| Handler | Golden bytes | Integrated ordinary bytes | New alias bytes |
| --- | ---: | ---: | ---: |
| CALL_PY_EXACT_ARGS | 1,281 | 1,297 | 1,297 |
| CALL_PY_GENERAL | 317 | 317 | 333 |
| POP_JUMP_IF_FALSE_BOOL | 218 | 218 | 194 taken, 195 fallthrough, 194 both |
| RETURN_VALUE | 624 | 768 | Shared completion in the ordinary handler |

The five new boundary/discard handlers add 2,213 symbol bytes beside existing
handlers, before alignment and metadata. The separately reviewed operand alias
adds another 224 bytes. This table is not a whole-executable text attribution:
compiler layout and shared helper changes extend beyond these symbols.

The exact discard alias sets the high recipient bit at the existing save point.
The general alias passes `CALL_POSTCALL_DISCARD` as constant 0x20 to the existing
runner (candidate `0x36ab35`); it retains status/child handling and native
continuation. A same-size exact alias does not make its extra recipient operation
free, nor does the 16-byte general difference cover shared transaction costs.

## Ordinary versus tagged return

Candidate RETURN_VALUE at `0x36b110` contains this representative recipient
sequence after child retirement and result publication preparation:

```
0x36b1a3  movzwl 0x60(%rbx), %eax       # caller return_slot
0x36b1a7  cmp $0xffe, %ax
0x36b1ab  jbe 0x36b1dd                 # ordinary slots: publish directly
0x36b1ad  cmp $0xfff, %ax              # special recipients only
0x36b1b1  je 0x36b20f                  # ordinary IR_NO_DST
```

The normal path does not execute a second tag/sentinel branch. Golden's
ordinary recipient path used one `cmp 0xfff`/`je`; the new common path also has
one comparison/branch, with a different condition/layout. For tagged recipients,
GCC masks/stores the recipient, checks live POP_TOP and its A field, and branches
to direct closure. The direct path advances the preserved continuation by eight
bytes after the close. It does not publish a POP PC or caller result home.
Decline reaches the shared publisher. Required result canonicalization and
possible finalizer calls remain; common-path branch count alone cannot establish
unchanged latency or CPI. RETURN_VALUE grows by 144 symbol bytes.

## Boolean continuation

The ordinary bool handler still compares the selected successor handler with
NOP (`0x19`), conditionally loads its successor, writes the NOP PC and advances.
For example, candidate base addresses `0x32f522`–`0x32f535` contain that sequence.
All three skip aliases omit this NOP test and PC publication. Their only
`frame->instr_ptr` store is the initial bool instruction PC. The taken variant
forms the admitted successor directly at `0x32eee5`, then loads its dispatch word.
Guard failure still reaches the canonical generic false-branch handler.

GCC also changes branch selection: the ordinary handler uses a cmov to select
an edge, while the taken alias emits a conditional truth-selection branch.
Removing the NOP branch therefore does not automatically reduce the total
conditional-branch count. The new forms eliminate the actual NOP traversal
work, but branch history/layout costs must be measured. Bool operand class and
history work remain. Assembly checks PyBool_Type and the immortal bool values;
calling this an inline scalar representation would be inaccurate.

## Transaction layout

`readelf --debug-dump=info` identifies `_PyCallTransaction` as a typedef of the
captured structure DIE. Both golden and candidate structures are exactly
64 bytes. `prepared` remains at offset 56; candidate `postcall_discard` is at
offset 60, occupying the old final four bytes of padding. Existing members do
not move. This is actual DWARF layout evidence, not merely a source estimate.
The added field still needs initialization and child-commit interpretation;
unchanged structure size is not a zero-cost claim.

## Admission boundary

The captures verify static removal of the admitted NOP traversal and discarded
result-home/POP-dispatch work, and bound several new costs. They do not establish
an elapsed speedup or qualify the runtime. Keep the unchanged logging workload,
ordinary call/return totals, spectral and broader controls in the coordinator's
net comparison. The operand-only screen already showed why local instruction
removal must remain separate from throughput admission.
