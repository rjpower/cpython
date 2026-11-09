# Versioned remote layout contract

Use one fork descriptor with an invariant 48-byte prefix: eight-byte cookie
`pyjitdbg`, then uint64 schema version, descriptor byte size, Python version,
ABI identity, and free-threaded mode. Schema 1 identifies this descriptor;
ABI identifies native endianness, pointer width and PyValue width/format.
Readers copy only the prefix first and reject cookie/schema/size/Python/ABI/
threading mismatches before copying the complete descriptor. The descriptor
remains immutable while a process runs. No local-offset fallback is supported.

Export code co_nlocalsplus/co_stacksize and stack-chunk struct size, previous,
size and data offsets. Validate each exported offset before use. Chunk readers
use these values, including local-copy address translation. Await lookup reads
the target code's two counts, rejects negatives/overflow, then requires
2 <= return_slot < nlocalsplus+stacksize and return_slot != IR_NO_DST before
multiplication or address addition. Check address overflow independently.

Discovery, runtime initialization, profiler readers, command injection readers,
layout validators and tests move together. Source-only observer overrides and
the initialized arena remain separate followups. The async-for assertion names
the associated actual SEND through deoptimization-safe opcode recovery.

## Active source callback display: proposed bounded consumer port

Source-only LINE callbacks already publish a cursor on the observed
PyFrameObject: source_line_event saves f_source_event/f_lineno, sets the record
index and its line, calls tools, then restores both. The executing frame keeps
the real predecessor PC. The landing LINE wrapper explicitly publishes prev
before invoking source_edge_events; branch and semantic-completion hooks also
observe at their real source instruction. Nested callbacks can inspect this
observed ancestor. A successful debugger redirect clears the cursor before
changing its PC. The executable offset and the displayed source span are
therefore distinct facts; no virtual public instruction offset is introduced.

Export interpreter_frame.frame_obj and one frame_object descriptor section:
target struct size, f_frame, f_lineno, f_source_event. Include these offsets in
the existing full-descriptor size/prefix gate and normal offset validation.
Do not export or use f_source_event_error: that selects unwind, not an active
source callback. Existing descriptor consumers reject the changed exact size;
all repository readers and descriptor tests must move together.

Remote frame parsing reads the optional target frame object only after checking
the pointer span and exported size. Require its f_frame backlink to equal the
actual target interpreter-frame address. Carry the candidate cursor and line
to code parsing; do not treat a local copy's address as the target address.
For an active candidate, decode the already fetched code linetable container,
bound the record index, require event.from equal the actual compact PC, require
event.line equal the published f_lineno, and validate the source span. Only
then replace displayed LocationInfo with that record's full span. Keep opcode
identity and f_lasti tied to the compact executable PC. An inactive or
inconsistent snapshot uses the existing executable location; it must never
index an unchecked record or adopt an unrelated frame's line.

The remote code cache currently keys frame output only by code and PC. Bypass
that output-cache lookup and update for any active cursor candidate, including
a candidate later rejected as inconsistent; immutable code metadata remains
cached. Otherwise two source records at one predecessor would share a stale
display location. This adds no ordinary interpreter work.

GDB uses debug-symbol field access, so it needs no local-layout assumption or
descriptor fallback. Its current_line_num should inspect the associated frame
object and the same bounded IRSM record, applying the same backlink, cursor,
predecessor and line checks. Its existing f_lasti and instruction display stay
unchanged. Factor record decoding beside its current location-table decoder;
do not invoke target functions or create target objects while debugging.

Focused coverage should stop inside a source-only trace callback nested beneath
the observed frame and compare remote/GDB display with callback frame.f_lineno,
while checking f_lasti names a real executable word. Exercise two distinct
source records sharing one PC to catch the output-cache bug, ordinary inactive
frames, redirected callbacks, and invalid cursor/backlink/span through the
existing isolated descriptor/target mutation setup. GDB coverage follows its
existing debugger availability conventions. Root owns native tests/builds;
this proposal has not changed runtime source.
