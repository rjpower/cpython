# M1 cold post-review

2026-11-08. I independently reviewed the nine-file materialized M1 diff in
`pyvalue-rc-universal` on `73c20472a9f`, including regenerated C and opcode
metadata, rather than relying on the draft or its author review. No must-fix
source or correctness finding remains. This is a focused review, not merged
qualification or a performance claim.

The new provider accepts only an exact dict stored as an object value, an
inline-int key, and a combined general-key table on the GIL build. Its hash
uses the same `_PyLong_ValueHash` arm as generic `_PyValue_Hash`, and its first
index matches `do_lookup`. Generic `compare_generic` checks key identity
before invoking equality. Thus an identity hit needs no callback; a miss,
different first key, noncanonical shape or free-threaded build declines to
the original path. The provider changes neither owners nor exceptions on a
decline. A hit duplicates the canonical entry value while the dict is live.

The generated handler is consistent with that contract. It first probes
borrowed B/C inputs, then consumes those inputs only on a hit, publishes the
owned result, and retires key before dict. The retirement test is sequential:
it checks the dict only after closing a harmless key. Possible callbacks use
`retire_published_values`, reload the instruction and slots, and preserve the
result for failure cleanup. A local destination is installed after retiring
inputs; its displaced old value uses the existing observable retirement
path. Declines enter the original generic `value_subscript` and completion.
The source guard and generated validator require the named nonescaping
prefix, exact stack/format roles and no value record, preventing accidental
use on a different handler shape. The new intrinsic has conservative effect
metadata despite its terminal successful edge.

Regeneration removes `BINARY_OP_SUBSCR_DICT` from the currently unused legacy
optimizing expansion table. The register-IR handler remains generated. A
future optimizing frontend must supply its own generic dictionary lowering
before relying on that table; this is a design dependency, not a present
interpreter failure. No register word or call layout changed.

`make regen-cases` and both release/debug builds returned zero. The retained
round-two focus receipts show all 453 tests across `test_generated_cases`,
`test_ir`, `test_dict` and `test_monitoring` passing in each build. The three
new dictionary semantic cases also pass on the retained R5 ELF and M1 ELF.
The initial expected destructor order was wrong in both: frame-locals proxy
state retains the overwritten `Old` owner until frame destruction. The
corrected assertion checks that dictionary destruction observes the old
local before the folded store; its event order matches R5 exactly. The
mutating-collision fixture now arms its callback after dict construction.
The generated tests reject changed guard/shape, duplicate or misplaced
marker, and boxed value-record placement. `git diff --check` is clean.

The retained fixed-GCC disassembly has a call to
`_PyDict_TryGetFirstProbeValue`, tests its result, and branches to the success
edge before the separate decline edge calls `value_subscript`. The provider
is a small out-of-line probe with no calls on its common first-index path.
This proves that the accepted edge skips the generic helper in this binary;
it does not prove elapsed-time improvement. Isolated timing and merged
qualification remain necessary for a speed or batch admission conclusion.
