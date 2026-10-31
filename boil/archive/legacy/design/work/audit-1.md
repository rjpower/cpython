# Audit 1: object references behind pointers to plain structs

Status: done. Branch `boil/audit`. Trigger: `_testbuffer`'s ndarray had
no tp_traverse (fixed in 49580514d90).

## The tool

`Tools/boil/traverse_audit.py` (note 06, 14.4) now:

- follows a member that points to a plain struct, as `member->field`,
  through at most two pointers, including arrays (`items[]->field`),
  typedefs declared apart from the struct, and lists (a node's pointers
  to its own type are not followed again);
- takes pointers to module, thread, interpreter and runtime states
  (struct names ending in "state") as borrowed;
- reads, for the partial-traverse report, the same-file functions and
  function-like macros that traverse, dealloc and clear call, two calls
  deep, and counts `PyBuffer_Release()` as releasing the buffer's `obj`;
- takes a type's base also from a positional `tp_base`, from
  `Type.tp_base = ...` and from the bases argument of
  `PyType_FromSpecWithBases()` and its relatives;
- shares a struct defined in a .c file only with the types of that file.

`test_tools.test_traverse_audit` runs the tool on a fixture tree; its
ndarray-shaped case fails before the change.

`--check` makes it a gate: it exits 1 when a type with references and no
traverse, or a partial traverse, is not in `Tools/boil/traverse_allow.txt`
as `path variable: reason`, or when an entry no longer matches a reported
type (so the list cannot rot). `gate.sh` runs it at level 3 after the
regen check. The allowlist holds the five entries below that are not
fixed: the cross-interpreter view (runtime-managed), the module def
(static data), and the heaptype.c trio (handed to 6c).

A note on the trigger: the old tool did list ndarray, as
`staticbuf.base.obj`, on the tree before 49580514d90. A consumer's
exporter sits in the embedded `staticbuf`, which the old tool followed.
The pointer case (`head->base.obj`) is what this audit adds.

## Output and triage

The tool's output on 49580514d90 plus this branch's tool commits, before
the fixes: 9 types with references and no traverse, 4 probably
inherited, 1 partial traverse.

Categories: (a) real, an owned reference nobody visits; (b) inherited,
the base's traverse covers it after PyType_Ready; (c) runtime-managed,
the reference is visited or rooted by generic machinery; (d) static
data, the holder is static and the data-segment scan reaches it
conservatively.

| Finding | Report | Cat. | Disposition |
|---|---|---|---|
| `ssl.SSLError` (`PyOSErrorObject`) | inherited | b | `PyType_FromSpecWithBases(..., PyExc_OSError)`; OSError's traverse |
| `RecursingInfinitelyError` (`_testcapi/exceptions.c`) | inherited | b | `tp_base = PyExc_Exception` at init |
| `builtin_method` (`PyCMethodObject`) | inherited | b | base `PyCFunction_Type`; `meth_traverse` visits `mm_class` |
| `xxsubtype.spamdict` | inherited | b | `tp_base = &PyDict_Type` at init |
| `moduledef` `m_copy` (`PyModuleDef`) | missing | d | module defs are static; the data-segment scan reads `m_copy` |
| `CrossInterpreterBufferView` `shared->view.obj` | missing | c | the exporting interpreter roots `view.obj` with `PyGC_AddRoot()`; the receiver must not trace another interpreter's object |
| `_testcapi.HeapCTypeWithDict` `dict` | missing | a | hand-off 1: handed to 6c |
| `_testcapi.HeapCTypeWithDict2` `dict` | missing | a | hand-off 1: handed to 6c |
| `_testcapi.HeapCTypeWithNegativeDict` `dict` | missing | a | hand-off 1: handed to 6c |
| `awaitType` `ao_iterator` (`_testcapimodule.c`) | missing | a | fixed |
| `GenericAlias` `item` (`_testcapimodule.c`) | missing | a | fixed |
| `xxmodule.Xxo` `x_attr` | missing | a | fixed; the module is not built (`Modules/Setup`), checked by compiling the file only |
| `_remote_debugging.BinaryReader` `reader->strings[]` | missing | a | fixed |
| `_ssl._SSLSocket` `Socket`, `ctx`, `owner`, `server_hostname` | partial | a | fixed; `PySSL_traverse` visited only the type, `PySSL_clear` (called from dealloc) releases all four |

Found by reading, outside the tool's reach:

| Finding | Cat. | Disposition |
|---|---|---|
| `_remote_debugging.BinaryWriter` `writer->string_hash` keys | a | fixed; a `_Py_hashtable_t` whose `key_destroy` drops a reference |

The tool cannot see references held as `_Py_hashtable_t` keys or values.
A grep of `_Py_hashtable_new_full()` callers with a destroy function that
drops a reference found only the BinaryWriter; `RemoteUnwinder` already
visits its cache, and tracemalloc's tables hold C strings.

After the fixes the tool reports 5 missing (the three heaptype.c types,
`moduledef`, the cross-interpreter view), 4 inherited, 0 partial.

## Fixes

- `PySSL_traverse` visits the socket's four references;
  `test_ssl.SSLObjectTests.test_sslobject_keeps_its_references`.
- `awaitType` and `GenericAlias` get a traverse;
  `test_coroutines.CAPITest.test_tp_await_keeps_iterator`,
  `test_genericclass.CAPITest.test_c_class_keeps_item`.
- `xxmodule.Xxo` gets a traverse.
- `BinaryReader` visits its string table and `BinaryWriter` its string
  hash's keys; both `__init__`s clear the old pointer before allocating
  the new reader or writer, since a collection in between would run the
  traverse on freed memory;
  `test_binary_format...test_string_tables_survive_collection`.

Each test drops the last visible reference, collects, fills freed memory
with junk and then uses the referent.

## Hand-off

1. **`Modules/_testcapi/heaptype.c`, HeapCTypeWithDict and its two
   siblings: handed to 6c.** The dict at `tp_dictoffset` is visited by
   nothing: `subtype_traverse` covers it only for Python subclasses, and
   the types have no traverse. `o = _testcapi.HeapCTypeWithDict();
   o.value = [...]; gc.collect()`, then a read, segfaults on build/debug.
   The coordinator's ruling: the collector is to visit the dict word at
   `tp_dictoffset` generically (6c), which removes the class rather than
   these three instances. Until then they are on the allowlist; once the
   collector visits the word, the tool should learn it too (a type with a
   `tp_dictoffset` and only that reference is then runtime-managed), and
   the three entries go.
2. **The tool and hash tables.** References kept as `_Py_hashtable_t`
   keys or values are invisible to it. Reporting a `_Py_hashtable_t *`
   member whose table is created with a destroy function that releases a
   reference would close that class; not done here.

3. **`_ssl` is not built on this machine** (no OpenSSL headers): the
   `PySSL_traverse` fix and `test_sslobject_keeps_its_references` are
   checked by reading only; test_ssl is skipped in every gate here.

## Gates

On 4fef9745cbc (the fixes and the check), 2026-10-27:

- Gate 1, test_tools, test_ssl, test_coroutines, test_genericclass,
  test_profiling, test_capi: build/default 1,987 tests, 49 skipped;
  build/debug 1,987 tests, 23 skipped; test_ssl skipped as a file on
  both. test_tools again after the check: 91 tests, 1 skipped.
- The three runnable regression tests, with the C fixes reverted, on
  build/default: test_tp_await_keeps_iterator fails (1 != 3),
  test_c_class_keeps_item and test_string_tables_survive_collection
  segfault.
- Gate 2, build/default: 556 of 564 files, 50,851 tests, 3,427 skipped,
  94 files skipped, 8 resource-denied, 0 failures; 1 env changed:
  test_inspect, an unraisable `KeyError: '__import__'` while finalizing
  a coroutine. That test and the code it runs are not touched here.
- `traverse_audit.py --check`: exit 0.
