# Finite-major oracle source review

Disposition: accept the test-only correction `ef0af95904b87d888eee4a98b6a745835ff95085`
for bounded positive feedback and temporary source-negative preparation.
This grants no native execution or qualification credit.

The coordinator reviewed the exact parent856-to-ef0 diff: only
`Lib/test/test_gc.py`, 27 insertions and 11 deletions. The twenty nursery method
IDs remain. The patch SHA-256 is
`64a2fe8903b000cd1330645d06d293e9e60935e9f971b53a13c8d1944d5f7279`.

The earlier test ran manual minors and then allocated more than the major
budget in one later batch. Its early debt assertion detects a reset, but the
later major could occur even with that bug. It did not independently prove
progress under interleaved minor requests.

The corrected test establishes a major budget once, reads its actual limit,
then interleaves bounded allocations and manual minors while automatic GC is
enabled. It overwrites payloads rather than growing the live set. Bounds on
the initial remaining debt, each allocation delta and the finite iteration
count prevent one batch from crossing the major budget in the reset variant.
The final named assertion requires an actual major within that fixed exposure;
the old cycle must also die. The separate exact debt/statistics test remains.

The proposed source negatives are narrow: forcing the guarded minor entry to
major must fail the existing old-owner retention assertion; resetting only
successful-minor allocation debt must fail both the exact debt assertion and
the strengthened finite-major witness. These are expected diagnostic failures,
not runtime options. Require the corrected positive case first, fresh products
for each runtime negative, exact source/product attribution, idle cleanup and
a reviewed finite launch before execution. Keep sealed856 products unchanged.
