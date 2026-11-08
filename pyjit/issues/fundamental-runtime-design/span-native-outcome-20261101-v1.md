# Stable-span first native outcome

2026-11-01, coordinator. **STOP: focused correctness incomplete.** Tool session
83685 returned 1; the supervisor stopped at the first failed test row at
03:47:48 UTC, before its 04:30:59 ceiling. Release build/tests and timing did
not run. The unused activation window grants no retry.

The fresh GCC O1/debug build completed and verified after cleanup in 94.72
seconds. Actual image SHA256:
`357dc0cb6cc8a3f026499dec207ef2f8479a68ca4a46bfe299cb074155cec1ba`.
Build source was `7f982b3ecb6ae8e7f1a5c0433daa9acde870b7e4`; configure and
make returned zero. The module check reported 116 modules, ten unavailable
optional modules and zero import failures. This is build feedback, not runtime
qualification. Exact tool/configuration/product manifests remain in
`build/worktrees/gc-integrated-spans/build/span-native-feedback-v1/debug`.

The new span test worker aborted in `test_c_api_constructor`, at the inherited
`gen_new_with_qualname` assertion `frame->frame_obj == f`. The log records the
preceding backend-realloc case as OK, but no complete worker result is accepted.
Three remaining modules were omitted by fail-fast. Parent cleanup acquired the
actual build lock and confirmed no live target PIDs. No 26-case pass or release
coverage is claimed.

Root and author inspected actual current and pinned upstream source. The test
called `PyCoro_New(PyFrame_New(...))`. Pinned `Doc/c-api/coro.rst` explicitly
deprecates this old frame constructor in 3.16, with removal in 3.18, and says a
proper frame cannot be constructed for it. `Doc/c-api/gen.rst` records the same
limitation. The contradictory frame-copy assertion is inherited; this does not
establish a span ownership failure or justify patching production frame logic
to accommodate the fixture.

Approved correction: replace the new invalid helper/test with a supported C
call to an actual async function, preserve its defaults, root it across the
call and propagate errors. Keep span admission, suspension, completion and
payload lifetime assertions. Remove the unusable helper instead of adding a
compatibility branch. Correct the earlier proposal's claim of two usable
construction paths explicitly; legacy frame-constructor native coverage is
unavailable under the pinned upstream contract.

Correction source is bounded by 04:03 UTC, then independent diff review and a
new finite activation. Fresh build directories preserve all original products.
The GC cutoff remains 15:14:44 UTC. Original failure and streams are untouched:
test log SHA256 `b7db87014a5fdcb906bd073d1960e113aa45cc07428db8ec6d2b7d420c80d71f`;
supervisor failure `5159c1a1a009449fe8334d2bae6d558a2e9b938858b6227e25c72d913b7e2517`.
