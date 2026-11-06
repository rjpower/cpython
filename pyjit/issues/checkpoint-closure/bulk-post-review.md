# Bulk ownership rewrite post-review

Reviewed the net diff from `4b2a6926d2a` in
`build/checkpoint-bulk-20261105`: 125 files, including the 48 reviewed table
rows, 447 mechanical call-site rewrites, bounded residue repairs, generated
Clinic output, and its producers. **No must-fix finding in this pass.** This
approval is for that snapshot; it does not assert R27 inventory completeness.

The substitutions inspected preserve result ownership and error tests:
object arguments are wrapped as borrowed `PyValue`, fixed-type object results
remain owned `PyObject *`, scalar extractors retain their old sentinel/error
checks, and value results are explicitly closed or transferred. The changed
`_pickle` extension-registry lookup now closes its owned value after scalar
extraction. The signal module's repeat-exec path can read a user-replaced
inline `default_int_handler`; the candidate correctly uses a fallible
`_PyValue_AsObjectStealAt(..., BOX_SITE_FIELD)` before storing the handler in
its object-pointer array. It must remain an explicit object-storage boundary.

The new Clinic `value_core` directive leaves object wrapper signatures and
`METH_O` registration intact while selecting value extractors for converters
inside a core unit. Its generated `transmogrify.h.h` still checks
`-1 && PyErr_Occurred()` for `expandtabs`, and the unsigned native-byte code
matches the existing `_PyLong_UnsignedInt_Converter` flags, width check and
error string. The directive rejects limited C API mode and conflicts with
`value_emission`. The generated AST edits match `Parser/asdl_c.py` at the
changed conversion sites. Platform offset aliases in `_iomodule.h` and zlib
preserve each `off_t` width branch; the `pid_t` port follows the core,
non-limited `PyLong_AsPid` branch selection.

`git diff --check 4b2a6926d2a` is clean. The author reports isolated clang
debug build with 117 modules and focused Clinic, AST, bytes/bytearray, zlib,
pickle, signal and SQLite Blob tests passing. Integration still needs its
normal merged lint and focused tests. The conditional signal storage box and
the unreviewed manual ownership pairs are visible remaining R27 work; neither
is hidden by this approval.
