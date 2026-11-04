# Review 2 of `register-ir.md`

Reviewed 2026-11-03 at python-jit `29135d0ad14` (1,928 lines). The document
is pinned at runtime `ca8a9d6d827`; the tree is now at `e704ab282d9`, which
changes no file the document cites. This review checks the 14 changes that
review 1 required, judges the four points the author disputed, and checks §4
and §5 for internal consistency.

## Verdict

**Accept after five small fixes (R1–R5 below).** All 14 required changes are
addressed: eleven fully and three partly. The disputed points are argued well.
In three of the four I agree with the author. The fourth, the feedback-index
encoding, is left undecided and should be decided now.

The §4 word budget adds up to 64 bits for every format listed. The §5
contract covers the effects the interpreter needs. It does not yet cover all
the effects that `jit-e2e-system.md` asks of it: abstract heap reads and
writes, allocation, guards with the feedback fields they read, result facts,
and the timing of deoptimisation relative to side effects.

One internal contradiction remains. §4.1 and G10's option (c) still say that
code with more than 4,095 names raises `SyntaxError`. §1.14 and decision O4
say it compiles to an intrinsic call.

## The 14 required changes

| # | Required change (review 1) | Status | What is missing |
|---|---|---|---|
| 1 | Re-pin; mark G1 landed; fix the small errors; re-read §1.4 | accepted | – |
| 2 | Ownership as two facts; rename G2's bit | accepted | IR-D3 adopts it, and the bit is now called the "temporary bit" |
| 3 | Instruction contract and generated table | **partly** | The section and the decision (IR-D2) exist; the columns in finding 6 are missing |
| 4 | State at a word boundary; liveness decision | accepted | Liveness and OSR are deferred to the JIT stage (IR-D4), which is acceptable. Two rows need edits (finding 8) |
| 5 | One jump-target rule; conditional back edges | accepted | The eval-breaker poll makes "may run Python" depend on the sign of D (finding 6) |
| 6 | Remove dead handlers | accepted | Checked: `TARGET(EXTENDED_ARG)` and the four superinstructions exist and are never emitted |
| 7 | Correct the comparisons in G6 and G7; split G7 by consumer | accepted | – |
| 8 | Omit `PUSH_NULL`; give unpacking a source field and a generic path | accepted | Small points in finding 9 |
| 9 | G5's dependency on G10; evaluate the offset encoding | **partly** | The dependency is fixed. The encoding choice is left to O4; decide it (dispute D4 below) |
| 10 | Re-measure limits on generated code; compare removing NOFB with a hard limit | **partly** | The contradiction in finding 1, and the maximum number of compare sites, which is not yet measured (finding 3) |
| 11 | Accumulator trade-off, NULL-store cost, `NOT_TAKEN` gap, missing rows | accepted | – |
| 12 | Bit-budget table; batch the format changes | accepted | IR-D1 and §4 |
| 13 | Direction of the 0.871× result; scripts in the repository | accepted | The corpus is not reproducible (finding 10) |
| 14 | Open decisions; interfaces for the JIT | accepted | §6 and §7 |

## The four disputed points

**D1. A hard limit on feedback sites.** The author is right. 84 generated code
objects pass 4,095 sites, and failing to compile valid Python is not
acceptable. That gives a principle the document should state once, in §1.14:

- a limit that real code reaches is handled by lowering the instruction or by
  sharing entry 0;
- a limit that no corpus reaches (slots, words) raises `SyntaxError`.

Apply the same principle to compare sites past the limit (finding 3).

**D2. Where the temporary bit goes.** The author's placement is better than
both of review 1's alternatives. Putting the four class bits in bits 12–15 of
the handler half-word keeps 12-bit slot fields, which the generated maximum of
2,856 slots needs. It adds no AND to slot accesses. Twelve bits still give 13
times the current handler count. Three conditions remain:

- The bit duplicates a fact the code object already holds: a field names a
  temporary if and only if it is at least `nlocalsplus`, and hidden constant
  slots count as frame variables. That is acceptable as a cached decode fact
  only if the verifier checks it and the bits of non-slot fields are zero.
- Every reader of the handler field must mask with `0xFFF`: `IR_HANDLER`,
  `_PyIR_SetHandler`, `INSTR_OPCODE`, `dis`'s `_unpack_words`, the JIT
  decoders and the instrumentation side arrays. List them.
- State a decision rule for the control experiment. If keeping `nlocalsplus`
  in dispatch state recovers the cost, the bits are not needed. The JIT knows
  `nlocalsplus` at compile time, and D4 concerns value tags, not slot classes.
  The bits would then go, and the handler would keep 16 bits.

**D3. One handler per compare operation.** There is no disagreement. Review 1
also objected to option (b). Option (c) is right: names move into the word,
compare operations stay in their entries, and those entries are assigned
first.

**D4. Offset encoding of the feedback index.** The author measured it fairly.
In 8-byte units, one Lib code object and 160 generated ones overflow, against 0
and 84 with a per-site index. 16-byte units lose most of the saving. But the
document leaves the choice open ("decide with O4", and "either way" in §4.1),
and a reference should not.

Recommendation: adopt 8-byte offsets, and assign entries in priority order.
Compare and contains entries come first, because they are required for
correctness. Sites inside loops come next, and module-body and class-body
sites last. Overflow then falls on code that runs once, which is where §2.5
says the generated overflows are. The saving is about 23% of feedback memory,
roughly 13.7 MB on Lib if every function ran.

## Consistency of §4 and §5

The bit budget holds. The handler (12), the class bits (4) and fields A, B, C
and D (12 each) make 64 bits, and CD (24 bits) overlaps C and D. Every format
in §4.2 fits four fields:

| Format | Fields |
|---|---|
| `CALL` | `{DST or 0xFFF, BASE, argc, FB}` |
| `UNPACK_SEQUENCE` | `{BASE, count, FB, SRC}` |
| `UNPACK_EX` | `{BASE, SRC, CD}` |
| `MAP_ADD` | `{BASE, depth, SRC, SRC2}` |
| `JUMP_IF_<cmp>` | `{lhs, rhs, FB, D}`, with the operation and the sense in the entry |
| `LOAD_ATTR`, `STORE_ATTR` | name in D |
| `LOAD_SUPER_ATTR` | name in D |
| `LOAD_GLOBAL` | `{DST, FB, CD name}` |
| `JUMP_BACKWARD` | `{FB, –, CD}` |

## Findings

1. §4.1 rules; G10's options table, row (c). Wrong.
   - Finding: both say that code past 4,095 names gets `SyntaxError`. §1.14
     and O4 lower such accesses to an intrinsic call. §4.1 also lists
     constants under `SyntaxError`, although constants sit in CD (24 bits) and
     no corpus reaches that limit.
   - Required change: make §4.1 and the G10 table agree with §1.14 and O4, and
     state D1's principle once.
2. G5, O4, §4.1. Weak.
   - Finding: the encoding of the feedback index is undecided.
   - Required change: decide it, per D4, with the priority order of
     assignment.
3. G10, option (c). Missing.
   - Finding: a code object with more than 4,095 compare and contains sites
     has no rule. Neither `COMPARE_OP` nor the fused form can run without an
     entry, so `COMPARE_OP_NOFB` cannot be removed unconditionally.
   - Required change: lower such sites to `CALL_INTRINSIC_2`, with one
     intrinsic per operation and the bool bit. That adds intrinsics, not
     handlers. Measure the maximum number of compare sites on the generated
     corpus.
4. G5; `bench/ir/static_counts.py`, `KIND_BYTES`. Wrong (small).
   - Finding: every BRANCH entry is counted as 16 B. The `POP_JUMP_IF_FALSE`
     and `_TRUE` entries are 24 B (UNARY layout), so the per-kind total is low
     by up to about 0.6 MB, and the saving is closer to 22% than 23%. The size
     of G3's fused entry (BINARY plus history) is not given either.
   - Required change: split BRANCH by family, recompute, and give the fused
     entry's size.
5. G5, G10, offset encoding. Missing.
   - Finding: entry 0 under 8-byte offsets has no stated size or rule.
   - Required change: entry 0 is header-only (16 B), and a generic handler at
     an entry-0 site touches only the header (counter and history). Assert
     this.
6. §5, contract columns. Missing (for the JIT).
   - Finding: the columns cover what the interpreter's passes need. Measured
     against `jit-e2e-system.md` ("Effects distinguish scalar purity, guarded
     reads, writes, allocation, exceptions, callbacks, reference releases,
     suspension and observation"), they lack:
     - (a) the abstract heap read and written (attribute, item, global and
       builtin dict, cell contents, `f_locals`);
     - (b) may allocate;
     - (c) for each specialised handler, its guards and the feedback fields
       they read;
     - (d) result facts: the type or representation of each output;
     - (e) the invariant that deoptimisation precedes any side effect, which
       the restart on the same word needs, checked by the generator;
     - (f) the monitoring events fired;
     - (g) inputs passed through rather than consumed (`COPY`, `FOR_ITER`'s
       iterator);
     - (h) instructions with two targets (`FOR_ITER`: `END_FOR` and the word
       after it; `SEND`).

     "May run Python" must also include the eval-breaker poll on the taken
     side of a backward fused jump (signals, pending calls and the scheduled
     GC run Python code), which depends on the operand, like the release of
     an old dst.
   - Required change: add these columns, or say which ones the JIT design
     owns instead.
7. G2, consequence of D4. Weak.
   - Finding: once no home holds a borrow, every `LOAD_FAST_BORROW` into a
     temporary takes a count for a mortal object. This is a cost moved into
     IR v2, before baseline, and its size is not given.
   - Required change: count `LOAD_FAST*` words that write a temporary,
     dynamically per kernel, with the §2.4 scripts.
8. §1.4, "State at a word boundary". Weak.
   - Finding: two rows are incomplete.
     - The `instr_ptr` row says it is stored by 289 of 292 handlers, but G9.1
       proposes storing it only on escaping and error paths. The contract
       should say `instr_ptr` is valid at observation points, and that a JIT
       exit sets it.
     - The table has no row for G6's constant slots, which hold their
       constants from entry onward and must be preserved by OSR, JIT exits
       and generator resume.
   - Required change: amend both rows.
9. G4.3, G4.4, G6. Weak.
   - Finding: three points are underspecified.
     - With a SRC field, what `LIST_APPEND`'s BASE and depth denote: if the
       value comes from SRC, the window is only the list.
     - The generic unpack path collects into a C array, which conflicts with
       G9.6 (no address-taken locals) unless it is an out-of-line helper.
     - For G6: which code-object table lists the constant slots and their
       values, who fills them (frame push, or `RETURN_GENERATOR` for
       generators), and a cap so that constant slots never push a frame past
       4,095 slots.
   - Required change: specify each.
10. §2.5, corpus. Weak.
    - Finding: "wheels in the local uv cache" is not reproducible.
      `run.py` writes the file listing to a temporary directory and does not
      keep it.
    - Required change: keep the listing, with content hashes, next to the
      scripts or as an artifact.

## Changes required before acceptance

- R1. Remove the `SyntaxError`-for-names contradiction in §4.1 and G10, and
  state D1's principle in §1.14 (finding 1).
- R2. Decide the feedback-index encoding (8-byte offsets with priority-ordered
  assignment), fix the BRANCH size in the script, and state the rule for entry
  0 (findings 2, 4, 5).
- R3. Give a rule for compare sites past the limit (an intrinsic lowering), and
  measure their maximum on the generated corpus (finding 3).
- R4. Add the missing contract columns, or assign them explicitly to the JIT
  design, and make the backward-jump poll part of "may run Python" (finding
  6).
- R5. Add the decision rule for G2's control experiment and the list of
  handler-field readers that must mask (D2).

Findings 7–10 can be done during the IR v2 workstream; they do not block
acceptance.

## Not verified

- The C frame sizes in G9 (35.6 KB, 5.5 KB, upstream 360 B).
- The new §2.4 slot-traffic columns.
- The §2.5 generated-corpus numbers. I read the scripts but did not run them.
