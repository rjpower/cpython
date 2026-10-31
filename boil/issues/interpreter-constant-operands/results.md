# Constant-operand investigation results

2026-10-31. Offline source/native inspection at integrated source `d98291018c9`.
Recommendation: **review a bounded small-integer RHS operand-form prototype**;
no runtime implementation, measurement, speedup or landing is claimed here.

The [offline extractor](../../../build/interpreter-constant-operands/offline.py)
reads existing profiles and bounded ELF ranges. Its
[evidence index](../../../build/interpreter-constant-operands/evidence.json)
records all inspected input hashes, source blobs, target-table addresses and
retained disassembly hashes. The extractor is `7f07a5b8` and the index is
`82df47c7`. No target interpreter, benchmark, rebuild or new
profile ran. The executable digest is carried from the sealed product receipt,
not a repeated bulk image hash during post-merge smokes.

## Concrete native work

Actual native image is the qualified `e9e10d5b503` GCC O3 executable at
`../boil-nursery-shape-qualification/build/release/python`; its integration
differs in project prose only. The register compiler, assembler, generator,
format interface and dispatch macros are byte-identical to profiled `0b15`.
The exact generated ADD_INT body is also byte-identical. Shape changes outside
that body do not become arithmetic savings.

The opcode target table at ELF VA `0x6315c0` maps ADD_INT handler 130 to
`0x459276`, and LOAD_SMALL_INT_INLINE handler 205 to `0x45de06`.

| Observed instruction | Meaning |
| --- | --- |
| ADD `0x45929c`: slot load | Reload RHS from its physical frame slot. |
| ADD `0x4592a0`: compare; `0x4592a3`: conditional branch | Exact compact/inline integer admission remains after the producer's proof. |
| ADD `0x46119e`/`0x4611a1` | Dynamic left admission on the RHS-inline path; this remains necessary. |
| ADD `0x469e2e` then `0x46528e`/`0x465290` | Inline operands reach addition and the overflow branch; preserve both semantics. |
| Small load `0x45de38` | Store the constructed canonical inline value in its temporary slot. |
| Small load `0x45de3c`–`0x45de4a` | Load the next handler and indirectly dispatch to it. |

The proposed work is therefore present in actual code. However, the compiler
already shares representation decoding along the admission branches: the
both-inline path moves the low integer bits and jumps to addition. Do not count
two independent guard/decoder trees per inline operand merely because two C
helpers occur in source. The selected cut targets one RHS admission, slot
traffic/consumption and the producer's separate dispatch, not an allocation.

## What the old profiles establish

The extractor reuses the accepted decoder, PID, inclusive integer hot bounds,
period weights and compiler-suffix normalization. It requires the actual
candidate-image physical frame for inline attribution. The four accepted
cohorts retain 1,656/1,292/987/646 samples and their original total periods.

| Innermost residual helper | Coroutines | Comprehensions | Generators | Richards |
| --- | ---: | ---: | ---: | ---: |
| `_PyValue_IsCompactInt`, samples / % total sampled CPU | 12 / 0.724638% | 0 | 0 | 4 / 0.619195% |
| `PyValue_Int32`, samples / % total sampled CPU | 6 / 0.362319% | 0 | 0 | 0 |

No innermost `_PyValue_CompactInt` or `_PyValue_FloatOperandsCheck` record was
selected. These are helper contexts across callers, not proof of RHS constants,
arithmetic-site frequency, allocation counts or a speed forecast. Missing
samples do not prove missing executions. They cannot be added to inclusive
evaluator shares or used to turn the remaining coroutine/generator deficit
into boxing. Constant-load dispatch may appear as the physical evaluator;
these helper percentages do not bound the whole proposed cut.

The [saved path inspection](../../../build/gc-post-cleanup-performance-v2/executed-path-inspection/candidate/stdout)
shows why the previous
arithmetic microdiagnostics are not a validation workload for this proposal:
their exact constants become locals before the loop, and the inner ADD reads
two local slots. Even the small-int variant loads its constants outside the
hot add. Those adjusted instruction/elapsed vectors measure invariant operands,
not a loop-carried `n += 1` with a constant producer inside the loop. No new
recurrence execution or constant-site distribution was observed here.

## Decision and proof cost

The [design](design.md) selects existing LOAD_SMALL_INT producers only and a
generated same-word operand source for add/subtract/multiply, with ordinary,
in-place, integer, mixed-float and sequence fallback behavior kept coherent.
This needs a shared domain/format/materialized-input extension; it is not a
safe one-line guard deletion. Every miss/restoration path must preserve the
operand form because C changes from slot to immediate.

Unchecked frame-slot facts, feedback-padding facts and lookahead dispatch
fusion are rejected for the initial increment. Same-line fresh definitions
alone do not prove debugger-entry safety: `frameobject.c:1912` considers every
word on the target line. Arbitrary co_consts/float forms are deferred rather
than multiplying encodings now.

The result supports a finite representation prototype after design review;
it does not yet establish that the change pays for its generator/reader scope.
The proposed 90-minute host-only follow-up has explicit reject rules and no
automatic native or timing campaign. Broader checked arithmetic or attribute
facts require their own demonstrated consumer and fallback/entry proof.
