# Preserve published attribute results through the final flush

2026-11-06. Bounded follow-up C to the accepted publication-boundary cleanup.
This proposal is source/assembly evidence, not an implemented or measured
speedup. Keep B independent for comparison. GCC only; no new representation,
runtime switch or publication-state cache.

## Remaining work

Each of the five specialized loads INSTANCE_VALUE, SHAPE, MODULE, WITH_HINT
and SLOT now publishes its temporary result once in `IR_RETIRE_PUBLISHED`.
The generated handler still ends with unconditional `IR_STORE_DST(dst, attr)`.
For a temporary destination this repeats canonicalization checks and the same
store. For a local destination the final store is necessary: its old value
remains observable until receiver retirement finishes.

The generic attribute completion path already describes this distinction with
`IRStack.published_temporary_result`. Its final flush emits the destination
store only under `IR_IS_LOCAL(dst)`. Reuse that existing postcondition for
successful specialized attribute retirement instead of adding more state.

## Contract and minimum change

The existing five-load branch in `IREmitter.retire_published` already selects
the local-only `failure_result`. Reuse that same branch to establish the
published-result postcondition after the initial clear-inputs/flush. Do not
add a second opcode whitelist. The fact is used by the final flush after the
successful retirement path; it is not runtime state or a cache of prior stores.

For these five producers, successful retirement means:

- If dst is a temporary, its exact home owns the canonical `attr` published
  before receiver release. The C variable names the same bits; it must not
  acquire or release another owner merely because the final write is omitted.
- If dst is a local, its old value remains in place and `attr` stays owned in
  C until the final store. Preserve that store and displaced-owner retirement.
- `retire_published_values` releases detached receivers/boxes and refreshes
  actual frame/PC state; it does not write the result home. An error jumps to
  cleanup before the final flush, retaining existing `failure_result` behavior.
- Callback success retains frame and executing-word identity. Instrumentation
  may replace the handler, but operand fields, including dst, are unchanged.
  Public `f_locals` mutation can change locals, not temporary homes. No cache
  invalidation check can substitute for these existing lifetime contracts.
- After retirement, the current five macro flows do not reassign `attr` or
  dst. Their only following operation is `_PUSH_NULL_CONDITIONAL`, which writes
  the distinct dst+1 companion. Keep that write and final dispatch unchanged.

Do not apply this rule to stores, pairs, arbitrary output windows or new
producer flows. A future operation that mutates the published result/destination
must either leave this postcondition false or establish a new reviewed
publication boundary. The current generator's same postcondition for generic
attribute completion remains unchanged; do not introduce distributed assignment,
copy, merge or reload tracking for this packet.

## Existing optimized GCC evidence

Read-only inspection of the existing release `python` ELF found build ID
`a7e0202b84f24b274660d46f0f9e9a0e1afcfd9e` and
`_TAIL_CALL_LOAD_ATTR_SHAPE` at `0x352540`, size `0x4d2`. This is the existing
compiled baseline inspected during B preparation, not a build of this proposal.

The disassembly retains the final temporary path after receiver retirement:

| Address / offset from handler | Observed instruction work |
| --- | --- |
| `0x3526c4` / `+0x184` | One earlier publication store of `r14` to the result home |
| `0x352944`, `0x352958` | Alternate earlier publication stores for other value-tag branches |
| `0x3529bd` / `+0x47d` | Call to `retire_published_values` on the deallocation path |
| `0x3529cf..0x3529f0` | Reload frame/word/slots and rejoin result completion after successful callback |
| `0x35274a..0x35275c` | Final local/temp comparison and temporary destination address calculation |
| `0x352766..0x352792` | Repeated pointer-range/tag/type checks, including exact int/float comparisons |
| `0x35279f` / `+0x25f` | Conditional `_PyValue_Canonicalize` call in that final publication path |
| `0x3527b0` / `+0x270` | Final `mov r14,(rdx)` to the temporary result home |

This demonstrates retained static instructions. It does not measure branch
frequencies, their cycle cost, or a speedup. The previously sampled shape
handler share is an entire-handler budget; eliminating one final publication
cannot claim that whole share or promise interpreter parity.

## Validation and decision

Prepare a separate artifact. Generated C should keep the first temporary
publication and callback refresh/error paths, wrap only the final destination
store in the existing local predicate, and keep companion NULL stores. All
operand/synchronization/error/effect metadata must remain unchanged. Extend
the focused generator contract test to require that final local-only form.

Use focused attribute/destructor/frame-locals checks for temporary and folded
local destinations, including destination/input overlap and receiver retirement
that reenters. The coordinator compiles release/debug and compares the actual
GCC instructions and elapsed/native-instruction counts against B. Reject a
complexity increase or a regression. No tests, builds or benchmarks were run
by the reviewer for this proposal.
