# Numeric result publication: next measurement question

Read-only inspection of the retained measured `6ef45e391aa` ELF and immutable
golden ELF. No implementation is admitted by this note. The initialized arena
and semantic completion batch must first build and pass its focused checks.

The first screen retains nbody/golden 1.05559 and float/golden 1.02804. Nbody's
main `advance` function does many arithmetic and unpack operations inside its
20,000-step loop but few Python calls. Removing call-frame initialization alone
does not establish a cause or repair for that loss.

The measured increment added scalar result sinks to `IR_STORE_DST` and guards
to temporary publication. Every used result currently passes the sink guard
too. In retained `_TAIL_CALL_BINARY_OP_SUBTRACT_FLOAT`, the candidate's
symbol-relative `+0x1cf` begins a destination extraction followed by
`not/and/je` testing the sink value before ordinary publication. The golden
handler has no sink test. This is real added work on a numeric operation;
the supplied instruction excerpt does not quantify its workload frequency or
explain the entire nbody regression. Register allocation, spills, other changed
paths and instruction-cache behavior remain possible contributors.

After the next integration:

1. Repeat the material broad losses and inspect actual warmed numeric handler
   selection. Count dispatches separately from PMU instructions and cycles.
2. If nbody still loses, collect matched counters and profiles before changing
   publication policy. Preserve the workload and GCC configuration.
3. Review whether the local-result path can precede the sink test, and whether
   a result is needlessly published twice. This is a bounded implementation
   experiment, not permission to remove checks from unknown native results.
4. If a broader result policy is needed, define it in the IR/generator contract.
   Known canonical numeric constructors can support provenance, but handler
   names alone cannot prove canonical results. Any eliminated publication needs
   a producer proof and mutation tests comparable to the return proof.

A frame-owned sink home would avoid a sentinel guard but retain an unused
owner until overwrite or frame retirement, require a properly hidden owning
home, and change observer/escape/constructor rules. Uniform generated sink
variants could move the policy to assembly but increase handler/code size.
These are alternatives with different costs, not selected next changes. Do not
add a second representation, ad-hoc dispatch aliases or a hot ownership map.
