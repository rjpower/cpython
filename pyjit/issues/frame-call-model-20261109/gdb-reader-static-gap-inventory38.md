# GDB pretty-printing: source gaps after38

Read-only inventory while root obtains the verbose census. The full38 summary
reports sixteen pretty-print failures; it does not identify their individual
causes. Core GDB misc/cfunction tests passed. No candidate execution, GDB run,
source edit or test port by A.

## Concrete mismatches

| Interface | Current source | Consequence / smallest repair to review |
| --- | --- | --- |
| Printer registration | `Tools/gdb/libpython.py:pretty_printer_lookup` accepts TYPE_CODE_UNION with tag PyValue. `Include/pytypedefs.h` defines both PyValue and PyValueBorrow as anonymous struct typedefs containing bits. | A real owned or borrowed value may never select the registered printer. Recognize the exact two value typedefs after appropriate qualifier normalization and check their struct/bits shape, rather than accepting arbitrary structs. |
| Conversion to logical values | PyObjectPtr.__init__ and from_pyobject_ptr test only `gdbval.type.name == 'PyValue'`. | Borrowed value parameters do not use inline decoding or tagged-object conversion. Share the same exact value-type predicate with registration. Decode bits without acquiring a reference or calling into the inferior. |
| Breakpoint and extraction | `test_gdb.util.BREAKPOINT_FN` is builtin_id. PrettyPrintTests.get_gdb_repr requires a top frame printed as builtin_id(self=..., v=...). Actual Clinic builtin_id takes self, args, nargs, kwnames; v is a local PyValueBorrow and the implementation's parameter. | The current backtrace regex assumes an obsolete C ABI. The verbose receipt must distinguish absent v/optimized locals from a printer failure. Read the actual value argument at the wrapper's stable args[0] boundary, or select a genuinely available implementation boundary after checking compiled debug visibility. Do not merely broaden a regex to scrape unrelated output. |
| Fault injection | NULL/corrupt-type cases assign v=0 or dereference v->ob_type while calling id(42). | v is a value struct/borrow, and42 is an inline int with no heap header to corrupt. Preserve corruption coverage using an explicit heap object and a derived PyObject pointer. Keep value-NULL and raw PyObject-NULL tests distinct; neither may silently bypass the original safe-fallback assertions. |

The scalar bit decoder already implements the current double offset and int51
layout. The first repair should reuse it, adding owned/borrowed route coverage,
not introduce another value format. Heap list/tuple/dict cells already call
from_pyobject_ptr on their value fields, so the shared type predicate covers
them as well. Confirm the exact type names/qualifiers in the actual GDB17
receipt before choosing how to normalize them.

## Proposed source packet after the native census

1. Port the real GDB reader: one exact PyValue/PyValueBorrow recognition helper
   used by printer registration and conversion; keep pointer printers and
   corruption fallback behavior. A borrow describes identical bits and owns no
   extra reference. Exercise inline negative/large ints, signed zero, float,
   object/deferred pointers, NULL and corrupted heap pointers through both
   value forms. No inferior allocation or helper call is permitted.
2. Port only the incompatible test harness boundary and its fault injection.
   Prefer explicit `print` of the wrapper input value plus a unique result
   marker over depending on optimized backtrace parameter spelling. Preserve
   breakpoint/backtrace coverage as a separate assertion. A pointer expression
   derived from an actual heap value replaces v->field assumptions; inline42
   remains covered by numeric printing, not invalid header mutation.
3. Root runs one scalar, one heap container, a borrowed wrapper input and one
   corrupted pointer before the full pretty-print module. Capture the original
   failure output and both current reader and harness diffs. Do not skip the
   sixteen failures, change GDB availability, or label every failure a fixture
   issue merely because the ABI changed.

The later active-source-callback frame-position consumer remains separate.
PyFramePtr currently derives compact f_lasti from instr_ptr-ir_words and uses
the line table. That is a valid executable-position coordinate; an eventual
source cursor display needs its own association/bounds proof and cannot replace
f_lasti. There is no evidence yet that it explains these sixteen repr failures.
