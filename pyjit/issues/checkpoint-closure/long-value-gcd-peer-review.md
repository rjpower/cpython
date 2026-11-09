# Exact-base value GCD: peer review

Coordinator approval of `long-value-gcd-proposal.md` for implementation.
It preserves one Lehmer implementation for large inputs and bounds mixed
word/large work to a remainder followed by word GCD. It avoids Python numeric
dispatch and operand projection, and returns a value directly at the small
result exit. Unsigned magnitudes and `PyValue_FromUInt64` cover INT64_MIN
and a gcd of 2**63 without signed negation overflow.

Both word buffers must remain independent and live while their spans are
read. A negative mixed-input remainder still needs a sign-independent
magnitude; close its owner on success and error. The existing big-input
ownership/mutation and allocation-failure cleanup must survive the refactor.
Retain a genuine object compatibility entry, with the provider and ownership
inventory updated. Integrate its unavailable mark only with all core caller
ports; the provider may land first without falsely claiming caller closure.

Post-review must inspect the actual shared-body diff, exercise mixed signs and
integer bounds, compare large-input results and check that the Lehmer path is
still reached. Decimal integer-ratio integration and math.integer callers
are separate bounded packets; they must not call the compatibility entry.
