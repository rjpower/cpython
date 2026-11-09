# Measurement preflight proposal

Scope: extend `pyjit/issues/checkpoint-closure/measure.py` without changing its
three rotating rounds, 29-kernel roster, `--max-load 4`, or 45-minute window.
This proposal adds preflight evidence only; it does not authorize or start a
timing run.

## Required invocation and qualification label

Require `--qualification qualified|diagnostic`. Persist the label in a
preflight JSON, invocation journal, and summary. A diagnostic run must stamp
its outputs `DIAGNOSTIC ONLY — NOT R27 ADMISSION`; it cannot satisfy any R27
condition regardless of its results. Qualified mode rejects any failed
identity, cleanliness, release, or JIT check before timing begins.

## Preflight capture per build

For upstream, interim and candidate, resolve the executable and source tree.
Read `srcdir` from that build's `Makefile` (resolve relative paths against the
build directory); record the Makefile path/hash, source path, `git rev-parse
HEAD`, full `git status --porcelain=v1 --untracked-files=all`, and expected
identity. Qualified mode requires clean sources and exact upstream
`3a440b65695` and interim `eba351fee7d` commits. The candidate must also be
clean; record its commit. Compare the executable's `sys._git` revision with
the source HEAD; a missing or mismatched revision fails qualification. Do not
silently substitute a changed source tree or binary.

Probe the interpreter without timing. Record `sys.version`, `sys._git`,
`sysconfig.get_config_vars()` fields for compiler, CFLAGS/optimization,
`CONFIG_ARGS`, SOABI, debug, allocator and relevant build flags. Query
`sys._jit.is_available()` and `sys._jit.is_enabled()` where present; also
record the fork's `_testinternalcapi.jit_method_natural` state when that API is
available, restoring the previous state immediately. Qualified mode requires
release (`Py_DEBUG` false), `sys._jit` explicitly disabled, and no active fork
method-JIT admission. Record method-JIT availability as reported; if its probe
exists, read the prior state by setting False and immediately restore it. An
available probe returning True fails qualified preflight. Do not treat an
unknown `sys._jit` state as off.

In a normal startup probe, import `json`, `encodings`, `_decimal`, `_ssl`,
`_sqlite3`, `_lzma`, and `_ctypes`. Record each imported module's `__file__`,
origin, and import error if any. Require `json` and `encodings` to come from
that source tree's `Lib`, and each of the five named extensions to load as a
shared library from the matching build/source tree. Preserve the expected extension roster variation: candidate
117, upstream/interim 116; record the actual result of each build's existing
module check and do not call this difference a mismatch. Do not infer the
roster from the seven explicit import probes. Record ELF Build IDs for the
interpreter and each imported extension (`readelf -n`); missing IDs are
explicit failures in qualified mode. Keep this identity evidence separate
from the timed result files.

Save the full preflight snapshot before timing. Start the 45-minute timing
window only after preflight finishes. Recompute source HEAD/status, Makefile
hash, executable and imported-extension Build IDs, interpreter
version/configuration, module origins, and JIT state after the final round.
Always persist postflight status and both snapshots, even if timing produced
samples. Abort qualification if any identity changed; retain the per-field
differences and ensure no summary labels a changed run as qualified.

`--preflight-only` records whether the preflight itself passed but always sets
`eligible: false`, because it has no timed rounds or postflight identity check.

## Execution and result wording

The current three-round rotation remains unchanged. Preserve its invocation
vectors and per-kernel samples. Change summary ratios to the canonical
baseline/candidate convention: a ratio above 1 favors the candidate. Name the
baseline explicitly in each table. A diagnostic run can still produce timing
vectors for investigation, but cannot be presented as a qualified comparison
or R27 evidence.

RSS remains a later, separate operation after the timing window. Before that
operation, map and touch every page in each compared `python` and
`build/lib.*/*.so`; retain the exact touched-file list and `fincore` output.
Do not fold page warming or RSS collection into the timing rounds.

## Minimal checks before use

Add no benchmark samples. Run a syntax check and focused preflight fixtures
with fake build metadata/probe JSON: exact controls pass, dirty source and
wrong control commit fail qualified mode, changed postflight identity fails,
missing imports/build IDs fail qualified mode, the 117/116 roster is retained,
and diagnostic output is visibly excluded from admission. Confirm by inspection
that the script's normal timing loop is not reached when preflight fails.
