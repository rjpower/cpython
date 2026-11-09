# First stopped-state consumer increment

Proposal only after native29 gates. Implement SOURCE_POST for an ordinary
executing frame in inherited handled context, targeting a save-empty location
with no required scratch inputs. Retain existing PRE behavior. Caught exits,
retained-root reconstruction, actual suspended frames and yield cancellation
are separate packets. No new interpreter-frame or ordinary-dispatch state.

B owns frameobject and focused code tests. Introduce one request-owned
RegisterStoppedState: actual owner site, exact physical continuation, edge,
context and borrowed/owned save-top/origin rows. PRE borrows Analysis rows.
SOURCE_POST copies the exact site's rows once and applies the existing normal
transfer for the selected edge. It never edits the global landing row. Refactor
the existing action builder and Plan consumers to take the view; do not build
another liveness/context engine. Reuse current token generation, binding-safety,
warning reproof and detach/install/commit-before-close rules. Preserve separate
physical availability and old-origin facts; produced values cannot resurrect
old saved tokens.

A owns source-provider scopes. Before authorizing SOURCE_POST, classify the
actual call site: a landing wrapper after completed dispatch and a terminal
committed provider can prove publication; a legacy inline hook with later
stores cannot. The scoped record must carry this positive contract, rather
than B inferring it from public event offsets or the SOURCE_POST spelling.
The minimal interface can make SOURCE_POST itself the strengthened provider
contract and mask all unproved source hooks as UNSUPPORTED. A chooses the exact
checked factory/argument after a complete source_edge_events call-site census;
there must be only one scoped authority, not a second setter-side event rule.
Display cursors and ordinary source callbacks still work at unsupported hooks.

Validate the scoped frame/site/next/edge/generation against the actual source
record and published cursor. Distinguish semantic landing from immediate real
relaxation helper. Source-record geometry proves association but not dynamic
owner generation. A same-generation supported stop supplies one exact edge
view. A changed generation needs the existing independently proved fresh PRE
landing, never old edge facts. Callback-error attribution and pending source
recipe invalidation remain the native29 mechanisms.

H owns shared physical-home/outcome helpers and constructor contract tests.
For this packet, expose/reuse the current selected-home decoder as read-only
facts; do not add a second shadow schema. Unknown physical availability or an
old-cut required root refuses. No weakening of the constructor's ordinary PRE
versus source POST distinction is needed. Any new provider requires its
positive output-commit/encoded proof before B consumes it.

Minimal tests: source-only active comprehension exit restores bound and truly
unbound locals; raw/folded/attribute unsafe targets still refuse; same-PC replay
and setter-then-raise retain exact target precedence; two ingresses to a shared
landing cannot borrow the other edge's save state. Add one legacy inline source
hook negative to prove event spelling cannot authorize unfinished output. Keep
existing expected line/event lists. Root runs focused release/debug gates and
constructor witnesses before widening to retained roots or YIELD_READY.
