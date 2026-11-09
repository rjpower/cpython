# Format and layout fixture repairs

The `87f13087ed9` release diagnostic separates stale format fixtures from
behavior failures. This proposal covers only the former. Existing tracing,
debugger, monitoring and exception-behavior assertions remain unchanged.

1. `test_dis` has ten failures caused by obsolete loop/send cleanup words,
   old generator recipients and shifted real offsets/labels. Regenerate only
   affected static disassembly/instruction fixtures from the native interpreter,
   preserving source-relative line substitutions and the existing comparison
   machinery. Review the actual instruction/edge differences against semantic
   FOR/SEND completion, then run the whole dis module. Do not calculate expected
   output from actual output during a test.
2. `test_peepholer.test_except_handler_label` expects five optimized frontend
   instructions. The actual six include a source-origin carrier after
   SETUP_FINALLY; label resolution must include it. Update that static frontend
   expectation and retain the handler-target assertion. Register assembly still
   emits only real executable operations. Its three failing debugger tests are
   separate behavior work, not fixture repairs.
3. `test_exceptions.test_missing_lineno_shows_as_none` supplies a fixed old
   location table spanning eight words, which no longer fits its compact code.
   Construct one no-location marker per actual 64-bit instruction instead. Keep
   the assertion that the traceback line is None.
4. `test_sys` must describe actual frame fields: five int line/cursor fields,
   two uint64 generation/epoch fields, and an interpreter header with thirteen
   pointer-sized fields and two uint16 fields. Derive struct strings from the
   declarations, retaining independent size checks for frames and generators.
5. The embedded frozen-main header encodes obsolete marshalled IR. Make its
   generated target depend on the current native interpreter and input/generator
   sources. Include the build-root header explicitly so out-of-tree builds use
   the declared dependency rather than a stale source-neighbor header. Preserve
   in-tree builds and use the native compiler, then check frozen-main embedding
   in both candidate builds. Golden is unchanged.

Root owns these files and the mechanical fixture processor. Workers own
frameobject and instrumentation repairs. Independent peer precedes edits;
native validation and a source/result post-review precede integration.
