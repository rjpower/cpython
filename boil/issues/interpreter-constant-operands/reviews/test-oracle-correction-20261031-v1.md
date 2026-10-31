# Coordinator review of the source-control oracle correction

2026-10-31. **Accept the test-only correction at
`a475acbf46d117137f7201adcb4557ce92fb9ec5` for remaining native feedback.**
Runtime/compiler acceptance and the original deadline remain unchanged.

The completed [exact-code observation](failure-observation-20261031-v1.md)
ran once on the frozen 7f0b1819 debug image and source at 21:33 UTC. It selected
the three original nested CodeTypes without invoking a function or test.
The actual words show:

- `shared` retains LOAD_SMALL_INT, COPY/STORE and ordinary ADD, with no immediate.
- `dynamic` retains ordinary ADD, with no immediate.
- `canonical` has ADD_SMALL_RHS with literal 1 at offset 4 after the existing
  true-arm exit-block inlining. Its false path retains literal 0's load and
  ordinary ADD. The source spelling no longer describes a canonical input at
  that inlined consumer.

The observation returned zero, recorded 27 collections and zero fallbacks, and
passed gate/current-idle/verification cleanup. These are compiled-word facts,
not calls of the examples, specialization execution or a speed measurement.
The root audit binds all probe artifacts plus the external parent-directory
launch/result/gate/verify/idle receipts:
`build/constant-operands-review/observation-audit-20261031-v1.json`, SHA-256
`d7bf9af6357bc56c9f7b778c250715c72fc028f98204cabc9883832c75a53efd`.
The words are `69cf1a2740cbfbd60705336e1d2d1a34939c0962c87f7b2b27178dbd9d14937c`;
the result is `ad7781690dd5706c0648f94c5cbeeece04ecb2c8b581ce7ccefca8ea27f0c794`.

I reviewed the complete 13-line test diff: eleven insertions and two deletions,
only `Lib/test/test_ir.py`. The method is renamed to
`test_shared_inputs_and_exit_block_inlining`. It keeps the shared/dynamic
negative assertions, checks the actual immediate and ordinary false-path
forms, and retains both branches' output checks. The actual raw-CFG canonical,
multiple/implicit-use and other rejection fixture is unchanged. The correction
does not loosen pass admission, drop a failing control or change arithmetic.

The exact patch SHA-256 is
`fb38b2e192bc00e978f08f6730754f7e85c1fb4555a1ee0c0dc418cbde548d88`.
All other tracked paths, including runtime, generator outputs and build inputs,
are unchanged from 7f0b1819. Existing debug products can therefore serve this
test-only continuation through an explicit source/image bridge. Preserve the
original source/image IDs and receipts; do not relabel them as an a475 build.
Refresh the actual test-file hash and renamed eleven-case roster. A later O3
build uses its actual a475 identity.

The original eleven-case failure and diagnostic remain evidence. Re-executing
the corrected eleven cases and broader planned checks requires separately
reviewed continuation helpers and root activation, with the original
**22:20:45.868428 UTC** deadline. No native retry occurred for this review.
