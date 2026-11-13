# Cold public construction extraction: independent proposal review

GO for the proposed isolated three-file correctness extraction onto golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254`. This approves the scope and
contract, not an implementation or admission. The reviewer read the actual
baseline initializer, publication, Copy, constructor and completion validator;
no runtime source, generator, compiler, inferior or test was changed or run.

The baseline already supports the required first-input contract. Its
SetReturnValue checks raw return_slot==IR_NO_DST before any home indexing;
Copy preserves the raw recipient. Common constructed completion validation
requires ordinary RETURN_GENERATOR's destination to be IR_NO_DST. Therefore
the cold helper can install raw discard before Copy and discard its permitted
first None input using the original hot machinery. NONE, descriptor tags,
setter helpers, different activation defaults and remote/debugger changes are
unnecessary. Subsequent suspension still installs its ordinary resume slot.
Keep _PyGen_YieldFromRef's original numeric access and all hot headers/DSL/
handlers untouched.

The reproduced baseline association failure is a real cold API bug: the
FRAME_OBJECT source has frame_obj NULL, Copy transfers NULL, then the old
helper asserts it equals f. The new owning f association is required; merely
removing the assertion would leave incorrect lifetime ownership. Preserve the
existing two-argument Copy and direct f_locals ownership. Acquiring the f
reference before releasing the stolen constructor input balances both ordinary
completion and escaped-frame takeover.

The proposed commit sequence is coherent: subtype allocation and any enabled
coroutine-origin construction precede the move. On failure, GC-free the still
untracked empty allocation and consume f without invoking generator teardown
on uninitialized fields. Under f's critical section install discard, move the
homes/header, set owner/state and owning frame_obj, then initialize weakrefs,
exception linkage, origin/finalizer, async flags and name owners before
publishing f_frame and GC tracking. Release the stolen f reference after
unlocking. The post-move initialization consists of nonfallible reference
acquisition and stores; no Python callback may intervene. Do not retain old
PyCoro_New/PyAsyncGen_New wrapper initialization: that would leak the first
origin owner and reintroduce fallible postpublication mutation, the blocker
already corrected in R2. Both wrappers should directly return the helper.

The exact extraction boundary is sound: genobject.c's cold helper and two
wrappers; the testcapi frame_new_generator helper; and the three direct
constructor test methods plus their imports. Exclude R2's YieldFromRef port,
fresh-frame clear test, NONE state, pystate/header work and debugger callback
regression. No generated-product, opcode, magic or source-map change follows.
The baseline's common generator head already has the shared subtype fields;
no layout port is required.

The three tests exercise all public subtype constructors, first input,
association, direct namespace ownership across copy/escaped-frame cleanup,
coroutine origin, async-generator hooks/asend/close and wrong first send.
Root should repeat focused release/debug execution on the new actual identity;
R2's passing results cannot be relabeled as baseline-extraction results.
Repeat the bounded allocation/origin failure probe, retaining the distinction
between failed frame allocation, subtype allocation and origin construction.
Verify code/namespace owners on each failure/success and keep original
baseline assertion evidence.

Final actual-source review must confirm exactly the three intended files and
unchanged hot source/generated products against golden51. A new binary may
have different placement/alignment despite unchanged hot source, so source
scope alone is not a machine-code-equivalence or throughput claim. Root owns
integration, qualification and any selected decision; this correction is
supported by the reproduced API failure and makes no performance promise.
