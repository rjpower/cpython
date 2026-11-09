# SharedStep34 compiled review

Read-only review against root's exact committed33 baseline, SHA256
`349eb492a3026bea2e6f95aef7d7b9a9fa83aa1fbd5da5190b3a1a03a80d24e7`,
build ID `935d5bfd723b8829e56da4aa37b54ffbd80c3c37`. After34 is build ID
`67cbe7015e97e1fb89ff2e27492a48222ebf0215`, SHA256
`60ec4f1c8962d116c4c4ddce3ea9894d4ab1c022db0888ef9a3c0a176ad74472`.
`E11/sharedstep34-assembly-review/comparison.json` and four named assembly files
retain paths, identities, symbol sizes and call sites. No candidate was executed
by A.

| Compiled text | Before33 | After34 |
|---|---:|---:|
| CompleteStep | 1858 B | 1597 B |
| CompleteGeneratorReturn | 340 B | 1795 B |
| Combined | 2198 B | 3392 B |
| Whole .text section | 4951993 B | 4953145 B |

Combined named completion text grows **1194 bytes**. Whole text grows 1152 bytes
after alignment/layout effects. `nm` shows no separate complete_step_impl,
constprop/isra completion clone or completion cold symbol; the two literal-kind
bodies reside inside CompleteGeneratorReturn. This is the anticipated code-size
tradeoff, not a reason to presume either a gain or a loss.

## Intended work eliminated

The VM wrapper has no call to CompleteStep, no eight-argument setup for that
call, and no call to Canonicalize. Its immutable HandlerBase family choice
selects a FOR or SEND body. The FOR body detaches two homes without SEND/THROW
window-width or destination branches; the SEND body detaches three and retains
the necessary NoDst destination check. Neither has a runtime canonical-input
test or THROW fourth-home load/close. The physical continuation still comes
from signed D plus the saved return PC. Canonical input is a caller fact, not
a numeric fast-case guess.

The generic exported Step still calls Canonicalize and contains dynamic kind
paths for native/THROW callers. Its compiled layout changed and shrank; it is
not byte-identical to the baseline despite the single source body's unchanged
ownership sequence.

## Stack and observed/error paths

Before33's wrapper saves RBX plus RBP and reserves 24 bytes, then enters Step,
which saves five additional callee-saved registers plus RBP and reserves 104
bytes. After34's wrapper saves five registers plus RBP and reserves 40 bytes;
there is no nested Step prologue. The unobserved SEND/FOR commit paths keep
owners in registers; stack spills are visible around observation calls and
deallocation/error paths. Thus inlining increases wrapper register pressure
but does not add an unconditional owner spill bank to these normal paths.
This is a static path inspection, not a measured stack-traffic count.

There are separate PRE/POST ObserveStep call sites for the two literal kinds.
The monitoring pointer checks remain; observation still uses the existing cold
adapter. Error paths retain GetRaisedException/SetRaisedException around owner
closes. Successful tails reload the actual frame PC after retirement; no old
cached continuation is returned after a close. Interpreter-parent redirect and
discard/deallocation paths remain compiled into the wrapper too.

The reviewed source changes only ceval.c and no generated/provider text. Native
575-test results and isolated elapsed/PMU adoption decision remain root-owned.
The code-size growth and machine path simplification must both be considered;
neither the prior sampled share nor this static instruction removal predicts
the performance result.
