# A handler numbering review

**Recommend and approve explicit format-bumped reuse of retired slot111 for `LOAD_ATTR_CLASSMETHOD`, with magic64418, for the bounded A increment.** No general-correctness obstacle was found. Keep one normal LOAD_ATTR family member; do not widen the specialization domain as an incidental classmethod optimization. This is numbering approval, not implementation/native post-review or permission to alter B during qualification.

Inspected runtime46's number table, analyzer/generators, specialization setter, IR decode/base/event mapping, instrumentation, disassembly and stats consumers. Existing B changes do not alter these conclusions. No runtime edits, builds, regeneration, native jobs or commits were performed.

## Concrete bounded path

1. In `Tools/cases_generator/ir_numbers.txt`, replace `# removed: STORE_FAST_LOAD_FAST 111` with a plain historical comment recording intentional format64418 reuse, followed by `LOAD_ATTR_CLASSMETHOD 111`. Do **not** leave the machine-recognized removed marker at111: `parse_table` correctly rejects active reuse of a still-retired number. Preserve the ordinary no-reuse rule for every other retired slot.
2. Add the real `LOAD_ATTR_CLASSMETHOD` macro to `family(LOAD_ATTR, ...)` and its guarded producer in `specialize_class_load_attr`, per the reviewed A semantics. Keep the existing uint8 specialization parameter:111 fits. No handler reservation at326, synthetic execution variant, wide-family generator path or expanded opcode tables is needed.
3. Update `Include/internal/pycore_magic_number.h` history and `PYC_MAGIC_NUMBER` to64418. `Lib/importlib/_bootstrap_external.py` derives its token from `_imp`; do not add a second literal. Let established regeneration/build update affected products and frozen dependencies, and require native preflight identities/magic to agree.
4. Regenerate all opcode/IR IDs, Python maps, names, targets, deopt/cache/selector/format/effect tables and generated handlers together. The normal family machinery emits `deopt[111]=LOAD_ATTR`, base78, specialized flags, class-attribute cache format, full handler name and dispatch target. No manual map patching or old-name compatibility alias.

The instruction is argument-bearing;111 is above HAVE_ARGUMENT41 and below MIN_INSTRUMENTED_OPCODE233. `assign_opcodes` explicitly permits specialized family members in `[1,233)`, rather than requiring numbers above RESUME128. `MIN_SPECIALIZED_OPCODE129` is currently only defined/generated; no consumer found uses it to reject a low-numbered family member. Existing specialization mechanisms already permit specializations outside the conventional contiguous range. Family metadata, not a guessed numeric interval, supplies the semantics.

`IR_NUM_HANDLERS` and 16-bit word width do not grow when using111. Stats arrays/pairs and handler-domain metadata already span the IR handler domain. Specialization success/failure can continue indexing the existing 256-entry deopt map safely. Instrumentation's `word_opcode` normalizes through `_PyIR_EventOpcode`/`_PyIR_BaseOpcode`, so LINE/INSTRUCTION and ordinary instrumentation operate on LOAD_ATTR and rewarm rather than preserving a classmethod-specialized original byte. Disassembly already uses generated contract base/name data and the specialized opmap.

## A semantic gates remain separate

The number does not change A's ownership proof: first scope exact builtin type metaclass on class receivers, method-form load, exact builtin classmethod descriptor and a live exact Python function payload; no free-threaded specialization. Every hit guards the metaclass before reading the guarded descriptor. Acquire the current `cm_callable` at lookup and transfer the accessed class as self; CALL uses that captured pair, never rereads the descriptor after argument evaluation. Store descriptor-backed `class_value` in snapshots, not the mutable wrapped function. Attribute-form retrieval and unsupported cases stay generic.

Use the existing LOAD_ATTR A/B/C/D/feedback geometry; no new frame/call ABI. The new macro must preserve the family output/window contract and checked pair acquisition/ownership. Failure before acquiring outputs falls back with original owners intact. Compiler/generator proof failures are real blockers, not grounds for a handler-name exemption.

## Why not widen to an IR-only specialization now?

The runtime already has useful wide infrastructure: 16-bit handlers/setter, IR metadata/base/event maps, IR names/disassembly, stats sized by `IR_NUM_HANDLERS`, fused IR-only families and execution-only ownership variant319. This supports a future general path; legacy tier1 compatibility is **not** an admission gate.

However, a genuinely new IR-only family member is not currently a drop-in use of those mechanisms:

| Area | Required coherent wide-path change |
| --- | --- |
| `analyzer.py` | Stop assigning an IR-only member in a real family to the byte-specialization range; preserve its family association while separating real opmap membership |
| `py_metadata_generator.py` | Avoid indexing `analysis.opmap[name]` for a wide family member; publish it through the IR map and retain specialization/base relationships |
| `ir_metadata_generator.py:shadow_of` | Give the new member a valid ordinary shadow/base; a self-shadow326 fails current `_PyIR_BaseOpcode` assumptions |
| `specialize.c:specialize` | Accept int/uint16 handler; normalize stats/failure family through `_PyIR_BaseOpcode` instead of indexing `_PyOpcode_Deopt[326]` |
| Guard generation | IR guard exits must assert against the normalized IR base; common emitter currently emits `_PyOpcode_Deopt[opcode]` |
| Tables/tests | Keep byte-only generated tables byte-only, wide domain complete, and prove instrumentation/deopt/dis/stats round trips with mixed real/IR families |

Execution variant319 is generated by reusing an existing real specialization body and canonical identity. A classmethod pair has different semantics from the ordinary class-value reader, so pretending it is only a local/temp refinement would misuse that mechanism. Fused families are IR-only as whole families, not a first wide member of an otherwise byte-numbered family. Neither example proves widening just one C parameter is enough.

These changes are feasible but constitute a general generator/domain migration. They remove a future numbering constraint, not additional classmethod wrapper work. Explicit retired reuse is smaller and has an actual precedent: format64414 repurposed slot87 for argument-bearing CLEANUP_THROW. Choose the general wide path when its domain contract is the task; avoid introducing it solely to preserve an arbitrary vacant-bytecode policy after a format revision is already authorized.

## Serialization decision

Normal compiler output remains generic LOAD_ATTR; warmed specialization and feedback are not marshalled. Nevertheless, assigning an old retired number a new valid meaning changes the declared handler/format revision, and the number-table policy explicitly prohibits reuse within one revision. **Bumping magic is the clean explicit boundary**, even though this new specialized handler is not expected in ordinary serialized compiler words. Old pyc headers must reject under64418 and recompile; old golden controls and their cache directories remain unchanged. No hard compatibility promise or retained STORE_FAST_LOAD_FAST interpretation is needed.

Do not weaken `ir_numbers.parse_table` to permit duplicate live/removed entries, teach the checker a special111 exception, or advertise unchanged format while recycling a retired number. No feedback size change is needed: ATTR48/POLY120 remains the contract.

## Independent host probe and required qualification

A host-only temporary input added a **dummy normal family member** at111 to actual DSL and a private parsed number table. It passed analyzer numbering, identical LOAD_ATTR IR format, generated `_PyOpcode_Deopt` entry, Python specialized map111, IR specialized flags and base LOAD_ATTR. The dummy body tested numbering only, not classmethod behavior. Temporary input was deleted; runtime source/products were untouched.

Implementation tests must pin ID111 and magic64418, verify deopt/base/name/cache selectors and family format, exercise adaptive disassembly and stats counts, and enable/disable LINE/INSTRUCTION/CALL monitoring after specialization. Cover deopt/re-specialization and serialization/header rejection/roundtrip using established importlib/code tests; verify original `co_code` remains generic. Update any retired-hole expectations from generated current number metadata, not by silently deleting the invariant.

Root owns regeneration, release/debug/stats builds and focused/API/import/monitoring tests. The actual A implementation still needs Sol ownership review and generated/native post-review. Scope approval is limited to this explicit111/64418 numbering choice; wider IR-domain work remains deferred.

## Review of the author's concrete A component proposal

Read [A-implementation-proposal.md](A-implementation-proposal.md) after the numbering audit. **GO for that scoped component**, with two contract clarifications conveyed to author/coordinator: preserve the shared LOAD_ATTR writes extent `1 + (oparg & 1)` rather than globally declaring a fixed-two-output family; handler111 admits method bit1 and therefore realizes two outputs. Also select the explicit reader-side free-threaded miss before descriptor/target borrowing, in addition to producer decline, as the smallest unambiguous enforcement of this GIL-only live-target acquisition.

The proposed existing `_PyClassMethod_GetFunc` is the correct allocation-free borrowed getter. Per-hit builtin-metaclass then class-version and exact-descriptor checks precede the live NULL/exact-function guard; every miss leaves the receiver/window intact. Only then duplicate the target and TAKE the actual accessed class. No callback-capable release or failure follows acquisition before the pair is complete. This preserves inherited subclass binding and captures the old target before explicit arguments may mutate/reinitialize/delete its descriptor. A's snapshots retain the descriptor, not `cm_callable`; ordinary CALL handles later code/default/provider mutation of the captured function.

The method-bit assertion is valid only because the producer admits that immutable form; tests must retain that gate. Attribute-form retrieval, instance receivers, metaclass/descriptor subclasses and arbitrary callable payloads decline. No additional ownership or numbering blocker was found. These are proposal findings; the actual pair-emission/source diff remains a separate post-review.
