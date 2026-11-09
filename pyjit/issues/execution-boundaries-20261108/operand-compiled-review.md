# Operand-mode compiled-code review

2026-11-08. Read-only inspection; no native builds or timing by this reviewer.

The golden release executable and the separately prepared golden release
candidate both contain `_TAIL_CALL_LOAD_ATTR_SHAPE` at `0x352bf0`, size `0x39c`
(924 bytes). Both prepared identities report golden `0765e9be47f`, GCC release
`-DNDEBUG -g -O3 -Wall`, native JIT and natural method admission disabled.
Inspection used `nm -S` and `objdump -d --disassemble=_TAIL_CALL_LOAD_ATTR_SHAPE`.
Bounded assembly captures are retained under build evidence
`pyjit-link-wave1/execution-boundaries-20261108/operand-compiled-review/`:
`golden-shape.asm` and `prepared-baseline-shape.asm`.

The local receiver / temporary result success path contains the predicted two
actual slot-class comparisons, not merely repeated C macro expressions:

- `+0xa9`: load frame executing-copy pointer; `+0xb6`: load nlocalsplus
  (`mov -0xc(%rax), %edi`).
- `+0xca`: compare B with nlocalsplus; `+0xcc`: branch to temporary-input
  handling. Local receiver takes fallthrough.
- `+0xd6`: compare destination A with nlocalsplus; `+0xd9`: branch to the
  local-destination path. Temporary result takes fallthrough.

Addresses above are offsets from the symbol start (absolute instruction
addresses `0x352c99`, `0x352ca6`, `0x352cba`, `0x352cbc`, `0x352cc6`,
`0x352cc9`). The dependency loads and these two compares/branches are real
compiled work. Generic receiver-retirement and displaced-local slow paths
also occupy handler text, although the warmed local/temp path never takes them.
Result duplication, type-version/shape/value checks, optional NULL output and
dispatch remain necessary. Removing all 924 bytes is not the budget: the
ordinary handler remains for other modes, and one extra handler adds text.

Candidate operand variant has not yet been compiled. After root integrates and
builds it, inspect `_TAIL_CALL_LOAD_ATTR_SHAPE_LOCAL_TEMP` and the ordinary
handler again. Record actual added symbol/text bytes and confirm that the
nlocalsplus dependency and both class branches disappear, while required result
duplication and guards survive. Do not convert this verified instruction budget
into a time prediction before the coordinator's matched performance screen.

## Candidate after root's operand integration

Root reported successful native release/debug compiles (logs
`operand-v1-{release,debug}-build.log`). Read-only inspection of the rebuilt
release ELF finds the new variant at `0x3522f0`, size `0xe0` (224 bytes), and
the unchanged-size ordinary handler at `0x352cd0`, 924 bytes. Captures are
`candidate-local-temp-shape.asm` and `candidate-generic-shape.asm` beside the
baseline captures.

The new variant contains neither the executing-copy/nlocalsplus loads nor either
ownership-class compare/branch. It retains pointer-space/type-version/shape
guards, canonical result duplication, destination publication, optional NULL
output and final musttail dispatch. GCC proves the NULL receiver retirement and
unused displaced-local branches away. No stack space is reserved and no stack
spill/reload appears in the new handler; its guarded frame setup uses one
push/pop of rbp. Baseline reserves 32 bytes, although its admitted fast path
also avoids spill/reload instructions.

For a representative successful local-receiver/temp-result execution, non-NULL
inline result and oparg bit zero, the captured baseline path executes 70
instructions versus 48 in the variant, including entry and dispatch. The two
removed class decisions reduce conditional branches from eight to six. A
reference-counted pointer result takes the unchanged increment-and-return block
in each handler (two extra instructions each). These are static path counts,
not hardware retirement measurements: deopts, optional NULL output and different
value categories take other paths. The exact admitted path count removes 22
instructions; it does not predict 22 instructions saved for arbitrary LOAD_ATTR.

The ordinary handler remains, so the local addition is a 224-byte handler plus
small metadata/dispatch entries. Whole executable `.text` changes from
`0x4a5879` to `0x49e9b9` (28,352 bytes smaller), far beyond this bounded addition.
That broader compiler/layout effect is reported separately and is not credited
as deleted operand work. Coordinator performance and spectral screens remain
the admission evidence for resulting throughput/layout effects.

Coordinator's initial two-round elapsed screen reports candidate/golden logging
1.01372, Richards 0.98583 and spectral 0.99995. Thus this bounded operand change
has **not demonstrated a logging speedup**, despite removing the verified
instructions on its admitted path. The source-identical prepared baseline's
logging ratio was 0.99827. These early screens do not establish causation; wider
layout/CPI and total workload costs remain open. Keep the change provisional
through the coherent batch and use a coordinated source ablation if needed.
No performance tuning or new profile was started by this reviewer.
