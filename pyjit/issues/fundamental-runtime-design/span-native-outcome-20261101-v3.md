# Stable-span third native outcome

2026-11-01, coordinator. **STOP before cases: test extension link failure.**
Session 35054 returned 1 at 04:14:56 UTC. Source was
`34deae3b29260aea286b0c4017b963e092a2db62`. Configure/make returned zero,
but the module check reported `_testinternalcapi` failed to import because
`_PyGC_Collect` was undefined. The first test worker confirmed that module
was unavailable. No selected case, release build or elapsed measurement ran.
The make return code does not establish a usable test image.

The declaration in `Include/internal/pycore_gc.h` is plain `extern`, hidden
from shared extensions. Built-in `Modules/gcmodule.c` can call it; the new
shared `_testinternalcapi` call cannot. Both source reviews missed that link
boundary. The existing core partial-constructor witness can use the internal
entry normally. No allocator/collector behavior was tested by this failure.

Root moved the exact scoped-route collection body into a new exported cold
`_PyGC_SpanTestCollectWithRoute` hook in `Objects/gcspan.c`. The module now
calls that hook. The header declaration and no-mimalloc stub match the existing
test-hook boundary; the general collector entry remains internal. Error/route
restoration and all positive test assertions remain unchanged. Three source
files changed; production allocation and collection policy are untouched.

Correction `7d10d5bfd33cb7302df8008065c4dd637e453478` is frozen for independent
review. Existing 22 host root methods (one expected failure) and six source
root checks pass. No corrected image exists yet. A root preparation command
initially used the wrong working directory and failed before creating files;
the corrected command and checks succeeded. This is host preparation, not a
native rerun.

Original v3 artifacts remain under
`build/worktrees/gc-integrated-spans/build/span-native-feedback-v3`.
Actual post-gate cleanup acquired the debug-v3 build lock, found no live target
and completed before the activation ceiling. The unused window grants no retry.
The hard GC cutoff remains 15:14:44 UTC.
