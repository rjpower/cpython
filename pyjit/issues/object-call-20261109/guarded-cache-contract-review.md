# Guarded cache metadata contract review

**Approve the revised proposal's separate guarded storage for the immediate gate repair. Current43 remains API-gate rejected.** A single metadata ADT is architecturally defensible only with the fail-closed contract below, but its checker complexity is not required to preserve this optimization. That alternative remains deferred. Admission requires independent review of the actual diff and tests, followed by full qualification. A weaker exception is not an alternative.

Reviewed `guarded-cache-union-proposal.md`, the current `_PyFbAttr`/`_PyFbPayload` declarations, `_PyValueGuarded` helpers, `fb_row_of`/`fb_put_row`, the NONDESCRIPTOR producer and consumers, and the checker's WORD_TYPES/WORD_HOLDER/rawunion rules and finding handling. No runtime/checker edits or native jobs were performed.

## Finding and policy

The checker deliberately bans unions containing owners, transient borrows, or guarded words, including nested holders. The two reported findings are therefore real violations of the implemented policy. Earlier reviews—including mine—accepted the handler-selected metadata representation without checking that conservative union rule. Passing native tests does not waive it.

`_PyValueGuarded` preserves a canonical word as non-owning metadata. It neither roots a mortal object nor grants a lasting borrow. Loading it creates a transient borrow only while an independent owner/version proof holds; receiver retirement must still follow result acquisition. A native C specialization-descriptor pointer is not such a language value and must not be converted into one to simplify storage.

The inspected producer writes `class_value` for NONDESCRIPTOR and an object pointer for method cases; `fb_row_of` selects the corresponding member by opcode, and poly storage copies the resulting metadata alongside the recorded handler. These operations are consistent with an externally discriminated union, not evidence of reading guarded bits as a native pointer. That distinction justifies considering a narrow ADT contract. It does not justify changing the policy to permit arbitrary guarded-word unions.

## Why the proposed repair is preferable to a name exception

One private payload type with object/native-descriptor/guarded-value/shape views makes the metadata abstraction explicit. Typed accessors keep canonical words from being exposed as pointers or integer encoding and keep the guarded-to-borrow conversion separate. Reusing this type in mono and historical feedback removes duplicate layout definitions while preserving the footprint.

The exception is justified only by the combination of exact layout, exact accessor semantics, restricted access, and explicit discriminator mapping. An allowed typedef name, an allowed file, or an allowed function name supplies none of those proofs. Keeping `_PyValueGuarded` in WORD_TYPES is mandatory; ordinary raw-bit, cast, nested-union, borrow-storage and ownership rules remain in force.

The four-view schema intentionally contains a `uint32_t` shape index. “No raw integer alias” must mean no additional encoding view or reinterpretation through that shape view; it must not ambiguously ban the required scalar field. The checker should validate its exact intended type/member rather than accepting arbitrary scalar alternatives.

## Implementation requirements

1. **Validate before exempting.** Resolve the declaration's canonical identity and spelling location, exact union/containing-struct field schema, and exact permitted accessor signatures/bodies. A malformed or unknown AST contract must retain the original failing finding. Do not postprocess findings by basename or line number. Keep unions that merely contain this ADT subject to the general nested-word rule.

2. **Close access escapes.** Reject direct private-member access outside verified accessors, including same-header code and macro arguments. Reject member addresses, cross-view return/copy, alternate initialization, pointer reinterpretation and raw bits inside an otherwise approved-name function. Consider whole-payload address escapes to untyped write/copy functions in the actual usage census; a typed setter must not become decoration around a parallel byte-buffer API. Whole-payload assignment remains legal metadata copying, not authority to load a different view.

3. **Tie selectors to transitions.** Document and test reset → active-member write → specialized-handler publication for mono feedback, and member selection → payload copy → handler publication for historical feedback. Cover generic/no-payload, failed/partially populated observations, and respecialization across object/guarded/shape kinds. The active discriminator may remain the existing handler; no new runtime tag is needed. A reader's return type alone cannot establish that the matching member was written. The exact mapping and producer/generator/snapshot usages must be part of the reviewed contract, with one authoritative mapping where practical.

4. **Preserve guarded lifetime meaning.** A metadata accessor returns `_PyValueGuarded`, not an owner or borrow. Existing hit guards and safe result publication remain required before `_PyValueGuarded_Load` and receiver retirement. Copying stale historical metadata must neither load it as a borrow nor close it as an owner. Do not widen NONDESCRIPTOR admission or change free-threaded behavior as part of this repair.

5. **Keep the checker change bounded and falsifiable.** Retain existing owner/borrow/array/wrapped-union negatives and add guarded-word equivalents, modified schema/accessor bodies, same-file and macro access, forged declaration identities, altered selector mappings, and an outer union containing the ADT. Test exact positive snapshots for each legitimate view. These checks enforce this finite internal protocol; they must not be described as a general proof of C union dataflow safety.

The implementation review must inspect how clang-query output establishes canonical types and identities, not merely the tests' expected diagnostics. If that requires a general C dataflow engine or leaves declaration/accessor enforcement as a fragile name filter, the bounded approach has failed its feasibility gate.

## Fallback and qualification

Separate guarded fields are the recommended low-risk immediate repair: retain the native-only attr union and add an independent guarded field; make each snapshot payload a struct with its native-only union plus the independent guarded field. This adds 8 bytes per attribute entry (40→48) and 32 bytes per four-row poly table (88→120). It preserves the existing blanket rule and simplifies reasoning. Its footprint may matter, but unmeasured cache growth is not grounds to weaken the representation boundary. Measure and account for it. Preserve selected-member snapshot semantics and keep guarded fields absent from native unions at every nesting level.

The revised proposal correctly identifies the feedback-capacity effect: larger attr entries can reach the existing 4096-unit limit earlier and change private-feedback admission for later sites. Verify alignment/offset generation and overflow/fallback behavior, not only `sizeof`. Count actual metadata bytes and admitted sites for affected workloads; review poly capacity accounting through `sizeof(_PyFbPoly)`.

For a future approved ADT, retain the 40-byte attr and 88-byte poly assertions; for split storage, update them to the reviewed sizes. Regenerate affected cache operands and inspect all changed users. Run checker positives/negatives, the full API gate, real release/debug builds and focused scalar/object/polymorphic/lifetime tests under a new source identity. Any changed generator output or access sequence also needs compiled/performance review; equivalent field layout does not guarantee equivalent emitted code. Retiming should include the class micro and logging/DeltaBlue/coroutine controls, retaining their instruction/coverage evidence.

Current43 evidence remains attached to its original source and its two failing API findings. A corrected successor cannot inherit a green API result or silently replace those records. The fix resolves a representation-policy issue; it does not resolve the outstanding application regressions or authorize promotion.
