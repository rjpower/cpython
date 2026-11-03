# Pre-I string storage and certificate audit

Status: design/source audit for coordinator review, 2026-10-29. This is
an addendum to `pre-I-linear-string-concat.md`, not an implementation
grant. The original parent was reviewed at SHA256
`1616f77fe3ce3317f44f5c4ca07d3404d192a40433982d38bb54b3c91cdf47be`;
the later S0 revision below selects certificate ownership while preserving
this storage inventory and its separate approval boundary.

Audited primary HEAD: `db96fc12f15c850848e9afac3cabb948530c0814`.
The relevant `Objects`, `Include`, `Python`, `Tools/build`, remote-reader
and GDB sources have no changes from `9dd24bf42c4`. Evidence here is source
inspection and repository searches only. No native edits, probes, tests,
builds or benchmarks were performed. The inventory covers repository
producers and consumers; it does not establish compiled layout on every
supported C compiler or validate arbitrary extension-created object layouts.

## 1. Recommendation and decisions still required

Retain section 5's flat capacity trailer as a viable **conditional**
candidate. No inspected consumer requires a rope, a new public accessor
failure, allocator-size recovery, or a changed canonical-data offset.
The source does not yet establish ABI preservation or authorize mutation.

The smallest storage contract is:

* One named state bit identifies storage, for the allocation's whole
  lifetime. Sealing clears only the separate owner permission. The storage
  bit and capacity word remain after frame return, hashing, UTF-8 caching,
  interning and immortalization.
* Only the new private allocator creates flagged, compact exact strings.
  Every ordinary producer sets the bit to zero. Copies copy characters;
  unchanged-result aliases preserve the existing allocation and its bit.
* Writable Unicode writers and `_PyUnicode_ResizeCompact` do not accept
  flagged storage. Read-only writer adoption remains allowed and copies
  into an ordinary allocation before any write.
* `__sizeof__` includes the complete request, and normal/debug/ASan use
  their existing object-domain allocation/free pair. No debug prefix or
  allocator usable-size query becomes part of the representation.

The revised design selects a compiler-only immutable certificate in a
dedicated optional code-owned pointer, attached **during code construction,
before CREATE watchers**. Its code-layout cost and H3 integration still
require review. Section 6 retains the earlier alternatives as source
analysis; they are not implementation fallbacks.

Before implementation, the coordinator and Unicode/C-ABI owner must rule
on the bit/trailer and platform-layout checks; H3 must rule on certificate
backing and its construction interface. ROOTS and H1 still own allocation
callback/root and frame/observer closure. This audit does not close those
dependencies or approve S2/S4.

## 2. Finite header and state inventory

`Include/cpython/unicodeobject.h:50` defines the state: `interned:2`,
`kind:3`, `compact:1`, `ascii:1`, `statically_allocated:1`, followed by
24 unnamed bits. The first eight bits already have meanings. A proposed
capacity bit consumes one of the unnamed bits; it must not change any
existing field's width/order. `state` has explicit four-byte alignment
at line 153 because canonical data must remain suitably aligned, including
on m68k. Retaining the total bit count is a source argument, not a compiled
`sizeof`, alignment, bit-position or DATA-offset proof.

The searches for Unicode struct names, `_PyUnicode_STATE`, direct state
assignments, object allocation/reallocation and header-sized copies found
the following producer set. No inspected producer clones an existing
Unicode header. The remote reader's local header copy is covered in
section 5; it does not create a result by copying that header.

| Producer or state writer | Source and present behavior | Required disposition |
|---|---|---|
| Dynamic compact ASCII/Latin-1/UCS2/UCS4 | `Objects/unicodeobject.c:1220`, `PyUnicode_New`; one object-domain allocation; state members assigned individually at 1287 | Set the new bit explicitly to zero. Existing unnamed padding is not a reliable initialized flag. Keep empty-singleton return ordinary. |
| Dynamic subtype/legacy header | `Objects/unicodeobject.c:14264`, `unicode_subtype_new`; `type->tp_alloc`, member-wise state initialization, separate data allocation and logical data plus terminator copy | Always zero the bit. Do not propagate it from a flagged source. Subtypes remain outside the proposed representation. |
| Static global strings, identifiers and empty string | `Include/internal/pycore_runtime_init.h:191`, `_PyUnicode_ASCII_BASE_INIT`; generated layouts in `pycore_global_strings.h` | Explicit zero in the common state initializer; no trailer. Named omitted aggregate members are zero-initialized, but naming this invariant keeps the generation contract visible. |
| Cached ASCII/Latin-1 characters | `Tools/build/generate_global_objects.py:359`; generated `pycore_runtime_init_generated.h` invokes `_PyASCIIObject_INIT` and `_PyUnicode_LATIN1_INIT` | Same common initializer, no new per-character allocation or trailer. |
| Deepfreeze native Unicode literals | `Tools/build/deepfreeze.py:190`, separate ASCII/non-ASCII state aggregates at 225/238 | Both generated forms zero the bit; data arrays remain logical length plus terminator. Do not emit a flag from a host object's storage. This generator is an inventory consumer, not claimed current build coverage. |
| Interning state changes | `Objects/unicodeobject.c:1630`, `:14536`, `:14559`, `:14683`, `:14801` | These assign only `interned`; preserve capacity storage. They never grant private ownership. A sealed flagged object may become hashed/interned/immortal. |
| In-place compact-header preservation | `Objects/unicodeobject.c:1077`, `_PyUnicode_ResizeCompact` | Realloc preserves existing header bits while replacing allocation extent. Exclude flagged input before this path; clearing the bit before a possibly failing resize would lose the old allocation's capacity metadata/accounting. |

`PyUnicode_New` and subtype construction are the two dynamic Unicode
header producers in `unicodeobject.c`; its other direct object allocation
constructs `EncodingMapType`, not a string. Static literal producers are
the common runtime initializer and deepfreeze's two aggregates. Generated
singleton checks in `pycore_unicodeobject_generated.h` consume the result;
they are not another header allocator.

`_PyObject_Init` registers the object before `PyUnicode_New` initializes
its Unicode fields (`unicodeobject.c:1278`; `Objects/object.c:2512`). A
new helper must not publish the storage bit before its metadata is valid,
or infer that constructor return values cannot alias. Active reftracers
and unknown allocator chains remain disabled for the initial feature,
as the parent requires; existing constructor/collector safety remains
ROOTS' dependency.

## 3. Allocation, copying, widths and trailer lifetime

For header size H, character kind k, logical length L and capacity C,
the requested object size is `R = H + (C + 1) * k + sizeof(Py_ssize_t)`.
The metadata address is `(char *)DATA + (L + 1) * k`, read/written with `memcpy`.
Require `0 <= L <= C` and checked arithmetic for every addition/product,
including the final word. The metadata word fits even at `L == C`.
Writing it with `memcpy` avoids alignment and effective-type assumptions.
No native address is cached across a collecting allocation.
If the extra word or reserve cannot fit but ordinary logical concatenation
can, decline private storage and use the existing exact-copy path. Metadata
overhead alone must not add a new overflow/OOM result to that operation.

The concrete candidate allocator interface is an internal
`_PyUnicode_NewWithCapacity(logical_length, capacity, maxchar)`, sharing
the ordinary compact-header initialization with `PyUnicode_New` rather
than duplicating its state setup. Its checked request includes the word;
the caller fills the prefix and establishes the final terminator/footer
before granting permission or publishing the result. A single internal
capacity reader is used by consistency/sizeof and the approved private
append transaction. These are proposed interfaces, not existing APIs or
permission to use the helper from general C append/writer callers.

| Path | Source-supported result and required handling |
|---|---|
| Logical character copy | `_PyUnicode_Copy`, `unicodeobject.c:2338`, allocates via `PyUnicode_New` and copies `length * kind`; `_copy_characters`, line 1333, also copies/converts bounded character ranges. New copies are ordinary, regardless of source storage. |
| Legacy subtype copy | `unicode_subtype_new:14328` copies `(length + 1) * kind` into its separately sized data block. The source flag is not copied; the terminator remains ordinary. |
| Exact unchanged result | `unicode_result_unchanged:749`, `PyUnicode_FromObject:3355`, and empty-concat shortcuts may return the same object. Preserve that object's metadata; these are not freshness or ownership evidence. |
| Public resize | `unicode_resize:1681`, `PyUnicode_Resize:1712`: same length preserves the object; zero returns the empty singleton; changed positive length uses `resize_copy`. Thus copies zero the bit, aliases retain it. No public resize privilege is added. |
| Width adjustment | `unicode_adjust_maxchar:2294` and writer widening/narrowing allocate a new `PyUnicode_New` result and copy characters. The proposed helper likewise creates fresh storage for every representation change. |
| Ordinary append | `PyUnicode_Append:11708` allocates/copies nonempty operands; its empty shortcuts alias, and its error path clears `*p_left`. Leave this public contract intact. |
| Result normalization | `_PyUnicode_Result:717` may replace empty/single-character results with singletons. Such replacements remain ordinary; no permission follows the old pointer or flag. |
| Marshal/pickle/freeze | `Python/marshal.c:752` writes logical ASCII bytes or encoded bytes; readers at 1586/1631 construct normal strings. `unicode_getnewargs:13840` uses a logical copy. Deepfreeze emits logical characters and fresh static state. None serializes capacity or privacy. |

ASCII-to-Latin-1 is a separate transition even though both have kind 1.
An ASCII compact object uses `PyASCIIObject`, while non-ASCII kind 1 uses
`PyCompactUnicodeObject`; the UTF-8 alias and DATA offset change. For
example, adding U+00E9 to a private ASCII prefix must allocate a non-ASCII
result even if spare capacity exists. Checking only `kind` would be wrong.
Latin-1-to-UCS2 and UCS2-to-UCS4 similarly require fresh storage. Appending
cannot make an existing non-ASCII prefix narrower.

The trailer is not a temporary growth hint. After sealing, logical length,
terminator, capacity and canonical bytes stay fixed. Hashing
(`unicode_hash:12012`) reads logical bytes and caches the hash; UTF-8 cache
creation (`unicode_fill_utf8:5702`) allocates a separate encoding except
for compact ASCII's canonical alias. Both may operate on sealed flagged
objects. Interning may retain such an allocation, including its slack,
for its whole immortal lifetime. Do not clear the storage bit on these
transitions, and do not make a storage consistency check require private
conditions such as hash -1 or no UTF-8 cache after sealing.

`_PyUnicode_CheckConsistency:592` needs storage checks in addition to its
existing kind/content/terminator checks: flagged objects are exact,
compact and non-static, `L <= C`, and the requested extent is representable.
Its private-state checks belong in the owner invariant, not in the
permanent storage invariant. The footer is trusted only because the
audited producers set the flag; reading it cannot independently prove the
true allocation extent. A corrupt/accidentally propagated bit is unsafe.

## 4. Writers, frees and accounting

Writable writer buffers currently encode allocation capacity as their
**logical Unicode length**. `_PyUnicodeWriter_SetBuffer` sets `size` from
`PyUnicode_GET_LENGTH` and clears `readonly`
(`Include/internal/pycore_unicodeobject.h:127`). Its debug ownership
assertions only check hash/interning/immortality, not publication history.
Those conditions do not authorize writing a sealed capacity-bearing input.

The complete direct writable-adoption inventory is:

* `_PyUnicodeWriter_InitWithBuffer` (`Objects/unicode_writer.c:120`),
  called only by the UTF-8 and ASCII decoder paths at
  `unicodeobject.c:5290` and `:7398`; both pass a newly allocated ordinary
  `PyUnicode_New` result.
* `_PyUnicodeWriter_PrepareInternal:131`, whose new/widened buffers come
  from `PyUnicode_New`, or whose owned ordinary buffer is passed to
  `_PyUnicode_ResizeCompact`. Its internal `_SetBuffer` calls adopt only
  these results.

Keep that set ordinary. Make the exclusion explicit at writable adoption
and compact resize, with a retained failing control for accidental
flagged adoption. Do not teach the existing writer that a sealed capacity
bit makes the buffer writable. No extra copying is required for current
decoder callers.

Read-only adoption is different: `_PyUnicodeWriter_WriteStr:256` calls
`_SetReadOnly` for an exact input with an empty writer. That sets size/kind
to zero and `readonly=1`; the next prepare allocates and copies. Finish
at line 563 returns the same input without mutation. This path may retain
a sealed flagged object and must preserve its footer. Mutable finish
truncation (`:598`) and width narrowing (`:582`) remain ordinary-only.
The other `_SetReadOnly` caller is `_PyUnicodeWriter_WriteCharInline`
(`pycore_unicodeobject.h:163`), which adopts an ordinary cached Latin-1
character; it does not introduce a flagged writable buffer. Writer buffer
assignments outside the setters in this implementation only clear or
transfer existing ownership during prepare/finish/deallocation.

| Layer | Source | Consequence |
|---|---|---|
| Object request/counts | `Objects/obmalloc.c:1224`, `:1611` | Pass the full R to `PyObject_Malloc`; allocation-pressure bytes and size-bin counters receive that request. No hidden reserve is omitted. |
| Collector live storage | `Python/gc_trace.c:2143` | Live-byte accounting uses the allocator block size, which may include rounding/debug overhead. It is distinct from `__sizeof__`'s requested size. Exact Unicode is a leaf (`:1891`); the footer is not an owned object reference. |
| Ordinary free | `unicode_dealloc:1590`, `:1650`; `PyUnicode_Type.tp_free:14420`; `obmalloc.c:1671`, `:1699`, `:275` | UTF-8 cache is freed separately if present, then the original object allocation is freed via its pointer. Neither the Unicode free nor sweep-block free reconstructs size from logical length. No footer-specific free is needed. |
| Debug allocation/free | `obmalloc.c:3034`, `:3115` | Debug allocation records full R, places forbidden bytes after R and returns an adjusted data pointer. Free retrieves that recorded request and its matching underlying pointer. Use this API unchanged; never inspect its prefix from Unicode code. |
| Debug quarantine/realloc | `obmalloc.c:175`, `:264` | Quarantine realloc copies a block to a new allocation; this does not repair a lost/moved logical footer. The explicit compact-resize exclusion applies in this mode too. Normal free quarantine retains the whole block. |
| ASan | `Include/internal/pycore_mimalloc.h:43`; `obmalloc.c:345` | mimalloc enables `MI_TRACK_ASAN`; without mimalloc the default switches to malloc. Full R, including footer and slack, is inside the requested accessible region. No new manual poison/unpoison or usable-size scheme is needed. Configuration and runtime allocator-chain qualification still apply. |
| Python size | `unicode_sizeof_impl:13810` | Current formula uses `(L + 1) * k` and undercounts flagged storage. For flagged compact objects report R, then add the existing separate UTF-8 cache bytes with checked size arithmetic. Ordinary and legacy formulas stay as they are. |

Debug wrappers can reject a request near `PY_SSIZE_T_MAX` because their
own extra bytes also need to fit. Keep that normal allocation failure;
do not derive a platform-specific reserve limit from debug prefix offsets.
Speculative reserve failure follows the parent's exact-copy retry rule.
ASan alone cannot detect a wrong write inside valid spare capacity or an
overwritten footer. The future controls must verify characters, logical
terminator, metadata and requested-size accounting as well as red zones.

## 5. C access, physical reads and ABI boundary

| Consumer | Source and audit result |
|---|---|
| Compiled inline C API | `Include/cpython/unicodeobject.h:201`, `:247`, `:249`, `:265`, `:286`, `:300`, `:324`, `:344`, `:372`: existing predicates, KIND, DATA, LENGTH, READ/WRITE and MAX_CHAR use existing fields/offsets. READ_CHAR explicitly permits the logical terminator. They need no footer lookup or allocation. |
| Public writers | `unicode_check_modifiable:1316`, `WriteChar:4160`, `Fill:10525`, `CopyCharacters:1500`: the caller must own a newly created writable string. These modify bounded logical character positions, not length/capacity. A flag must not weaken that contract or serve as ownership evidence. |
| Equality/search | `Objects/stringlib/eq.h:21` compares exactly `len * kind`; `fastsearch.h:10` expressly permits reading `s[n]`; memchr/wmemchr calls carry explicit lengths. The required logical terminator remains present before metadata. |
| Chunked max-character/codecs | `stringlib/find_max_char.h:31` bounds whole-word loads by end and has scalar tails; `stringlib/codecs.h:44`, `:536` bound their word loads. `unicodeobject.c:4888`/`:4942` has bounded chunk/tail loading for ASCII scans. No inspected algorithm treats the capacity word as characters. |
| UTF-8/wide/UCS4 access | `unicodeobject.c:4073`, `:5702`, `:2456`, `:3247`; `Doc/c-api/unicode.rst:1071`: logical encoding lengths and terminators remain unchanged. Exported canonical/UTF-8 pointers must remain stable for the published object's lifetime. Widening/growth therefore allocate a different object; sealing cannot realloc or shrink it. |
| Native diagnostic reads | `_PyUnicode_Dump:1190`, `Python/traceback.c:900`; `Tools/gdb/libpython.py:1506`: these select existing headers/kinds and display logical characters. Retained offsets let them continue ignoring capacity. A dump may optionally report storage later; it must not infer privacy. |
| Remote header copy | `Modules/_remote_debugging/object_reading.c:51`, `:80`, `:148`; `pycore_debug_offsets.h:395`; `debug_offsets_validation.h:317`: the reader copies a whole `PyUnicodeObject`-sized remote prefix, then uses target length/kind/header sizes and copies only logical text into a fresh ordinary string. It does physically read beyond a short string's logical terminator already. It does not clone state into the result. Preserve remote layout/version checks; do not claim every physical read stops at L+1. |

The parent wording about no reads beyond canonical L+1 should distinguish
the public character-access contract from retained diagnostic/implementation
physical reads. The defensible claim is that **string results depend only
on the logical characters and terminator**, and every retained physical
read has its existing bounds/remote-memory contract. No extra character
or public accessible capacity is advertised. Platform libc vector code
is not proved instruction-by-instruction by this source audit; native
qualification must cover short/tail and all-width cases without treating
uninitialized slack as text.

For the ABI decision, check state size/alignment, existing state-bit
positions, all three header sizes, field offsets, DATA offsets and
character alignment on supported ABI/compiler combinations. Include
ordinary and debug C extension builds and compiled inline access. The
limited API uses functions and does not expose this struct layout; the
non-limited inline API and remote layout readers are explicit consumers.
An unchanged source struct size alone is insufficient evidence. No
compatibility promise is made for foreign code fabricating private
Unicode layouts or interpreting old unnamed padding as a named flag.

## 6. Compiler certificate storage and revalidation options

Decision for the next design revision: the
[S0 proof/owner prerequisite](pre-I-linear-string-proof-owner.md) selects
the dedicated optional **code-owned pure-data pointer**, passed through
the private constructor and installed before CREATE. It preserves lazy
`_co_exec` creation and survives DESTROY retention until actual deallocation.
The alternatives below remain the original source-analysis record, not
implementation fallbacks. S0 specifies owned transfer/failure handling,
explicit NULL on uncertified paths, accounting and the required H3/H1/
ROOTS reviews. No native/layout approval follows from this choice.

The compiler path is `optimize_and_assemble_code_unit`
(`Python/compile.c:1662`) through register conversion/passes/compaction
(`Python/regcfg.c:1405`, `:1432`), `_PyAssemble_MakeCodeObject`
(`Python/assemble.c:896`), `assemble_words`/`assemble_emit`, then
`makecode:488` and `_PyCode_New:599`. The internal instruction-sequence
assembly path (`compile.c:1947`) also reaches this assembler. It must earn
the same proof; the caller's identity is not a certificate.

The other path is public/unstable constructors and `code.replace` through
`code_new_with_fbinit` (`Objects/codeobject.c:736`, `:896`, `:2745`),
plus `PyCode_NewEmpty:963`; marshal `TYPE_CODE` constructs a normal
`_PyCodeConstructor` (`Python/marshal.c:1833`). These receive no ownership
certificate. Current scalar handler/slot/feedback checks are not escape or
CFG proofs. Serialization must not carry a trusted optimization flag.

`init_code` installs words, locals information and `_co_exec=NULL`, then
calls CREATE watchers **before `_PyCode_New` returns**
(`Objects/codeobject.c:462`, `:473`, `:485`). A watcher can observe the
object at that point. Attaching an allegedly immutable plan afterward is
not an unpublished initialization step.

| Storage option | Concrete interface/lifetime work | Assessment |
|---|---|---|
| Dedicated optional code-owned pointer | Add a core-owned pure-data certificate field to `_PyCode_DEF` (`Include/cpython/code.h:30`); compiler passes an owned verified plan through the private constructor/initializer, before CREATE. Every other constructor explicitly supplies NULL. Free the payload in `code_dealloc:2379`; include payload in `code_sizeof:2688`; static `_Py_InitCleanup` and deepfreeze initialize NULL. | Recommended clear S1 contract, subject to H3/code-layout review. It preserves lazy `_co_exec` construction but costs a field in every code object and shifts code layout; do not claim ABI neutrality. No arbitrary callback destructor or Python references are needed. |
| Execute-owned immutable payload, compiler eager path | Extend `_PyCodeExec` (`pycore_ir.h:153`) with a count/offset or pointer into one checked NoScan allocation. Build the certified execution copy during compiler construction, after final words and before CREATE, via a reviewed internal counterpart to `make_exec` (`ir.c:247`). Generic constructors still get uncertified lazy copies. Update `_ClearExec:318`, `_ExecSizeOf:332`, alignment assertions and `_PyIRStaticExec`/`_PyIR_InitCleanup` zero metadata. | Viable way to use the existing `_co_exec` owner without a new code field. It makes certified code eagerly allocate words/feedback and needs H3 publication/layout approval. Do not bolt a compiler plan onto an already published execution copy. Optional-allocation failure must decline certification rather than add an unreviewed new compilation failure. |
| Recompute a proof in generic `make_exec` | Derive and fully verify the ownership certificate from actual pristine words, constants, locals and exception/control-flow metadata every time an execution copy is constructed. Store it as above. | Avoids a stored compiler provenance marker, but needs the complete proof builder for arbitrary admitted IR. Current scalar validation is insufficient. This is more work than the initial compiler-only feature and is not required for S1. |
| Explicit registered `co_extra` slot | `PyUnstable_Eval_RequestCodeExtraIndex` (`Python/ceval.c:3540`) allocates interpreter-owned indices/freefuncs; Get/SetExtra (`codeobject.c:1557`, `:1584`) publish replaceable pointers. Define interpreter/sharing ownership, immutable attachment, accounting and callback lifetime. | No hidden slot exists for this task. Extras are replaceable, arbitrary free callbacks run before sweep (`_PyCode_Destroy:2365`), payload bytes are not counted by `code_sizeof`, and stateless-code documentation excludes extras (`pycore_code.h:437`). More contracts than a dedicated pure-data owner; not recommended here. |

Recommend the pre-CREATE call path. The exact candidate internal boundary
is an owned optional plan parameter from assembler `makecode` into
`_PyCode_New`/`init_code`; update all internal callers, with NULL on
non-certified paths. It is not a new Python-visible constructor argument.
H3 reviews the selected dedicated backing before implementation. The
original alternative with post-construction analysis would have to make
**permanent no-upgrade** explicit
once CREATE/publication has occurred; absence stays absence for that
object, even if no execution copy existed yet. This is less useful than
construction-time attachment and is not the recommendation.

With dedicated storage, `_PyCode_Destroy` must not discard the certificate
before its DESTROY watcher can retain/resurrect the code; the pure-data
payload lives until actual deallocation. Constructor failure paths first
initialize the pointer to NULL and define exactly when ownership transfers.
No Python callback/freefunc is needed to release this payload. Frozen-code
generation/initialization must be reviewed as consumers, without assuming
that the retained deepfreeze code generator is a current qualified path.

The minimum proof interface is a versioned immutable pure-data plan
containing bounded word/local indices and site permissions, not object
addresses, PyObject references, caller-provided flags or a runtime ownership
bitmap. Its final verifier compares the actual assembled pristine words,
physical slots, all ordinary/implicit reads and writes, exception edges
and synthetic relaxed jumps against the compiler proof. Constant merging
must not create a hidden assumption that a string constant is fresh.
Verify after the final representation-changing pass, before publication.

At execution, retain immutable proof identity and revalidate **permission**:
actual approved canonical handler/site and operand/target relationship;
active frame and permitted local; owner bitmap; exact representation;
frame exposure and observer/allocator gates. Specialization/instrumentation
changes the executing handler, not the pristine words (`ir.c:266`). Use
H3's approved generated semantic/canonical selection, never handler-number
cutoffs. Missing or mismatched proof declines the optimization. Constructor,
replace, marshal and frozen objects do not gain a certificate by copying
flags, words or a previous object's metadata.

The certificate owns no object references, so it needs no hidden GC roots.
The thread's owner bitmap names frame-local slots; reload referents and
DATA pointers from rooted slots after collection. Any lifetime for a
cached owner-frame identity belongs to H1's frame-storage contract. None
of these storage choices closes observer revocation, ROOTS' collecting
constructor contract or future movement by itself.

## 7. Bounded follow-up

S1 shadow ownership is a development checkpoint, not an independently
useful production landing with permanent hot-path overhead while mutation
is disabled. Keep the primary's production behavior unchanged until a
coherent useful fast path qualifies, unless retained shadow tooling has
zero production cost. Certificate layout/proof work may be reviewed in
isolation without promoting it to a production performance improvement.

Before committing to S2-S4, a separately authorized performance experiment
must compare the parent code with the proposed certificate/barrier path
under matched release builds, JITs off and the same collector/allocator
configuration. Include functions with no string concatenation, ordinary
calls/returns, tight backward-jump loops and frame/observer transitions,
as well as eligible appends. Record compilation/first-call cost, resident
certificate/bitmap bytes, actual barrier/slow-entry counts, release wall
time and instructions; show per-case uncertainty and slower cases. A
feature-disabled run that elides all barriers does not measure the
enabled path's overhead. Instrumented shadow runs can explain counts but
cannot substitute for release timing. No arbitrary ratio or opcode-count
threshold is introduced here, and this audit executes no such experiment.

1. Coordinator/Unicode/C-ABI decision on the permanent bit/trailer and
   explicit ordinary-only writer/resize contract. If accepted, S2 covers
   the finite producer table, consistency/sizeof and allocation-mode
   controls before any Python append uses it.
2. H3 decision on pre-CREATE immutable certificate backing; compiler owner
   specifies the final-word verifier and all constructor/static consumers.
   S1 remains shadow ownership with mutation disabled.
3. ROOTS/H1/string owners close the parent's callback/frame/export inventory
   before enabling writes. Empty breaker checks remain distinct from actual
   callback entry; no storage flag substitutes for that proof.
4. Later qualification must include ASCII-to-Latin-1 despite unchanged
   kind, all widths, read-only writer reuse versus forbidden writable
   adoption, seal/hash/cache/intern/free, requested versus live storage,
   debug/ASan footer integrity, and CREATE-watcher observation. Keep
   allocation/proof failures and unqualified constructor paths visible.

These are design prerequisites and future controls, not executed results
or approval to implement. The frozen parent design needs no silent edit;
the coordinator can accept or reject this addendum explicitly.
