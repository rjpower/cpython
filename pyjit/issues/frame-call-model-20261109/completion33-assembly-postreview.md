# Decoder33 compiled post-review

Read-only `nm`, `readelf` and `objdump` inspection of root's release33 build.
No candidate was executed. Before is retained
`E11/ninth-workload-perf-record/candidate-python`, build ID
`c22c55552d50db3a77068f1f4ecaae14149c2079`; after is the release build, ID
`aa8bc6736a361277d268307a0cd5baa64fdb6d00`. Full ELF hashes and symbol-body
hashes are retained in `E11/completion33-assembly-review/symbols.json`.

`_PyEval_CompleteGeneratorReturn` is 343→340 bytes. The two real calls to
`_PyIR_BaseOpcode` and `_PyCode_InstrOparg` disappear. The VM route loads the
immutable word and HandlerBase table, extracts signed D, computes the physical
continuation from return_pc, and calls CompleteStep. Its remaining deallocation
and interpreter-parent redirect paths are present; the compiler rearranged
their code, so this is not an assertion of byte-identical interpreter code.

`_PyEval_CompleteStep` remains 1858 bytes and has identical complete raw symbol
bytes before/after (SHA256
`caac230afb006e461ca238ac2170e2531102d448396cc5b8d964aba47cee6d96`).
The source diff changes only the decoder and two focused tests. No generated
provider or metadata changes were made. This inspection verifies intended
compiled scope, not a performance improvement.

Root's first native gate found a test-selection mismatch: exported row.base
uses BaseOpcode, retaining the instrumented FOR ID. The fixture now includes
positive canonical-family twin links while preserving every geometry and
membership assertion. H independently approved this fixture-only correction;
the failed first receipt remains retained. Native v2 results are root-owned.
