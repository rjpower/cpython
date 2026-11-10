# Final source and measurement-driver review

Reviewed 2026-11-10: clean runtime commit
`0a35a382909ca169ce3f381cb38206b1725001b3`, against parent `4d666e1b482` and the
previous [implementation review](implementation-review.md). This narrow follow-up
covers the positional-default repair, final declaration/codegen correction,
`capture-keyword-hits.py` and the packet's `measure.py`. No runtime edits or native
tests were performed by the reviewer.

**Approved for the coordinator's remaining validation and measurements. No
blocking source finding.** The coordinator's `object41-build-checks.json` records
clean source at this full commit and successful real release and debug builds.
Both returned zero; the release log reports 117 checked modules. The earlier
focused-module passes preceded the positional repair and are not evidence that
its new regression has run.

## Positional defaults

The implementation in `Python/ceval.c:3697` matches the approved proposal. When
the direct positional plan needs defaults, it reads the current tuple count,
checks required-argument coverage and compares the cached offset with the current
right-aligned offset. All checks precede allocation, owner detachment and frame
overlap. Empty/absent or insufficient defaults miss; an equal-sized replacement
continues to read current values; a changed-sized tuple uses the existing generic
transaction. No new owners, callbacks, binding semantics or frame fields were
introduced.

`test_positional_same_code_different_default_count` warms a fresh caller, confirms
`CALL_PY_GENERAL`, substitutes functions with the same nested code object, and
checks equal-sized, larger, shorter, empty and overlong default tuples. This
targets the shared-version assumption rather than ordinary mutation, which would
clear the version and conceal the defect. The larger-tuple case verifies correct
right alignment, while short/empty tuples require the normal missing-argument
error. Root should retain its executed result with the final candidate checks.

## Generator correction

Both target and non-target declaration branches now derive `this_instr`
mutability from the existing checked result-publication contract. This repairs
the release compile failure without another opcode-name exception. The fifth
lookup codegen test constructs a family head with that contract and checks its
reloadable instruction declaration; the actual NONDESCRIPTOR tests cover the
specialized branch. The final release build confirms the generated C compiles.

## Keyword counter capture

`capture-keyword-hits.py` is suitable for mechanism coverage, not timing. Each
case warms before clearing counters, then enables counters for 1,000 calls,
dumps into a fresh directory, disables/clears counters and selects the newly
created stats file. The measured wrappers contain one intended keyword call;
the surrounding loop invokes them positionally. The arithmetic expectations are
correct for permutations, positional prefixes, default holes, explicit
keyword-only parameters and implicit self. Bound wrappers, omitted keyword-only
defaults, variadics and over-capacity calls are valid zero-hit controls.

The check requires at least 1,000 plan hits for admitted cases and zero for the
fallback controls. This is stronger evidence than a `CALL_KW_PY` opcode alone.
The output records the executable revision/hash and wrapper IR. Root must verify
that revision is the final stats candidate when accepting results; the script
records identity rather than enforcing an expected commit internally. Use it
without `-O`, since its assertions are the admission checks.

## Timing driver

Comparison with `issues/execution-boundaries-20261108/measure.py` shows only three
changes: add `--group kernels|micro`, select that registry group, and require
frozen39 (`4d666e1b482`) for the golden role. The upstream control identity is
unchanged. Fixed benchmark bodies, native driver timing/warmup, seven samples,
balanced role ordering, affinity/hash/JIT checks and pre/post identity capture
remain unchanged.

The optional group therefore permits elapsed micro measurements through the
existing runner; it does not turn prior instruction-count ratios into elapsed
results. The output identities distinguish the new frozen39 control from the
older immutable golden even though both use the role label `golden`. Report the
actual control revision alongside results. No speedup or full-suite qualification
is established by this review.
