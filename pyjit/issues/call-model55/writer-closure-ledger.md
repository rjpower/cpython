# Writer closure after checked array loops

Source checkpoint: tuple/list packet `2c27c4bd2e9`, following loop packet
`948577105fb` and fixed/helper packet `99560a5115e`. P is synced to integration
`72f9a06be14`. B remains a separate writer-validation implementation; these
commits do not establish a stale-safe runtime or a complete static map.

## Completed bounded source packets

The first packet replaces six fixed-output/helper uops with Storage-bound
operations. The second replaces tuple/list unpack output loops and SET input
draining. Loop induction is typed zero-to-declared-extent with one matching
operation on every normal backedge. Skip/duplicate paths, mutable counters,
raw addresses and repeated fixed-index publications reject. Ordinary helper
escape handling stays separate from manual publication protocols.

Root and GC give actual loop source GO. Three parser-only checks pass. Root
owns emitted storage/metadata checks and runtime fixtures. A checked array
operation proves its own footprint; it does not certify unrelated statements
or a whole-frame initialized shape.

## Remaining source obligations

| Actual writer family | Present mechanism / remaining gate |
| --- | --- |
| Generated scalar inputs, local outputs, final publication, window vacate, SEND and CALL role holes | B has consume/store/vacate and role-aware bit hooks. Preserve them when merging incoming emitter changes; compare generated writes against actual event receipts and all result/discard/local variants. The native interpreter-exit capability remains separate. |
| Fixed arrays and consuming adapters | P now has WRITE_OUTPUT, TAKE_INPUT, string/map and special-pair adapters. Remove B's copied nine-body bridge only after these actual source contracts and final-vacate/family checks compose. No marker grants arbitrary frame aliases. |
| Tuple/list builders | Implemented in `2c27c4bd2e9`: checked markers lower to the same native helpers; a parameterized success suffix requires NULL failure to retain inputs, then INPUTS_DEAD and native-result adoption before actual final vacate. No wrapper, extra argument or duplicate sweep. Native helper escape policy is inherited. Root owns final emitted/native/fault qualification; not every const-array call inherits this drain. |
| Other array uses | BUILD_SLICE and RAISE_VARARGS borrow through value bridges; RERAISE reads a tagged index; BUILD_INTERPOLATION uses scalar/conditional format storage. These are not unported stores merely because an array name occurs, but their reads, consumption and exceptional cleanup require their own existing protocols. `_POP_TOP_OPARG` / CloseStack and UNPACK_EX / generic unpack need actual emitted drain/prefix coverage. |
| Direct local/child stores in bytecodes | MAKE_CELL, COPY_FREE_VARS, unicode target detach, getitem/property/getattribute independent children and EH lasti/exception installation have strict B frame writes. Static source contracts must retain their exact local/child capability; the new array checker does not cover them. |
| Call/binder/provider/init helpers | `initialize_locals`, defaults/varargs/kwargs, exact physical moves, role promotion, provider result publication, init shim and unlinked reservations have B Write/Move hooks. Audit source-to-destination aliasing and callback-free publication together; arena bits, typed header pins and native owners are distinct. |
| Ceval alias helpers | kwargs materialization, finish_unpack_to_locals, generic unpack prefixes and call result/native-scope completion have B hooks. Input pointers alias arena homes; no after-the-fact rescan substitutes for detach-before-close. |
| Frame lifecycle | initialization/constants, selected copy/created copy, return/resume recipient, local-proxy mutation, redirect save/restore and retiring drains have B hooks. New general-return extent and delegated lifecycle packets must be merged without restoring full-N inactive reads. |
| Frame/GC diagnostics | B's full count-tag/bit comparison is a NULL-control validation oracle. Before stale words, restrict it to positively readable homes and positive bit owners. Physical NULL assertions and debug dumps are readers, not evidence that stale words remain valid. |
| Synthetic storage | Fake frames in C helpers, embedded generator probes, public frame/gen allocation and extension consumers require actual bitmap tails/binding before any strict writer. Root's first build is not a substitute for this constructor census. |

A lexical scan of P bytecodes finds no remaining direct assignment or pointer
increment store through a declared array element after the loop ports. This
does not cover aliases passed to C, scalar/local emitter stores, implicit
children, cleanup helpers or generated code. Therefore it is not writer
closure evidence on its own.

Remaining array consumers need actual Storage extents and ordinary escaping
analysis. Do not add whole copied opcode bodies. The bounded known-home refinement is implemented in P `6a80182f670` and
staged in the reviewed B merge; it retains actual current C values
and physical vacancy in the commit API. B already selects counted bits for precise roots and private drains. The next
representation gate is removing remaining NULL work and closing non-counted
copy/traverse readers; see bitmap-authority-assessment.md.

GC gives bounded source/policy GO to this ledger. It is a queue of concrete
obligations, not an assertion that every row has complete event coverage.
