# Shared phase/output contract progress

2026-11-07. Owner: Sol design and independent review; 60-minute first-result
timebox. Runtime `7780e32e0e2` is read-only while the coordinator captures a
fresh baseline. No native edits, builds, tests, benchmarks or commits by this
owner.

Source inspected: the five specialized attribute producers in
`Python/bytecodes.c`; `IREmitter.retire_published`, `IRStack.flush`,
`handler_contracts` and the generated metadata path in
`Tools/cases_generator`; current `LOAD_ATTR_SHAPE` generated C and its existing
generator fixture; `value_reader_complete`, `retire_published_values` and
`_PyValue_Publish`. The current five-name branch supplies both the local-only
failure result and the final-store suppression. Word-level effects do not
express that postcondition.

Delivered: `contract-proposal.md` declares a source-level result/retirement
fact, uses one checked source contract for emitter and metadata, and replaces
the five-name branch. It specifies first-output temp/local/error/callback
semantics, role/position/effect-based suffix checks, negative fixtures and a
separate known-canonical cost hypothesis. Independent publication review
accepted the amended semantic design. The generic completion helper's actual
publication-before-close order was rechecked in C; it shares the semantic
phase while earlier generic lookup remains able to escape.

Also delivered: accepted source reviews of the call-record cleanup and the
outlined Python-call fallback, with no speed claim; a `value_owners` warning
audit; and a canonical `jit-e2e-system.md` amendment specifying Action,
FrameState, ownership, effect phases and a small recovery checker. The
coordinator owns code integration, builds, tests and measurements. No native
edits, builds or benchmarks were made by this owner.
