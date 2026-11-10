# Disassembly fixture and capacity census peer review

## Fixture: approve after the current freeze

The proposed single expectation update in `test_dis.test_show_caches` is correct. The feedback allocation sequence is header 2 units, loop FOR_ITER 3, loop ATTR 6, loop CALL 4, then GLOBAL 4/CALL 4/GET_ITER 3. New entry starts are therefore 0,2,5,11,15,19,23; instruction order yields **[15,19,23,2,5,11]**. The poly-table size does not affect these offsets.

Read the 143-test diagnostic result with eight failures and checked the source assertion and subsequent field/state checks. Preserve those exact checks, update only the list plus its explanatory comment, and run the complete release/debug `test_dis` module after the ongoing source freeze ends. Those later assertions were not reached by the failing subtests, so they remain to be validated. Retain the failed full-suite and diagnostic records separately. No production/disassembler change is indicated.

## Census: useful scope, small fixes before evidence use

The script intentionally inspects a bounded set of module-defined functions, descriptors and nested code constants, plus logging for `logging_silent`. It is not a call-frequency census or a complete application heap census. Keep that limitation prominent and match reports by source digest, filename/module, qualname and first line; report unmatched code rather than merging totals from unequal scopes.

The no-forced-execution-copy claim is supported by source: `_co_feedback` returns None when no executing feedback exists, and `_testinternalcapi.get_ir(adaptive=True)` uses the existing copy only if present. The before/after byte comparison additionally catches inspection changing recorded feedback. Setup may already execute/specialize code, so “after setup” is the correct label, not cold interpreter. Allocation totals exclude the feedback-block header, code words and allocator overhead as already documented.

### Poly inference

The current maximum-header-index reasoning is valid under the inspected lifetime paths:

- `fb_save_mono` appends table `n`, increments `block->npoly`, and assigns `h->poly = n + 1`.
- `fb_begin_ex` clears only the entry body after `_PyFbHeader`; normal respecialization preserves the table index.
- `_PyFb_Clear` frees the tables when `_PyCode_ClearExec` destroys the executing copy at code deallocation. It does not leave a live code object's headers pointing into a reset allocation.

Thus a live complete header census retains the most recently assigned index, and the allocation capacity rounds that `npoly` up to a power of two. This is a source-dependent inference, not data read from the allocator. The hardcoded header offset 12 and user-supplied 88/120 table size are additional assumptions. Do not label the product an independently measured allocation.

Prefer removing `allocated_poly_bytes` from this small diagnostic if it is not needed for the decision. Alternatively name it `inferred_poly_capacity_bytes`, retain observed max-header index separately, tie the declared table size to the reviewed source identity, and state the monotonic-header/whole-exec-lifetime assumption. Do not add runtime inspection APIs or a general allocation framework just for this estimate.

### Required identity corrections

1. **Hash inputs before and after.** Current hashes are collected only after warmup, so they cannot prove which source bytes produced the run. Hash the script, registry, driver and selected benchmark before loading/executing them; capture the scoped source files once the module scope is established, before warmup; recapture the same lists afterward and reject differences. Retain both manifests. For known logging input, include its configured source file before import where practical. No output from a changed-input run should be admitted merely because the final hash is recorded.
2. **Bind the supplied source to the executable and loaded Lib.** A matching `sys._git` prefix does not establish configured source or stdlib origins. Reuse the existing checkpoint helper's configured-source resolution or a small equivalent check against the build Makefile, require it equals `--runtime-source`, and verify scoped stdlib code/module paths (especially logging) lie under that source's `Lib`. Record actual resolved origins and their hashes. The benchmark files intentionally come from canonical docs and should have their own explicit root.
3. **Enforce the documented execution mode.** Assert JIT off, hash randomization off, and the intended no-site mode rather than documenting them only in the invocation comment. Keep source/native/build identity pre/post equality. This is not timing, so adding a CPU quiet-load protocol is unnecessary.

Keep the report's admission/handler snapshots separate from dynamic hit counts. Checking private/shared indices and metadata byte totals is sufficient for this diagnostic's purpose; avoid treating one warmup as proof that every scoped code object ran. The corrected script needs source review before its first retained census, but no runtime change is requested.

This review used source and retained logs only. No runtime edits, builds or native workloads were executed.
