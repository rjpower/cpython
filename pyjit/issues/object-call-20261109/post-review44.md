# Candidate 44 independent-storage post-review

**The initial storage diff is internally consistent, but its first release build exposed a required serialized-format correction. Approve the magic-version fix below before rebuilding.** The checkout was already clean at `5a3f468b8a9` when first reviewed; that reviewed diff is its parent `47dc23f48cf` to the commit. It changes exactly the feedback header and three tests, matching the approved split-storage proposal. No runtime files or native workloads were changed or executed by this review.

## Build finding: serialized feedback needs a new magic number

The coordinator's first release build failed during frozen-module preparation with `ValueError: co_fbinit malformed`, while importing an existing pyc. My initial post-review checked in-memory allocation and malformed-feedback tests but missed the producer-format version: `Python/marshal.c` serializes the raw `co_fbinit` records, whose offsets and total units encode the old five-unit attr geometry. The new six-unit validator cannot accept those records under an unchanged cache identity.

Approve bumping `PYC_MAGIC_NUMBER` from 64416 to 64417 with a history comment identifying the 48-byte attribute feedback format. Update both exact expectations in `Lib/test/test_import/__init__.py` (the numeric magic assertion and `PINNED_MAGIC_AND_TABLE`), not just the header. The handler-number table itself is unchanged, so its pinned digest remains unchanged. Source search found no other relevant hardcoded 64416/64417 expectations in runtime tools or canonical checkpoint/object-call measurement helpers; unrelated numeric test data must remain untouched.

`importlib._bootstrap_external.MAGIC_NUMBER` derives from `_imp.pyc_magic_number_token`. Its standard header check rejects an old magic before unmarshalling code, allowing normal source-backed regeneration. Do not delete caches to hide the mismatch, weaken feedback validation, or relabel old records. Raw standalone marshal code data has no pyc-header compatibility guarantee; this fix does not promise to load it across the format change. Frozen/generated artifacts still need the normal coherent rebuild.

The source/header/test update needs a new identity, fresh compilation and import/marshal validation. Existing source-backed pycs carrying 64416 should exercise rejection/regeneration; normal current-format roundtrips must pass. Measurement snapshots need not demand equal magic across historically different controls, but must continue to capture each executable's correct source/build identity and unchanged inputs.

## Representation and initialization

### Candidate 45 format and executed-check follow-up

Reviewed clean successor `55b2a87cf51d29f8880114f4a73c9945e9ca73ec`. Its diff from 44 is exactly the approved magic header/history update and both import-test numeric expectations. The handler-table digest is unchanged. No additional runtime behavior or checker change is present.

Retained `object45b-build-checks.json` records release and debug returncode 0 at this clean source identity; both build logs report 117 checked modules, zero missing and zero failed imports. The corrected focused campaign now also completes on **both** builds: ten modules, 564 tests run and 13 skipped, SUCCESS. Its native preflights report `55b2a87cf51` and magic 64417. The reviewed records are `object45-corrected-focus-checks.json` and the corrected release/debug focus logs. This includes import tests, feedback geometry/capacity, cache codegen, call transactions and ownership/completion cases.

The initial focus attempt selected nonexistent `test_ir_native_call_transaction` and returned 2. It remains a rejected diagnostic; the corrected selection uses `test_pyvalue_native_call_transaction` and includes the cache-codegen module. Do not combine the failed attempt with the successful campaign or omit its reason.

At this follow-up, `closing45-qualification-commands.json` records the API job started but has no completed API/stats/full-suite results. Those checks, fresh capacity/performance evidence, and any explicit old-pyc regeneration witness remain pending; the successful build/import suite is not a claim that every proposed validation has run. The earlier 44 API result applies to 44's unchanged representation checker, not an independently completed 45 gate. Source review approves continuing qualification, not promotion.

`_PyFbAttr.class_value` is now a separate `_PyValueGuarded` field after the native pointer/shape union. `_PyFbPayload` is a struct containing a native-only descriptor/shape union and its own separate guarded field. Neither union contains a word or nested word holder. No checker rule, word type, encoding exception, admission condition, or owner conversion was changed.

The declared sizes are consistent on the established 64-bit profile: attr 48 bytes, payload 16, poly 120. The poly arithmetic is three four-element uint32 arrays (48 bytes), four uint16 handlers (8), and four 16-byte payloads (64). The guarded attr field follows the existing 32-byte prefix and 8-byte native union at offset 40. Native members retain their offsets; consumers of `class_value` use its new declared offset.

Initialization remains complete without additional edits:

- `make_exec` zeroes the allocated feedback block using the computed feedback size.
- `fb_begin_ex` clears the entire attr body using `sizeof(_PyFbAttr) - sizeof(_PyFbHeader)`, so the new field resets too.
- `fb_row_of` starts with `fb_row row = {0}`. Its opcode-selected snapshot fills the relevant native or guarded member; the independent inactive field remains zero.
- `fb_put_row` copies the complete payload struct. It does not load guarded metadata as a borrow or close historical data as an owner.
- Poly allocation, initial clearing, growth and memory accounting use `sizeof(_PyFbPoly)`.

Thus stale/non-active metadata is not newly interpreted. The existing handler-selected snapshot and version/lifetime proof remain necessary and unchanged. The guarded result is still acquired before receiver retirement. This repair resolves forbidden storage overlap without weakening the semantic contract.

## Capacity and test geometry

Assembler feedback allocation uses `fmt->fb_size >> IR_FB_UNIT_SHIFT` and bounds each entry by `unit + size <= IR_FB_UNITS_MAX`; generated metadata supplies `sizeof(_PyFbAttr)`. Six 8-byte units therefore replace five. Overflowed optional sites retain shared entry 0, whose unreachable counter prevents specialization.

The new 700-site test is arithmetically correct: two header units plus 682 six-unit entries gives 4094 units, leaving two unusable units below 4096. Private offsets `range(2, 4094, 6)` contain 682 values; 18 sites share entry 0. The test checks byte length, actual private NONDESCRIPTOR specialization, generic shared sites, and a type-value mutation. This covers behavioral admission as well as a size constant.

The existing mixed test starts attr allocation at unit 10 after its required compare entries. `10 + 681 * 6 == 4096`, so 681 private attr sites fit exactly. Its updated second offset 16 and shared-site count are consistent. Other required feedback families remain independently allocated; this is not a blanket 682-site limit for arbitrary functions.

The `co_fbinit` fixture changes also preserve their purposes: one attr beginning at unit 2 needs total 8; total 7 is too small; the reordered two-entry fixture needs total 14; header overlap, excessive limit and missing header units remain rejected. Contract tables update LOAD_ATTR, STORE_ATTR and LOAD_SUPER_ATTR together. Searches found no relevant stale 40-byte attr or 88-byte poly constants in the current runtime/test/generator usages inspected. The separate 4095 assertions in frame-call tests describe the discard destination, not a feedback size, and correctly remain unchanged.

## Evidence and remaining gates

Read `E/object44-release-lint.log`: its final result is 306 value-native units, **0 failing findings**, **0 parse errors**. Non-failing home-copy/slot-read/unchecked notes remain and must not be described as zero total diagnostics. No checker exemption was introduced.

At the initial 44 review, release/debug compilation and focused execution were pending. The candidate 45 follow-up above records their actual successful completion after the format correction. These results are supported by build/focus records, not inferred from the earlier clean API log.

The layout change needs fresh capacity/coverage and performance evidence under the new native identity. Count actual metadata bytes and any changed private-site admission; include class lookup, logging, DeltaBlue and coroutine controls. Generated metadata references `sizeof`, so a header-only runtime change can correctly alter allocation and emitted offsets without textual regenerated differences. Confirm the normal generator check nevertheless; do not manually patch generated offsets.

Candidate 43 remains the measured API-rejected checkpoint. Candidate 44's clean API check resolves those two representation findings, not its predecessor's application regressions or the remaining full qualification. No promotion recommendation follows from this source post-review.
