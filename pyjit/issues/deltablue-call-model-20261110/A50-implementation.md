# A50 implementation contract

Astra approved A in [the grouped review](astra-proposal50-review.md). This
implementation preserves the three ordinary method-layout guards and their
ownership behavior while carrying canonical representation through capture.

Nomination records the existing canonical descriptor in the separate guarded
feedback word. Execution acquires its owner after lookup guards, then transfers
or duplicates the canonical receiver owner. Historical rows copy guarded words,
never reinterpret them as raw pointers. Feedback geometry is unchanged.

The DSL declaration `IR_CANONICAL_OUTPUTS(attr, self);` certifies multiple
canonical owning outputs independently of receiver retirement. Its initial
bounded grammar is a terminal scalar producer with sole unconditional output
assignments: `PyValue_DUP(_PyValueGuarded_Load(class_value))` or `TAKE(input)`.
The latter input must survive unchanged from a canonical register. Prefixes
may only preserve those stack values through nonescaping guards. The analyzer
rejects aliases, arrays, suffix transformations, reassignment, address escape,
helper shadowing, preprocessing and unrecognized calls. Declarations fail
closed with an analysis error. No instruction-name allowlist is used.

The generator retains the fact per output and uses canonical temporary stores,
including repeated publication, while retaining debug home/owner checks. It
does not remove increments or transfers. The existing first-result publication
and receiver-retirement contract remains independent and unchanged.

Focused host tests cover the declaration, renamed producers, real method
handlers and false declarations. Runtime tests exercise warmed method variants,
mutation/shadowing, temporary receiver ownership and historical rows. Workers
do not regenerate shared products or build native targets. The coordinator
regenerates, compiles release/debug, reviews emitted code and measures A before C.

Worker validation: host `test_ir_attr_cache_codegen` and
`test_generated_cases` pass all 110 tests (77.738s), using
`UV_CACHE_DIR=/home/power/.cache/uv uv run --no-project python -B` with `Lib`
appended to `sys.path` after interpreter startup. Prepending this modified
stdlib is incompatible with the host interpreter. The first combined run had
one synthetic guard test lacking its receiver-object proof; that test was fixed.
Peer findings about compound/nested guard writes and indirect callbacks are
fixed and covered by negative tests. Runtime source is frozen for coordinator
qualification; source `git diff --check` passed before regeneration.
