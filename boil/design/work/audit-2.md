# Audit 2: members that neither the traverse nor a release names

Status: design, for review. Branch `boil/audit2` from 72bed4aa822.
Trigger: 6d rung 4's gate 3 found the CJK stream writer's `pending`
freed under the live writer (fixed on boil/6d at 46bd95ee72c). Report 3
of `Tools/boil/traverse_audit.py` (72bed4aa822) lists the class: a member
with references that the traverse does not name and no release names
either. Upstream leaked such a member or borrowed it; under the tracing
collector a strong one is freed under the object.

## Method

1. Fix the tool's parsing so that report 3 lists only members the code
   really leaves unnamed (B below). Each fix gets a fixture in
   `Lib/test/test_tools/test_traverse_audit.py`, and the whole tree's
   reports 1 to 3 are diffed before and after each fix: an entry a fix
   removes is read in the source to confirm that the member is named, so
   that no fix hides a real omission.
2. Read every remaining entry and classify it: (t) tool defect; (b)
   borrowed or static, with the path that keeps the referent alive; (r)
   real, a strong reference no traverse visits.
3. For each (r): a test that holds the referent only through the member,
   collects, and uses it; it must fail (or trap under
   `PYTHON_GCVERIFY=kill` on build/debug) before the fix and pass after.
   The fix is `Py_VISIT` in the traverse, plus the release in dealloc or
   clear where upstream had none.
4. Each (b) goes in `Tools/boil/traverse_allow.txt` as `path variable:
   reason` naming the path; report 3 joins `--check` (D). An entry whose
   path does not hold is (r).

## First reading of the 16 entries

| # | Type (file) | Members | Reading |
|---|---|---|---|
| 1 | `_deque_iterator` (_collectionsmodule.c) | `b->data` | b: `b` points to a block of `it->deque`, which the traverse visits; `deque_traverse` visits every item; the iterator reads `b` only after checking `deque->state` |
| 2 | `_deque_reverse_iterator` | `b->data` | b: as 1 |
| 3 | `defaultdict` | `ma_keys` | t: `defdict_traverse` ends in `PyDict_Type.tp_traverse(op, ...)`; the tool does not follow a call through another type's `tp_traverse` slot |
| 4 | `_ctypes.COMError` | `args`, ..., `traceback` | t: delegates to `PyExc_BaseException->tp_traverse`, as 3 (Windows only) |
| 5 | `ElementTree.Element` | `extra->_children` | b: `_children` is the inline storage, and while `children == _children` (until the first spill to a PyMem array) the traverse's `children[0..length)` is `_children[0..length)`; after a spill `_children` is unused. An alias the tool cannot see |
| 6 | `functools.partial` | `placeholder` | b: set from `state->placeholder` without a reference (the singleton); the partial visits its type, the type its `ht_module`, the module state visits `placeholder` |
| 7 | `functools._lru_list_elem` | `next`, `prev` | b: upstream's "borrowed links"; every link in the list is visited by `lru_cache_tp_traverse`'s walk, and a link out of the list is held by the cache dict |
| 8 | `functools._lru_cache_wrapper` | `prev` (of `root`) | b: as 7; the walk goes by `next` |
| 9 | `_lsprof.Profiler` | `currentProfilerContext->ctxEntry->userObj`, `freelistProfilerContext->...` | t, then b: `ctxEntry` points to an entry in `profilerEntries`, whose `userObj` the traverse visits through `visit_entry`, a callback passed to `RotatingTree_Enum` that the tool does not follow (it follows calls, not function references); free-list contexts' `ctxEntry` is stale and never read before `initContext` |
| 10 | `sqlite3.Connection` | `Warning`, ..., `NotSupportedError` | b: "Borrowed refs" from the module state, reached by type, `ht_module`, module traverse (`_sqlite3` is not built here) |
| 11 | `_sre.SRE_Scanner` | `state.string`, `state.buffer.obj`, `state.fail_after_exc` | t and r: `state_fini` releases the first two, but the tool's function parser files `LOCAL(void)\nstate_fini(...) {` under `LOCAL`, so the release is not seen; once parsed, the entry moves to report 2 (a partial traverse). `state.string` is a strong reference (`Py_NewRef` in `state_init`) visited by nothing: real. `fail_after_exc` (debug only) is borrowed from the pattern, which the scanner visits |
| 12 | `MultibyteStreamWriter` | `pending` | r, fixed on boil/6d (46bd95ee72c); the same hunk and test are applied here (coordinator, 10:40: a real defect is fixed, never listed; identical hunks merge cleanly whichever branch lands first) |
| 13 | `instancemethod` | `func` | t: named through `PyInstanceMethod_GET_FUNCTION()`, a static inline function in Include/cpython/classobject.h that the tool does not read |
| 14 | `enumerate` | `one` | b: `_PyLong_GetOne()`, a static small int in `_PyRuntime` |
| 15 | `typing.TypeVar` | `raints` | t: `PyObject *constraints;`: the declarator pattern strips `const` from inside the name |
| 16 | `symtable entry` | `ste_parent`, `ste_table->st_*` | b: `ste_table` points to the compiler's `struct symtable` (PyMem, held for the build from the C stack, a 10a rung 3 concern), and dangles after `_PySymtable_Free` while the entry lives on (the `_symtable` module returns entries), so the entry must not visit through it; `ste_parent` is set only for inlined comprehensions, to `st_cur`, which `st_blocks` holds for the whole build, and read only during analysis |

Likely real after the tool fixes: 11 (SRE scanner), plus whatever the
parser fix moves from silence into report 2 elsewhere in the tree (every
function defined after a `LOCAL(...)`, `Py_LOCAL_INLINE(...)` or similar
line was invisible to the tool, in reports 2 and 3 alike).

## The tool fixes (B)

1. **`const` inside a name.** `DECLARATOR_RE` takes `const` as a
   qualifier only as a whole word.
2. **Functions.** A function is recognised by its name at the start of a
   line and its balanced parameter list followed by `{`, not by
   `[^;{]*`, so a return-type macro on the line before
   (`LOCAL(void)`, `Py_LOCAL_INLINE(int)`) is no longer taken as the
   function; a definition with its return type on the same line
   (`static inline PyObject *f(...) {`, the header style) is recognised.
3. **Helpers in headers.** A function or function-like macro the traverse,
   dealloc or clear calls that the file does not define is looked up in
   the headers of the file's directory, then in Include/ (accessors such
   as `PyInstanceMethod_GET_FUNCTION`, helpers in `sre_lib.h`). Not other
   .c files: a generic helper there names generic members, and crediting
   them would hide omissions.
4. **Function references.** A function of the same file named in the
   body without a call (a callback handed to an enumerator, such as
   `visit_entry`) is read like a callee.
5. **Delegation to another type's traverse.** `X.tp_traverse(...)` or
   `((PyTypeObject *)X)->tp_traverse(...)` where X is a type the tool
   parsed (`PyExc_Name` resolving to `_PyExc_Name`) credits the members
   of X's struct, and only when that struct is the type's own struct or
   one of its bases; otherwise nothing is credited.

The call depth stays 2: with the parser fixed, no entry needs more.

## Joining --check (D)

`--check` reports a type that report 3 lists and the allow list does not
name, as it does for reports 1 and 2; the allow-list format and the
stale-entry rule are unchanged. The key is per type, so an entry alone
would hide a member added to the type later; `--check` therefore also
fails when an entry's reason does not name (as a word, by its last
component) every member the type is reported for. The five audit-1
entries already do. For a type I fix, I visit even the borrowed members
(the scanner's `fail_after_exc`) rather than allow-list the type.

## Tests and gates

- Tool: one fixture per fix (B1 to B5), each failing before its fix, and
  a `--check` case for report 3.
- SRE scanner: `test_re`, a scanner over a string built at run time and
  held only by the scanner, collected, then matched, with `m.string` and
  the groups compared; it must fail or trap before the fix (default
  build, and debug under kill).
- Gate 1 on both builds: test_tools, test_re, test_functools,
  test_collections, test_xml_etree, test_ctypes, test_sqlite3 (skipped,
  not built), test_cprofile, test_typing, test_symtable, test_enumerate,
  test_class, and the files of any other type fixed; the fixed types'
  tests under `PYTHON_GCVERIFY=kill` on build/debug;
  `traverse_audit.py --check` exits 0.

## File ownership

Owned: `Tools/boil/traverse_audit.py`, `Tools/boil/traverse_allow.txt`,
`Lib/test/test_tools/test_traverse_audit.py`,
`boil/design/work/audit-2.md`. For the fixes only, the traverse, dealloc
and clear functions of the types fixed: `Modules/_sre/sre.c`
(`scanner_traverse`), `Modules/cjkcodecs/multibytecodec.c`
(`mbstreamwriter_traverse` and `_dealloc`, 6d's hunk verbatim), and any
other found in C, with its test file (`Lib/test/test_re.py`,
`Lib/test/test_multibytecodec.py`, 6d's test verbatim). No getset functions (6d rung 4)
and no precise-marking directives (10a) are touched.

## Hand-off (first draft)

1. **Buffer consumers release in the sweep: hand-off to the collector
   owner (coordinator to assign).** `PickleBuffer`, `_pickle.Unpickler`,
   the SRE scanner, `_struct.unpack_iterator` and `_testbuffer.ndarray`
   keep a `Py_buffer` and release it in their dealloc; for a Python
   exporter the release either runs `__release_buffer__` in the sweep
   or, when the wrapper is swept first, skips it. The managed buffer's
   pre-sweep destroy hook is the model. Probe: `/tmp`-free form in the
   as-built record.
2. **Symtable's PyMem roots: 10a rung 3.** `struct symtable`'s
   references are held from the C stack through the transitive PyMem
   scan; the entry must not visit them (they dangle after
   `_PySymtable_Free`).

## Review

Round 1: codex, 10:45 (`reviews/audit-2-review.md`), verdict revise.

1. **The scanner's buffer owner.** Accepted. `scanner_traverse` visits
   `state.string` and `state.buffer.obj` (and `fail_after_exc`). The
   test adds a bytes pattern over a Python exporter (`__buffer__`), whose
   `PyObject_GetBuffer()` owner is a `_PyBufferWrapper` that only the
   scanner's view holds, and asserts with `gc.get_referents(scanner)` as
   well as by a match after a collection.
2. **The scanner's release runs Python in the sweep.** Real, and a class
   that predates this audit: every consumer that keeps a `Py_buffer` in
   the object (`PickleBuffer`, `_pickle.Unpickler`, the SRE scanner,
   `_struct.unpack_iterator`, `_testbuffer.ndarray`) releases it in its
   dealloc during the sweep. Through a `_PyBufferWrapper` the release
   calls the exporter's `__release_buffer__` unless the wrapper was swept
   first, in which case `PyBuffer_Release` skips it (note 06, 15.3 item 7
   and the 5b known failures). A probe on 72bed4aa822 (a PickleBuffer and
   a struct iterator over a Python exporter, dropped and collected) shows
   `__release_buffer__` called 0 times: skipped, not run in the sweep.
   Only the managed buffer and memoryview have the pre-sweep destroy
   hook (`live_destroy_hook_of`, gc_trace.c). Extending it to every
   buffer consumer is a collector change outside this audit's files;
   handed off (hand-off 1), with the probe. Visiting the buffer's owner
   does not make this worse: before the fix the wrapper is freed under
   the live scanner, and the scanner's release reads freed memory.
3. **Delegation credits layout, not traversal.** Accepted. The credit is
   now what the delegate's own traverse names, restricted to the paths of
   the delegate's struct (exact paths, not last components), and given
   only when the call's receiver is the traverse's own object (its first
   parameter or a local initialised from it). Fixtures: a partial base
   traverse, a repeated member name in a derived pointer path, and
   delegation on another receiver.
4. **The allowlist hides new members.** Accepted. An entry is now `path
   variable member...: reason`: the exact member paths it allows, as the
   report prints them. `--check` fails on a reported path the entry does
   not list and on a listed path no longer reported; audit-1's five
   entries are rewritten in this form. (This replaces the interim rule
   that the reason name each member.)

Non-blocking, addressed: `PyValue` members (a cell's `ob_ref`, a list's
`ob_item`) count as references (no new entries on the tree, two more
"probably inherited" list subclasses); a traverse the tool cannot find
(today `_testcapi`'s ManagedDict, whose slot is cast through `(void *)`)
is a `--check` failure rather than silence, and the cast is parsed; row
5's invariant is `children == _children`, not a length; the scanner
visits `fail_after_exc`, which `_fail_after()` can replace on the
pattern. Carried: header lookup by directory then Include/ takes the
last definition of a duplicated name (no conflict on the tree);
symtable's PyMem roots are 10a rung 3's (hand-off 2).
