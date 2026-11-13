# Combined R2 + validated S2: independent native review

Source/native GO for the combined mechanisms, not performance admission.
The clean source is `f3c464a54c89783e50af1ce9f3cffec6f5e7279d` in
`BASE/worktrees/frame54-combined`. Root regenerated without a new generated
diff. Both `combined-initial-{release,debug}-build.log` finish with117 modules
checked,79 shared and zero failed imports. Reviewer operations were source
reads, nm/readelf/objdump and non-running GDB type queries; no inferior,
generation, build, test, profiler or timing was run.

| Executable | SHA256 | Build ID |
| --- | --- | --- |
| BASE/frame54-combined-release/python |b5bd23d06f08b34b4b61fe821baa81bd3074be22395523ae9ef4484fb6344e98 |33be79d9a1cc7ccd3b4bc347e60461d135be107f |
| BASE/frame54-combined-debug/python |9595257f3eacb341ad0ca9a55b975b844c7789cdffdc3fcc95e0db5c1fd07358 |46aee1f63b0ed507a00886104280ce9d1b02b1d1 |

Both actual GIL ELFs have fixed header112 bytes, sizeof120 including the
declared trailing home, with owned_slots at112. This combination makes no
header-size reduction. Cold-source ownership review remains separate from
these GIL machine-code observations.

## R2 remains intact

Release exact CALL begins `0x37a8b0`. The fresh child receives packed
NONE/pending_kwargs constant0x0fffffff at `0x37aab2`; the original R child
co_nlocalsplus classification remains absent. The real caller destination
still compares against its execution-prefix local boundary at `0x37ab02`,
sets/shifts the replace bit and stores the descriptor at `0x37ab10`.
There is one initialized reservation and no added allocator on the exact path.

RETURN_VALUE begins `0x381650`. Its discard equality against0x0fff is at
`0x3816d7`; numeric masking and replace-bit test follow at
`0x3816e4/0x3816ee`. Empty publication directly stores at `0x3816f7`
without reading the old destination home. Replacement alone loads the old
owner at `0x3817c0` and commits the new owner at `0x3817c3` before retirement.
There is no release NONE/validity predicate or silent sentinel fallback.
The redirect branch calls CompleteRedirect at `0x38174f`, then rereads the
winning descriptor before publication.

Entry slot1 remains the native packed0x0fff0001 store at `0xa7ab5`.
CALL_ALLOC_AND_ENTER_INIT still classifies actual shim slot1 against the
shim prefix at `0x381c25` and stores that descriptor at `0x381c40`.
Neither entry nor trampoline inherits NONE.

The actual cold constructor is820 bytes at `0x170ad0`. Its complete native
instruction sequence matches R2 after normalizing addresses/alignment:
origin allocation/failure precedes ownership move, explicit first-input
discard precedes Copy, the f owner is acquired, all subtype metadata precedes
association/GC tracking, and stolen f retirement follows. No duplicate
subtype-wrapper mutation reappeared. Source lifecycle and failure-path gates
remain those in `recipient-default-post-review.md`.

## S2 remains intact

The overwrite variant begins `0x344920`, size296 bytes. Both inline guards
remain at `0x344953/0x344956` and `0x34496f/0x344972`. There is no A/B or
B/C geometry check, B sentinel test, B local-boundary check or geometry
fallback. C's required local/scratch classification remains at `0x3449a8`.
Its sole home NULL store is `49 c7 07 00 00 00 00` at `0x3449ae`, to C.
B receives the result through `48 89 3b` at `0x3449b5`, symbol offset+0x95,
without an intermediate NULL store.

Overflow branches at `0x34498a` to boxing at `0x344a1b`; the result NULL test
at `0x344a28` precedes both home changes. Success rejoins the commit route,
while failure tail-enters error at `0x344a43`. No call, allocation or error
edge occurs between the final C clear and B result store. Type-guard misses
retain their ordinary feedback/deoptimization path with PRE homes intact.
Generic ADD_INT remains available at `0x356010`, size642.

The variant's entire76-instruction sequence, excluding alignment nops and
normalizing relocation/branch addresses, matches qualified S2
`e95328ba1d6f3ea74dd92d0870213367287d64ef`. Its successful commit and overflow
spills are unchanged; rsp local reserve remains32 bytes. The specialization
function likewise matches S2's normalized sequence and495-byte size, with
complete geometry bounds paid on selection. Common constructor validation
remains8081 bytes. No per-dispatch geometry work was reintroduced.

## Text and stack comparison

| Release body | Qualified component | Combined |
| --- | --- | --- |
| Exact CALL (R2) |989 bytes /80-byte rsp local reserve |989 /80 |
| RETURN_VALUE (R2) |547 /16 |547 /16 |
| RETURN_GENERATOR (R2) |1070 /16 |1086 /16 |
| YIELD_VALUE (R2) |501 bytes |501 |
| Generic initialization helper (R2) |7484 bytes |7484 |
| Cold constructor (R2) |820 bytes |820 |
| Overwrite variant (S2) |296 /32 |296 /32 |
| Generic ADD_INT (S2) |642 /48 |642 /48 |
| specialize (S2) |495 bytes |495 |

Exact CALL, RETURN and cold constructor match R2's normalized instruction
sequences. RETURN_GENERATOR has the same227 non-padding instructions and
register operands; its16-byte growth is additional loop alignment padding,
not added frame work or spills. These comparisons normalize absolute/RIP
relocations and branch offsets and remove alignment nops; they do not claim
literal ELF byte identity or dynamic instruction counts. Relative to
selected51, R2's real producer encoding and S2's selection/admission/text
costs remain as recorded in the component reports.

The debug ELF retains assertions and is not claimed instruction-equivalent
to either component: its overwrite variant is1510 bytes, exact CALL1940,
RETURN1230. Both-role header geometry is directly verified above. Root owns
focused merged behavior, allocation-failure qualification, successful-variant
census and balanced unchanged-workload measurements. No elapsed-time cause,
hardware explanation, overall speedup or admission follows from this review.
