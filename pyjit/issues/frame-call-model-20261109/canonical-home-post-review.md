# Canonical-home duplication: actual source and result review

Disposition: approve the bounded source for engineering integration and the
coordinator's isolated component measurement. No concrete must-fix found.
Reviewed the current runtime diff against417a8bd6e2e3: generator, macro comment,
exact generated cases, generator tests and new runtime test module. No runtime
source edit, candidate execution, build or regeneration by this reviewer.

## Source proof and ownership

The selector is renamed consistently to CanonicalResult/canonical_result_source;
there is no stale numeric-only fallback or second store emitter. The added
proof accepts one pure scalar uop with no external stack inputs and one actual
LOCAL_IN field whose encoded immediate selects GETLOCAL(oparg). It requires the
exact assert + owned PyValue_DUP(GETLOCAL(oparg)) body. The actual first output
must be the unique final nonaliased output. An instruction name is not evidence.

Only Uop and unnamed Skip padding are allowed. Named caches are rejected before
recognition. Stack bindings shadowing audited home helpers are excluded; exact
body equality excludes local helper rebinding, callbacks, index changes,
result mutation and suffix work. Preprocessor tokens fail closed. Tests cover
renamed positive source and helper/reference/cache/directive mutations. Their
explicit FB_HEADER fixture setup supplies the synthetic feedback layout without
relaxing the body or binding proof. The original failed fixture receipt remains
retained; it was a test setup failure, not permission to ignore a named cache.
Trusted GETLOCAL/PyValue_DUP header semantics and canonical owning frame homes
remain explicit source boundaries, as for the existing canonical return proof.

The duplicated value owns its count and retains the canonical home's exact
bits. Only the final destination macro changes from Publish to the already
shared canonical transfer. All local/discard/temp destination bodies, displaced
owner handling and close ordering remain the same IR_STORE_DST_IMPL. A folded
local alias still duplicates before displacement; a temporary still owns its
copy across callback replacement; a discard closes only that duplicate while
the source home keeps its owner. No early publication, input consumption, new
borrow or observer authority is introduced. Observer-deferred output handling
still overrides this selector through its existing commit path.

The actual generated diff contains exactly two store-line substitutions in
LOAD_FAST and LOAD_FAST_BORROW. Constructor/error paths and the prior numeric
producer proof retain their full unchanged-suffix checks. No metadata/table,
format, interpreter-frame layout or code-cache format changes occur.

## Executed evidence and limits

Read canonical-home-host-checks-v2.log: four selected host methods pass,
including real-source recognition and prior numeric proof/mutations. Its actual
all-handler comparison identifies only LOAD_FAST and LOAD_FAST_BORROW changed;
285 other handler bodies remain unchanged.

Read canonical-home-minimal-{release,debug}.log:25 tests pass in each role,
covering the four new runtime cases, operand modes11, discard completion9 and
the new generator method. The tests independently witness a real temporary
LOAD_FAST_BORROW and folded-local LOAD_FAST, numeric/subclass value behavior,
callback replacement, owned payload identity and eventual weakref collection.
Both coordinator native builds complete117 modules. These are focused source/
ownership gates, not a new full-suite qualification.

Read canonical-home-assembly/summary.json: both selected symbols shrink from
134 to85 static disassembly instruction lines; the recorded two PyLong_Type and
two PyFloat_Type references per symbol become zero. This confirms removal of
the redundant publication type-test path in these binaries. It does not measure
executed instructions, elapsed time or a workload gain. The before/after ELF
hashes remain in that receipt. Dynamic component results must come from the
coordinator's unchanged workload protocol after integration.
