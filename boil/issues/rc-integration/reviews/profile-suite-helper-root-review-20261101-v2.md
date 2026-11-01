# Bound RC safety-suite helper review

2026-11-01. **ACCEPT the exact held ASan16 and full stackref-debug helpers.**
This independent review grants preparation acceptance, not execution or suite
coverage. Both bind clean source dce654 and the independently accepted fresh
builds; neither reuses the ordinary pointer-layout extensions.

Master capsule `build/worktrees/rc-successor/build/rc-profile-test-binding-v1/capsule.json`
SHA256 `fe17aa1b6e1bdad00c3c47e22b3bb41d92009f86b57b200e685ee04112a7456a`.
The exact per-profile capsule, plan, launch and ready hashes are retained in
the independent replay and receipt under
`build/rc-profile-suite-helper-root-review-20261101-v2/`.
Receipt SHA256 `d3c8cfec9bfdd80e587d154bf6ba4ab3ab28ba299e8d8c6f1e8a9855ae81a1ec`.

Root read the actual binding diff, loaders, argv validator, observer, result
contract, cleanup and authority. Independent replay rehashed7,186 ASan and
7,187 stackref inputs,22 ready artifacts per profile and all master bindings;
actual build/configuration/product/tool/CAPI/profile-witness/cache/import/output
preflight and four narrow host controls passed. Preflight's process guard
allowed only four git calls; no compiler query, target or activation occurred.
Seven earlier draft controls and the accepted alias/foreign-image cleanup
control remain scoped carried evidence rather than new native coverage.

Only two changes extend the immutable suite-v1 protocol: explicitly bind the
actual build-plan hash, and reuse the independently reviewed exact-executable
pidfd cleanup with its profile parameter and mandatory scoped receipt. The
cleanup body is unchanged. Both machine slots and the existing build lock stay
held until actual Python/_testembed cleanup; start-identity checks protect PID
reuse. Prior templates and failed host controls remain preserved.

ASan keeps all sixteen declared modules, including the standard deliberate
buffer-overflow test, and only the existing remote-exec exclusion. Options are
`detect_stack_use_after_return=0:detect_leaks=0:halt_on_error=1`; fake-stack and
LeakSanitizer are outside coverage. Stackref runs the complete default suite
with its compiled checker flag and actual evaluator-call witnesses. Both keep
j16, seed0/order, short TMPDIR, actual nested origins, own-image configuration,
visible standard skips and ordinary non-skipped protected-module requirements.
The result contracts reject failures and missing results; historical tracing
KILL/root/stress counters supply no RC credit.

Each profile still requires a separate root activation: ASan2,610 seconds
inclusive, stackref5,610, with their existing run/cleanup reserves, downward
clamps, stop-on-failure and no automatic retry. No source implementation,
collector option or new generic executor is introduced. The separately failed
CPU packaging gate is not passed by this review.
