# Cold R5 review

2026-11-07. **Accept runtime `0953757f4e7eb442896dcf42f6547009528defaa`
against parent `b58036e9718` for the bounded canonical-home experiment. No
must-fix source finding.** Keeping the optimization still requires the
planned elapsed-time evidence. This review does not establish a speedup or
full-suite qualification.

I read the applicable agent instructions and [CPython AI policy](https://devguide.python.org/getting-started/ai-tools/),
then the actual runtime diff, generated handler and surrounding ownership,
writer and generator code before reading the proposals and previous reviews.
The runtime worktree was clean at the stated commit before and after this
review. Runtime source, builds and performance artifacts were read-only.

The removed publication check has a concrete premise. `_PyValue_IsHome`
excludes pointer-form exact numbers that belong inline and invalid immortal
tags. `PyValue_XDUP` preserves its argument's bits and supplies an independent
owner where a count is needed. The shape load reads the filled inline slot
after the type-version and shape guards; its new intrinsic asserts the home
premise before duplicating. The internal `_PyObject_StoreInstanceValue`
declaration now requires a borrowed canonical home value or NULL, and its
entry assertion checks that requirement before duplication, callbacks or
stores. This is an internal caller precondition, with no new release-mode
normalization.

I traced the alternate in-tree writer routes: the generic value setter and
object adapter, `PyObject_GenericSetAttr` and its with-dict form, the native
setattro thunk, and `_decimal`'s generic-setter delegation. Object inputs go
through `PyValue_FromObjectBorrow` or `_Py_VALUE_ARG`; that constructor
canonicalizes exact numbers and tags immortals. The specialized shape and
shape-add stores transfer canonical frame-home owners through `TAKE`.
Initialization supplies NULL; duplication and detachment preserve existing
canonical bits. Materialization, clearing and detachment invalidate the
shape, and specialization declines `_Py_SHAPE_NONE`. Split-dictionary
replacement/insertion and copy paths also retain home checks. I found no
supported in-tree shaped-slot writer that defeats this premise. Rebuilt
external native callers must obey the explicit internal contract; this is
not an audit of arbitrary external extensions.

The generator records one narrow producer fact on the existing frozen
publication contract. It requires the direct unconditional result assignment
with one identifier argument and the matching final retirement marker. The
existing assignment, result-use, output-position and suffix checks remain in
force. Duplicate declarations, branches, reassignment, address escape and
intervening result use are rejected. Registering the intrinsic as a
nonescaping reader matches its expansion. The checker does not prove the C
pointer provenance of `found`; the writer audit and producer assertion are
the premise. No opcode-name list or general provenance/type framework was
added.

`Storage.prepare_flush` preserves the old preparation order. Only the
checked result's first temporary-destination flush receives the canonical
store choice. That macro retains the NULL/same-value assertion and debug
temporary check. A local remains installed during receiver retirement; on
success its final store still uses ordinary `_PyValue_Publish`, reading the
current home after callbacks. Failure still passes the C-owned result only
for a local destination and jumps to error before installing it. Temporary
ownership, receiver retirement, refresh of instruction/slots, displaced-local
retirement and companion NULL publication keep their previous order. The
abstraction is small enough for this experiment. Further simplification
should follow the keep/revert measurement decision, rather than expanding
this one source fact into persistent stack state or a broader framework.

I independently generated both source versions in memory with the final
generator, using `uv run` and `-B`. The current interpreter and metadata match
their committed products exactly. The complete interpreter delta is precisely
the two shape-handler lines: asserted canonical duplication and direct first
temporary publication. Formats, metadata and targets are identical between
the two source versions.

The retained ELF notes independently match R4 build ID
`a410f4a9c634117fdb4921079b2bcdcdfd190b92` and R5 build ID
`9d98a70694382c4650742915f0560f12346c85bf`. In the actual GCC disassembly,
R4's first temporary publication includes exact-number tests and a conditional
`_PyValue_Canonicalize` call at `0x352e04`. R5 stores the temporary directly
at `0x352d98` or `0x352efb`, according to receiver ownership. Its retained
exact-number tests and conversion call at `0x352e93` belong to the folded-local
path. Receiver retirement at `0x352fe5`, refresh and failure exit, and
displaced-local retirement at `0x352f90` remain. The shape symbol shrinks from
`0x45e` to `0x39c` bytes; its stack reservation falls from 64 to 32 bytes.
These are machine-code observations, not elapsed-time proof.

I inspected both new runtime tests, the new generator positive/rejection
fixtures, the existing specialized receiver-retirement test, and the relevant
frame-locals implementation. The category test checks actual shape selection
and temporary/local destinations across inline integers/floats, signed zero,
NaN, large integers, numeric subclasses, None and a mortal object. The folded
local test checks callback visibility, callback mutation and payload survival.
Its corrected receiver/sentinel/old order agrees with the retained upstream
and R4 oracle. `framelocalsproxy_setitem` calls `add_overwritten_fast_local`
before releasing the old home, so the original owner remains retained until
the frame dies. The initial failed oracle remains separate evidence.

I inspected `r5-checks.json`, `r5-identities.json`, both build and identity-link
logs, both test-log footers and the API-lint footer. The retained commands
returned zero; release/debug builds each checked 117 modules with no missing
modules or import failures, and each six-file focused run reports 630 tests
with one skip. Lint reports 306 native units, zero failing findings and zero
parse errors. The linked release/debug native Git identities both name
`0953757f4e7`; debug ELF identity is
`842535c720c4e7b0a8d52044e4865d3474294756`. I did not rerun native tests,
builds, full suites or workloads. The error/observer conclusion relies on
the inspected unchanged generated paths and retained focused validation,
not a newly forced native error experiment. Stats/full qualification remains
the coordinator's coherent-batch gate; performance admission remains separate.

R5 describes the existing owning-home register-interpreter baseline. Its
canonical-source fact and first-output publication phase do not supply a
native `FrameState`, error root map, post-effect recovery protocol or shared
method/tracing backend. Those remain planned work, as the recovery design
and re-review explicitly state.
