# W5 continuation

Base: `e1229448be3`; preserved worktree `build/worktrees/w5`. The diff has 28
changed files, including staged `Modules/mathmodule.c`. Preserve the index.

## Current evidence

`build/w5-out/k3.json` reports zero boxes for all 29 kernels, including pickle,
regex_compile and all XML kernels. The stats binary and measurement postdate
sampled changed sources, but the complete diff has not yet been rebuilt and
retested. `git diff --check` and its staged counterpart pass.

## Work and validation

1. Rebuild the preserved diff in the private stats and release builds; reproduce
   all 29 kernel counts and compile the actual release changes.
2. Run narrow module tests for changed behavior; independent review must inspect
   ownership and semantics before the coordinator integrates this increment.
3. The remaining R27 sweep needs the approved interfaces in `w5-interface.md`:
   mark object APIs with value counterparts, check the ownership table, and
   reject direct calls through marked object slots. Split independent work into
   `Parser/asdl_c.py` and generated `Python-ast.c`, Python compiler/parser,
   Objects excluding W5-owned files, Modules excluding W5-owned files, and
   `_decimal.c`. The existing W5 files remain owned here until coordinated.
4. Coordinator runs the full merged validation, static R27 gate, suite box
   census and performance measurements. The previously recorded suite census
   predates the current diff and cannot establish current behavior.

## Continuation results

Private `w5-stats` and `w5-release` builds passed on the preserved diff.
`build/w5-out/resume-kernels.json` reports zero boxes per loop for all 29
kernels. This is the existing benchmark metric, not proof of a complete R27
kernel gate: independent review found that gate must validate its roster and
startup-inclusive raw counters. `build/w5-release/resume-tests.log` records
4,290 focused tests passed, 106 skipped (math, array, JSON, random, pickle,
re, XML, copy, io and exceptions). Release build checked 117 modules, with no
missing or failed imports. `w5-preserved.diff` contains the full staged and
unstaged diff against the base. Independent review and merged validation remain.

## Follow-up increments

- `w5-gate.diff`: fail-closed R27 checks, reviewed; tests passed. Final gate
  remains closed on incomplete inventory and slot-call lint.
- `w5-config-flow.diff`: numeric configuration and bounded LOAD_CONST value
  paths; release compiler and embed tests passed (295, 9 skipped).
- `w5-marshal.diff`: recursive value decoder and public C compatibility exits;
  source reviewed, marshal/import and C API tests passed (412, 17 skipped).
- `w5-io.diff` and `w5-io-audit.diff`: direct value seek and filename stores,
  plus value-native open audit tuple. Source reviewed; I/O and audit checks
  passed (1,066 and 134 tests respectively).
- `w5-loadname.diff`: value-native LOAD_NAME, with a globals dict-subclass
  regression fixed during review. `w5-sysexit.diff`: internal SystemExit value
  home and inline exception normalization, including a regression for a
  constructor returning another exception subtype. The combined release build
  passed; scope, sys and exceptions tests passed (267, 11 skipped). Final source
  reviews pending at handoff.

The worktree contains the preserved W5 source and independent worker changes;
use the individual patches rather than treating the entire worktree diff as one
increment. A richards sample after the combined startup edits showed 16 CORE
and 19 THUNK boxes in the low process, down from 238 CORE before those edits.
It is not a final gate run. OSError's value homes and the other startup sites
remain; the reviewed design is `exception-load-name-proposal.md`.

## OSError value-home increment

`w5-oserror.diff` moves the five OSError payloads to internal value homes and
updates constructors, members, GC, formatting, reduce, SSL and buffered I/O;
it also removes the inline-float SystemExit printing box. The focused release
run passed 1,664 exception, sys, SSL, I/O and weakref tests (59 skipped).
Windows code was reviewed but not compiled on this Linux host. A parallel
`_json` change caused its shared module to fail import during `make all`, so
that build cannot be called fully clean. Independent source review is pending.

The combined worktree's fresh richards low-process raw counters are CORE 4,
THUNK 0 and adapters 0: three CORE boxes in `_io_open_impl`, and one in the
`Objects/dictobject.c` compile unit. This is not the 29-kernel gate. The raw
sample is `build/w5-out/oserror-richards.json`. Layer order after the kernel
and earlier startup prerequisites: `w5-io.diff`, `w5-io-audit.diff`,
`w5-loadname.diff`, `w5-sysexit.diff`, then `w5-oserror.diff`.
