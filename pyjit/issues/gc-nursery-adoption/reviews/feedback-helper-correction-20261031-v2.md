# Independent review of the first-feedback helper correction

2026-10-31, completed at 21:16 UTC. **ACCEPT the exact v2 helpers for
coordinator activation of the remaining original feedback window.** There
are no blocking findings. The source remains the separately accepted
`b34931a881b33b47a2b2f4b1b2a644c50fed963b`; no runtime change is part of this
correction. This accepts neither GC behavior nor qualification.

Reviewed directory: the isolated source checkout's
`build/nursery-adoption-feedback-v2`. Exact SHA-256 inputs:

- `plan.json`: `9d994fc26185422694c8ee56124570a138616088c36125034fc53bb2801c9f46`
- `launch-held.json`: `446e8b93f06d6d8968102ae6aa21c37fe3f3a28a7e41abd0178f89105f017924`
- `ready.json`: `53462ab972c243538d9b44ed5654c1e0a31055ef9f9397ee4e188ec865cd1228`
- `correction-diff.patch`: `39fe6c94a46cc108184d8dca74460910e6014c852cda07741ba090097edf1f52`
- `host-preflight.json`: `448dd58adf1897291b72337366a8ac4059e5f3d7032ef96c98879f0d5d19e877`
- `toolchain-mismatch.json`: `83f7da9a057b4f246f8308380511dcf9721205cbe929fab18df3ef26079e9213`

The original launch stopped at 21:07:47 UTC before configure or compilation.
Its manifest recorded PATH aliases `/usr/bin/as` and `/usr/bin/ld`, whereas
the live build helper used GCC-selected program paths. The new evidence
establishes that both pairs have identical resolved paths and SHA-256 values;
this was an identity-protocol mismatch, not evidence of changed tool bytes.
The original failure, launch result, gate log, cleanup, start and held
artifacts remain at their original paths and match the correction evidence's
recorded hashes. Original activation also matches its start-record digest.

The correction uses the actual `build_once.tools()` producer during manifest
preparation. The exact dictionary comparison now includes `cc1`, `collect2`,
assembler and linker as well as the other eight tools, without weakening any
reported-path, resolved-path or content check. Moving collection/comparison
inside the existing `try/finally` retains the failed toolchain and product
receipt if that comparison rejects. The prepared evidence directory exists.

I independently reproduced the v1-to-v2 helper diff. Source input hashes,
symlinks, changed-source hashes, all seven job definitions and prior count
origins are unchanged. Original helper origins match their prior bytes.
New helper hashes and modes match the held manifest. The twenty-method
roster and 37/12/2 focused-child accounting retain their prior meaning.
The correction does not change observer, test, skip, root or cleanup policy.

The original batch began at 21:07:46.664152 UTC. The new plan, ready metadata
and activation checks retain **22:07:46.664152 UTC** as the absolute deadline.
`run_batch.py` takes the minimum of that activation-bound value, sixty
minutes from its own start, the 22:18:24 checkpoint and the overall deadline.
Per-row downward timeout clamping and cleanup/verification reserves remain.
The continuation cannot reset or extend the original window. Both new
metadata files were held, with no activation or native products, during review.

Independent receipt:
[`build/nursery-adoption-review/helper-correction-20261031-v2.json`](../../../../build/nursery-adoption-review/helper-correction-20261031-v2.json),
SHA-256 `dc9a98c6f81f80e5e7a2288efaa883b70a10c962ddc3f17e07e6e72f27423bef`.
It binds the sealed inputs, helper hashes, preserved original artifact hashes,
host replay and an additional actual build-helper failure-path fixture.

The frozen host preflight replay passed its eighteen rejection controls,
including altered path/resolved/hash and unbound `cc1`. In a separate host
fixture, I supplied a synthetic assembler identity and guarded every
subprocess launch. The actual `build()` rejected it before any configure/make
call and wrote `native_build=FAIL`, the rejected toolchain, `commands=[]` and
the product digest. Evidence was redirected to a small reviewer-owned
directory. A first reviewer-only fixture attempt stopped at an overbroad mock
of its git call; its log is retained. The corrected fixture and preflight
replay passed, and the frozen helper files remained unchanged.

Host identity queries, source/artifact reads and synthetic controls provide
no GC correctness evidence. No configure, compilation, link or target
interpreter ran in this review. The coordinator must bind this review and
the unchanged source review in the actual activation; first unexpected native
failure still stops the batch.
