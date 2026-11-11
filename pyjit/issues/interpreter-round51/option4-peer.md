# Option4 proposal review

Approved for isolated implementation. Reviewed the existing ADD/SUB producer
chains, canonical_result_source geometry and canonical_body_tokens AST binding,
and the scaled-int/boxed-int API contracts. The existing two inline operands
make their unscaled sum or difference fit int64; overflow of the exact scaled
operation proves that result needs a boxed representation. The existing
nonoverflow scaled constructor is canonical as well. Retaining that complete
join is a suitable proof for canonical result publication.

Conditions:

- Match the two input guards, corresponding add/sub overflow builtin, matching
  unscaled arithmetic operands/operator, scaled result constructor, NULL error
  branch, owner forwarding and both unchanged integer retirements. Never treat
  arbitrary BoxInt64 calls as canonical or infer the proof from opcode names.
- Bind exact original source tokens to the emitted AST, preserving fail-closed
  rejection of preprocessing, helper shadowing, added calls, altered output
  shape/aliasing and changed retirement. Keep the existing outer scalar format
  and feedback-binding exclusions. No new value API or C primitive is needed.
- Keep this helper disjoint from option2's window iteration proof: option4
  enters only scalar canonical_result_source; option2's producer result is
  selected separately after a complete window proof. Integration must retain
  both tests and neither should silently override the other's facts.
- Negative tests must discriminate changed overflow predicate from changed box
  expression, since the safety precondition depends on their relationship.
  Include both boundaries, opposite overflow directions and unchanged error/
  displacement handling. Allocation-failure coverage is conditional on a
  reliable existing fixture; do not add a test-only runtime mode.

Root owns regeneration, release/debug compilation, runtime checks and native
inspection. Require disappearance of generic result normalization while keeping
overflow allocation and failure branches. This is a source proposal approval,
not executed qualification or evidence of a speedup.
