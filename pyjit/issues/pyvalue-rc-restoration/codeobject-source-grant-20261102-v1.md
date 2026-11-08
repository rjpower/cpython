# Canonical code-object constants and validation

Adopt D's critical review of amended codeobject-canonical-consumer-cut-v1.md
in build/pyvalue-rc-restoration-design-20261101-v1. Root implements; D reviews.
Distinct35-minute source cut: conservative start2026-11-02T02:23:00Z,
concrete checkpoint02:33:00Z, ceiling02:58:00Z. Record actual first edit.

Only intern_strings/intern_constants, validate_and_copy_tuple, inline_const_fits,
code_returns_only_none, one private equivalent-value tuple replacement and
canonical exact-Unicode preflight; precise name-access hunks in
get_localsplus_names/code_new_with_fbinit/GETITEM; focused new codeobject tests.
The accepted typed-key producer remains exact. No public API removal, type/
compiler/VM edits, tracing-GC work or runtime selector.

Raw code.replace name tuples are unvalidated. Preflight canonical object kind
and exact Unicode for varnames/cellvars/freevars BEFORE any object metadata
access/comparison; raw invalid values retain non-string-slot SystemError.
CodeType's separate validate_and_copy_tuple retains Unicode-subclass conversion
and nonstring TypeError. Constructed co_names metadata is separately proven.

Own constants/containers through callbacks and interning. Publish equivalent
representatives before displaced close; shared/hash-key tuples retain typed
equality/hash/cached hash. No uniqueness assertion, public setter weakening,
borrowed array across allocation or scalar object borrow. Frozensets enumerate
owned canonical values and rebuild only under original modification rules.
Inline constant handler eligibility matches actual inline int32/non-NaN float;
changed external constants fall back before effect. Canonical None stays pure.

Checkpoint actual canonical interning/name validation and syntax diagnostics;
focused invalid four-name-field, shared NaN/hash/typed constants, frozenset and
warmed replace witnesses are AST-only until candidate execution. At most2
meaningful actual syntax TUs per revision, no shims/suppression. Excluded
free-threaded intern code and generated Clinic remain explicit unclaimed gaps.
No objects/link/candidate/runtime/timing in this source cut.

Actual first source edit observed no later than02:25:31 UTC; checkpoint1da89ffd
at02:30:51 meets02:33. Narrow necessary source handoff also includes
code__varname_from_oparg_impl: owning GetItemRef replaces GetItem plus NewRef,
preserving returned ownership and checked index errors. No public getter change.
Final300b559c strengthens existing seven tests with distinct preinterned anchors
and external inline-handler construction/retention/fallback assertions.
Exact source projection initially omitted its real localsplus helper; that
diagnostic is preserved, and adding the unchanged actual definition yields a
passing projection. Full original TU still fails generated Clinic getters.
Source is held for independent review under the unchanged02:58 ceiling.

Window closed early after final fixture correction0aadc5b2. Independent C
reviewa864f8e6 and D's additive correctionca1fc0a9 accept the final source/test
bindings. D's earlierc80058b4 production review remains valid, but its v2
encoding-witness claim is withdrawn: encode_words accepts base opcode names
only, while inline handlers occur in the specialized map. Final tests pass
numeric IDs from that actual map; peer host controls reject the old names and
accept IDs203/204 with unchanged operands. Seven methods remain unexecuted.
Production/typed-key source is unchanged; prior syntax evidence carries exactly.
