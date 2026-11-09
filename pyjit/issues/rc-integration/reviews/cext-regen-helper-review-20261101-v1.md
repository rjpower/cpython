# RC extension and regeneration helper review

2026-11-01. **ACCEPT exact held helpers for the two proposed native rows.**
This is preparation acceptance, not execution authority or a native result.
Source remains clean `dce654018d5ae16ffe5e772976d9ccc2c9d1e01a`.

Reviewed directory: `build/worktrees/rc-successor/build/rc-cext-regen-feedback-v1`.
Exact capsule SHA256 `645418d7a0eae80169695705d3324ca3c7da016b493e05a2f058516912d39b6a`,
plan `5d2303911f6109a6a5889e5426cf982aaed990d9895ddfcadc01316f7d767659`,
launch `ab87e5e1a576ea4723c97b5249105a81b15a48a4e056cb5e8fcbe4d9fdde1a6c`,
and ready `85756e5ef51ddc27f8159e4643c4cc1518e5b05f122577ec30f13b7681207c17`.
The explicit d7-image/dce-library bridge is retained; no image is relabeled
as rebuilt at dce.

Independent replay checked all 7,217 plan inputs and 27 ready artifacts,
actual source/product/tool-byte/import/output preflight, and eleven host
controls. All passed. The utility controls used shell fixtures and one
task-owned copy of a utility executable plus a foreign utility process;
they ran no compiler, CPython target or extension build. Reviewer evidence
is `build/rc-cext-regen-helper-review-20261101-v1/{replay.py,replay.log,receipt.json}`.
The author's earlier failed host fixtures remain preserved.

No blocking finding remains. The literal command schema validates both the
outer timeout/shell/gate/lock/image roles and actual inner CPU-test or make
command before admission. The two serial rows have a 2,820-second inclusive
window, 1,200-second run ceilings, downward clamps, cleanup/verification
reserve, and no automatic retry. Activation is absent.

The helper-local cleanup resolves actual `/proc/PID/exe`, so a venv argv alias
does not evade the debug Python/_testembed sweep. The existing debug build
lock and both machine slots remain held during cleanup. A pidfd plus a
post-open start-tick/executable recheck protects against numeric PID reuse;
TERM/KILL waits are bounded, a final live-image scan must be empty, and sweep
receipts join the row and completion artifacts. The host alias control
demonstrated removal of its unique owned image while preserving the foreign
image. This accepts the exact-image ownership convention under the exclusive
build lock, not an unrestricted process killer.

The source contains sixteen outer test methods, with the existing Linux
MSVC-only C++14 skip. The contract requires all sixteen actual IDs, fifteen
passes and that exact one skip, and the 32 direct launches implied by two
setup calls plus fifteen install/test pairs. These are requirements for future
evidence, not observed native counts. The child adapter preserves PIPE output,
stderr merging and text/bytes return types with pre-wait raw files. It observes
launches and copies each unique installed private-RC-suffixed extension before
the unchanged child `runtests.py` command. Its three warmups and three measured
reference-count repetitions are unmodified and have no custom interval
observer. Mapped-child origins and a complete compiler/ensurepip descendant
census are explicitly unobserved. The bundled pip and setuptools wheel bytes,
selected tools and `PIP_NO_INDEX=1` are bound.

Regeneration runs the literal existing `make -s regen-cases clinic` command.
It retains before/after snapshots of every tracked path, regular-file byte
and mode, and symlink target, including failure paths; equality and clean
source are mandatory. It does not repair changed generated files or repeat
canonical Autoconf qualification. Actual native results and independent
outcome review remain necessary before granting any qualification credit.
