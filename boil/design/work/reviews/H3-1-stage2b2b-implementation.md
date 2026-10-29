# H3-1 stage2b.2b development source review

Independent read-only source review, 2026-10-29. This is a development
checkpoint, not landing approval. No H3 source, test, build or configuration
was changed by this review; no native execution or tests were run.

Final development verdict: **SOURCE_REVIEW_CLEAR** for refrozen manifest
`54b2832334af6148e6852799333361e1e089ffc6fc7642dbec92503ab84084a9`.
The one initial finding below is fixed in that reviewed delta. Runtime
gates and landing evidence still require their separate review.

Reviewed worktree: `boil-compiler-copyprop`, HEAD/baseline
`878647973ba08be9f2c8e32e972504b507440b46` plus the frozen 12-file diff.
Those production paths at the baseline match native prerequisite
`dc02f76c0ccf69cd7195431dc0335a349bdec48f`.

* Frozen diff: `build/H3-1-stage2b2b/frozen-production.patch`, SHA256
  `2245622bc4fafaa4ead4109b5be0142c368e582e5355e254632b6987b32dd38b`.
* Source manifest: `build/H3-1-stage2b2b/frozen-sources.json`, SHA256
  `603366f4b75c0bba2f6cb995d8f7d1547256efc9ee67dbc4fa21de7e8ede8c3f`.
  Every listed file hash, the exact changed-file inventory and the live
  binary diff hash were independently compared with the frozen record.
* Accepted design SHA256
  `84e518cb872e0522247ffced7f131e943add7891c348746e9ca626c62b2f2116`;
  shared CAPI proposal SHA256
  `71ec68cb7da7e217e213aa548a74f352176bf150f9ef233298b291b970899cae`.
  The private API/layout decision and three VARARGS grants are in primary
  PROCESS sections 0.2 and the bounded shared-file exceptions.

## Initial finding and reviewed correction

**Initial FIX_FIRST: preserve configured requests for no-monitoring code.**
`Python/instrumentation.c:1725` retains the early
`CO_NO_MONITORING_EVENTS` success return, but the local request/tool-version
write now occurs only in preparation/commit below that return. The old
`_PyMonitoring_SetLocalEvents` wrote those configured fields before calling
the force path; the new setter at line 1987 therefore changes observable
bookkeeping.

A source-supported counterexample is an ordinary function code copied
with `code.replace(co_flags=flags | CO_NO_MONITORING_EVENTS)`. The flag is
`0x2000000` in `Include/cpython/code.h:131`; the existing non-shim
`_co_firsttraceable < Py_SIZE(code)` guard still permits this code. A valid
local LINE request previously succeeded and was reported by
`get_local_events`; the frozen candidate succeeds but reports zero. This
review did not execute that counterexample.

The coordinator ruled to preserve the previous configured local
request/tool-version behavior on successful no-monitoring paths, while
allowing installed instrumentation to remain absent. No new compatibility
break or constructor restriction was approved. After the active gates
quiesced, the owner applied that bounded correction and a valid
`code.replace` regression, then froze the source again:

* Incremental `build/H3-1-stage2b2b/bookkeeping-fix.patch`: SHA256
  `c8e1c11cbecf16126d6ae4a0703b5c1591fa1a0691cbc11da45a16a93a916be1`.
* Complete `refrozen-production.patch`: SHA256
  `7c64784ac0481248cdf88f5d381da355d028246a008615531768c8af8a0c907d`.
* Complete `refrozen-sources.json`: SHA256
  `54b2832334af6148e6852799333361e1e089ffc6fc7642dbec92503ab84084a9`.

The incremental change touches only `Python/instrumentation.c` and
`Lib/test/test_monitoring.py`. All 12 final file hashes and the complete
live diff match the new manifest; the other ten files retain their prior
hashes. The no-monitoring success path now updates only the selected
configured request/tool version, before returning without arrays or word
rewrites. Internal global-refresh calls with tool_id -1 keep the original
no-op behavior. The added regression checks requested and cleared masks,
empty active/backing state, unchanged words and absent callbacks on a
valid non-shim copy. It targets the actual observable regression rather
than just reading an implementation flag. This source finding is closed;
the regression's final execution is not claimed by this review.

## Remaining source checks

No further blocking defect was found in the reviewed storage/transition
diff. The following checks describe source conclusions, not executed
qualification results.

* LINE has fixed `int` entries and a separately allocated aligned uint16
  original-token array; INSTRUCTION is uint16 throughout its pointer,
  writes, displacement helpers and generated dispatch reads. Tool masks
  remain bytes. Teardown frees the nested LINE original array before its
  header. Primitive stores and actual canonical admission remain separate.
* All five existing production GetLine callers change coherently. Public
  Addr2Line checks its nonnegative index before either cache/fallback
  branch; current/previous instrumentation indices and both legacy jump
  endpoints have upper bounds before direct loads. Legacy forward and
  differing-line DISABLE behavior remains intact; absent previous lines
  are not given a new nonnegative assertion.
* Initialization sets every logical line/token slot, preserves absolute
  lines at real heads including token-zero heads, and leaves relaxed
  trailing words at -1/token0. Eligibility remains distinct from an
  installed LINE wrapper. ASCII-style byte packing/delta widths are gone
  from active storage consumers.
* The new generated relations are immutable mappings and domain-sized
  uint16 tables. Ordinary twins use the shared-format analysis;
  transparent LINE/INSTRUCTION have self-install and no-deinstrument
  semantics. `_PyCode_Deinstrument(int)` validates metadata on the original
  scalar before base lookup/narrowing. Runtime token stores use the same
  scalar-first canonical check; restore uses HandlerFor to retain the
  pristine format/A-D layout and NOFB choice.
* Preparation copies current header state, reconciles stale local tool
  requests privately and allocates only missing blocks. Each failure path
  frees only unpublished new blocks, including the two LINE allocations.
  A completed independent empty header/executing copy can remain. Existing
  masks, versions, words and backing presence are untouched by failure.
  The local equality early return is removed, so identical requests cannot
  conceal a failed preparation.
* New masks are seeded from actual installed wrapper layers and old active
  membership, rather than eligibility alone or zeroed INSTRUCTION masks.
  Ordinary unused mask bytes are initialized too. Commit removes old
  membership while old active masks remain available, then publishes new
  active membership, adds new wrappers and stores code version last.
  No recoverable allocation/Python callback is introduced in that interval.
* The global setter retains its separate partial-progress contract:
  requested interpreter masks/version may change before a later code's
  preparation fails. Earlier committed code stays valid; a same-mask global
  call is not proof that every stale code refreshed.
* The `_testinternalcapi` diff is restricted to the granted include, three
  additive helpers and three VARARGS entries. Scalars are parsed as ints
  before narrowing; `monitoring_state` copies masks, versions and presence
  flags before its first result allocation. Existing GC/compiler/scalar
  fixtures are unchanged.

Current allocation products use the existing admitted code-length bound:
`_PyCode_Validate` limits code bytes to INT_MAX, and words are eight bytes.
That bounds the fixed-line, uint16 and byte products on supported 32/64-bit
targets; this is a source argument, not a large-allocation test result.

## Evidence boundary

The added tests cover primitive adjacency/boundaries, scalar aliases,
enabled membership when multi-tool storage appears, DISABLE/restart,
fresh/transition persistent allocation-failure prefixes with identical
retry, and accepted backward/extreme AST lines. The read-only state hook
does not expose array payloads; actual membership and exact adaptive-word
restoration remain required runtime oracles. Primitive token 65535
round-trips do not prove execution of a high canonical token in today's
unchanged domain.

Retained 450-test and eight-AST positives precede the final source
qualification and do not certify the frozen production manifest here.
Final affected normal/Og/actual-kill evidence, allocation/peak accounting,
negative-control restoration, generation/phase checks and exact source/
binary provenance require their later review at the refrozen identity.
The earlier 603-manifest smoke/affected runs do not qualify the subsequent
bookkeeping change. No runtime landing verdict is issued by this
development review.
