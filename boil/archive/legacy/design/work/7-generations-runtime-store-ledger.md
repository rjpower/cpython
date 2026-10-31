# Generations: tuple, cell, function, code and exception stores

Source-only handoff on frozen `5a45f9bb6a73e7351c861faf05624ae4810f6d87`
in `boil-pre-i-union`. No source changes, target runs, builds or probes.
This extends the domain/frame/container ledgers; it does not waive their
young-index, safe-promotion, root-handoff or earliest-reentry prerequisites.
The contract remains a noncollecting, nonallocating, idempotent remember of
the **old logical owner**, after allocating/calling preparation and before
publication, with no intervening safepoint. No such barrier exists yet.

## Owner and publication table

Anchors below refer to the frozen source. NULL stores/deletions need no new
edge; inline values and proven old targets can omit an edge barrier. GC
tracking, immutability, private access and apparent constructor uniqueness
are not age evidence. Current full tracing is not being declared broken.

| Owner / strong fields | Physical stores and required boundary |
| --- | --- |
| Tuple `ob_item[0:Py_SIZE]` (`Objects/tupleobject.c` traversal; `Include/cpython/tupleobject.h`) | `PyTuple_SET_VALUE`:54/58, `SET_ITEM`:67 and `PyTuple_SetItem`:107 have the tuple owner. Cover these public setters with the same public-header bridge decision as list/cell; internal-only declarations cannot simply appear in public inlines. NULL slots are legal during construction. Hash cache, length and tracking are control fields. |
| Tuple raw construction | `tuple_alloc`:34 returns uninitialized element storage; `PyTuple_New`:63–79 zeroes it before return. Pack/pair/array/value constructors (:160–217, :433–501), concat/repeat (:542–622), slicing (:880) fill after their last object allocation in noncollecting copy/canonicalization regions. This is a narrow region proof, not a global fresh-tuple exemption. Subtype allocation/copy (:793–824) must bind allocator return and safe slot initialization before any promotion proof. |
| Tuple incremental / raw-pointer publishers | `Python/marshal.c`:1681–1697 recursively calls `r_object` between stores; `Python/bltinmodule.c` map iterator tuples (:1484/:1530), zip iterator construction (:3166) and `zip_next`:3200–3227 call Python/allocate between slot stores. Recheck tuple owner at each store. `Objects/codeobject.c:133–196` mutates existing name/constant tuples during interning, including `_PyUnicode_InternImmortal(..., &_PyTuple_HEAP_ITEMS(tuple)[i])`; raw address loses owner. Intern to a rooted temporary, then publish through the tuple contract, or supply an explicitly owner-aware interning interface. Do not bless all `_PyTuple_HEAP_ITEMS` writes as initialization. |
| Cell `ob_ref` | `PyCell_SET_VALUE` (`Include/cpython/cellobject.h`:37–44) and `_PyCell_SwapValue` (`pycore_cell.h`:19–26) are distinct physical stores; both need the cell owner contract. `PyCell_Set`/contents setter, bytecode `STORE_DEREF`/`MAKE_CELL` (:2193/:2130), frame-local proxy (`Objects/frameobject.c`:309) route here. Swap-to-NULL and clear are deletions. `PyCell_New` (:10–22) allocates then canonicalizes/stores without another allocating call; caller inputs must survive allocation. Canonical `PyValue` contents, not the surrounding frame, belong to the cell. |
| Function fields (`Objects/funcobject.c:1241` traversal) | All 14 Python fields are strong: globals, builtins, name, qualname, code, defaults, kwdefaults, closure, doc, dict, module, annotations, annotate, typeparams. Code is not exempt for lacking HAVE_GC. Version/vectorcall are control/native pointers; the version cache is explicitly weak/nonowning (:265–286), not a new heap edge. |
| Function mutable setters | Public defaults/kwdefaults/closure (:405–515), Python code/defaults/kwdefaults setters (:640–701/:833–861/:883–914), and name/qualname/doc/module setters (:711–813) publish into function. Place barrier after boxing, audits, function watchers and other preparation, inside the existing final noncollecting commit. Stop-the-world/version invalidation is not a remembered-set operation. Closure tuple replacement remembers the function; subsequent cell mutations remember the cell. |
| Function annotations/typeparams | `func_set_annotations_and_annotate`:522–527 writes two fields; public/setter paths use it, but `func_get_annotation_dict`:531–574 has raw cache stores after calling `__annotate__` or allocating/filling a dict. `function___annotations___get_impl`:970–984 also installs a new dict directly; `__annotate__` None branch at :946 bypasses the pair helper. Cover every publication after preparation. Typeparams setters at :1037/:1051 likewise remember function. Mutating the installed annotations/kwdefaults dict remains that dict's obligation. |
| Function constructors and bytecode continuation | `_PyFunction_FromConstructor`:109–146 and `PyFunction_NewWithQualName`:150–232 initialize fields before CREATE watcher. Their short field-fill regions can be proved separately. **CREATE watcher runs before return**: `func_new_impl`:1157–1173 then replaces name/defaults/closure/kwdefaults, and `SET_FUNCTION_ATTRIBUTE` (`Python/bytecodes.c`:5380–5393) stores through raw offset after `PyFunction_New`. These cannot assume the function remains young. Offset table in `Python/ceval.c`:393–399 names closure/annotations/kwdefaults/defaults/annotate; use an owner-aware commit without creating a second field-effect table. `PyFrameConstructor` is temporary C input storage, not the function heap owner. |
| Code direct strong fields | `code_traverse` (`Objects/codeobject.c`:2417–2438) owns consts, names, localsplusnames/kinds, filename/name/qualname, linetable, exceptiontable and lazy cached values. `co_consts`/names are tuples despite old header comments saying “list.” Pristine IR words, FBINIT, numeric sizes/versions are data. Code objects are non-GC in this build but are traced/aged objects. |
| Code initial construction | `_PyCode_New`:600–639 validates/interns constructor input, then `PyObject_NewVar` and `init_code`:411–487. `init_code` stores filename/name/qualname and passes their field addresses to interning **before** initializing remaining fields; interning reaches dict setdefault and allocation/error paths (`unicodeobject.c`:14564–14710). A safe-promotion proof must cover that partial prefix, or keep the object SHALLOW/young through initialization. Do not infer no collection from `init_code` returning success. CREATE watcher comes only after the complete body. Legacy constructors, marshal and `code.replace` converge on a new code object; replace does not mutate the original's consts pointer. Input-tuple interning still mutates its actual tuple owners. |
| Code lazy strong caches | `init_co_cached`:248–273 installs a zeroed **raw** `_PyCoCached` block (not separately aged). `get_cached_locals`:1644–1665 allocates a tuple then stores through `cached_field`; remember **co**, after allocation, before store. `_PyCode_GetCode`:2154–2180 similarly allocates bytes then writes `cached->_co_code`. Fresh cache storage does not imply fresh code owner. |
| Base exception fields | `PyBaseExceptionObject` owns dict/args/notes/traceback/context/cause (`Objects/exceptions.c`:150–159). Base init :80–91, args setter :345–353 (after `PySequence_Tuple`), C SetArgs :561–568, traceback setter :379–395/C API :502, context setter :420–435/C API :543 and cause C API :523–530 all publish into **exception**. Actual calls from `Python/errors.c` chaining/normalization must use these commits. Suppress-context is a boolean, not an edge. |
| Exception constructor / traceback attachment | Base new :55–81 allocates through `tp_alloc`, initializes base NULL fields, then attaches args; vectorcall :97–124 allocates an args tuple after the exception object exists. Bind safe zero/prefix state and recheck age after that allocation. `PyTraceBack_Here` (`Python/traceback.c`:314–328) creates a new traceback and attaches it to an existing exception: rooting it as current exception does not recursively trace its old fields in a minor. |
| Traceback object | `tb_create_raw` (`Python/traceback.c`:76–96) owns tb_next/tb_frame; initializes after allocation in a noncollecting region. `traceback_tb_next_set_impl`:181–212 checks links then replaces `self->tb_next`; owner is traceback, not exception/frame. Traversal :244–249 owns both fields. Lineno/lasti are control. Frame escape/construction/handoff remains the frame ledger's prerequisite. |
| Generic object member stores | Exception writable subtype members route to `PyMember_SetOne` (`Python/structmember.c`:337–380): `_Py_T_VALUE`, `_EX`, `_Py_T_OBJECT` and `Py_T_OBJECT_EX` write by offset with **obj** available. Object variants box before storing; barrier goes after that boxing. Cover both value and object cases once; do not add per-exception descriptor wrappers. Readonly members are not write paths. |
| Owner's `__dict__` pointer | Function/exception GenericGetDict can allocate then install via `ensure_nonmanaged_dict` (`Objects/dictobject.c`:6105–6113); GenericSetDict (`Objects/object.c`:2091) delegates `_PyObject_SetDict` (`Objects/typeobject.c`:3855–3905). For these non-inline-values owners the actual write is `*dictptr`, and the owner is function/exception, **not the newly created dict**. Remember after allocation and before pointer publication. Dict element updates are already the container ledger; inline-values type/shape branches remain its separate scope. |

## Exception subtype fields: finite built-in inventory

All fields below are strong raw `PyObject *`, even numeric-looking offsets
and errno. Their traversals in exceptions.c corroborate the layout. Repeated
`__init__` calls operate on an existing owner; they are not fresh regions.
After parsing/conversion/callbacks, one remember covers only a contiguous
noncollecting batch; another allocating step starts a new obligation.

| Built-in owner | Fields and direct publication / preparation anchors (`Objects/exceptions.c`) |
| --- | --- |
| StopIteration / SystemExit | `value` :739, `code` :802/:805; tuple argument object getters may box before commit. Writable member tables :712/:834 also use generic member store. |
| BaseExceptionGroup | `msg`, `excs`, `excs_str` :975–977; sequence repr/conversion and instance/subclass checks precede `BaseException_new` (:858–978). Derived/split groups construct objects through this path and attach inherited traceback/context/cause through the base APIs; subclass callbacks forbid a freshness assumption. Readonly group members have no direct setter. |
| ImportError | `msg`, `name`, `path`, `name_from` :1803–1810; empty-tuple allocation/keyword parsing precede final batch; members :1959. |
| OSError | `myerrno`, `strerror`, `filename`, `filename2`, Windows `winerror` :2123–2148; args replacement too. Win conversion/newargs creation and `PyNumber_AsSsize_t` (:2118) can allocate/call before subsequent stores; no one early barrier across the whole constructor. `written` is numeric control, not an edge. Members :2449. |
| NameError / AttributeError | `name` :2587; `name`, `obj` :2671–2672 after keyword preparation; members :2616/:2806. |
| SyntaxError | `msg` :2845 then separate `PySequence_Tuple`/parse before filename/lineno/offset/text/end_lineno/end_offset/metadata :2866–2872. `print_file_and_line` is also a **strong object** member/traversal slot (:3012/:2918), not a C boolean. Members :2997. |
| UnicodeEncode/Decode/TranslateError | `encoding`, `object`, `reason` :3776–3780/:3899–3903/:4003–4006; decode path may call buffer exporter/release and allocate copied bytes first. Public reason setters share `unicode_error_set_reason_impl`:3233–3242, allocating a string before commit. `start/end` are numeric controls; object members :3740 use generic store. |

Notes differ from a direct field setter: BaseException.add_note (:284–309)
does optional attribute lookup, list allocation/attribute assignment, then
list append. `__notes__` therefore also depends on generic attribute/dict
publication and list coverage; do not invent a direct `self->notes` store.
Keep the declared notes slot in traversal/constructor safety evidence.
State restore (`BaseException.__setstate__`) invokes ordinary attribute
setters; arbitrary subclass fields remain a type/instance ledger dependency.

MemoryError's existing bootstrap freelist fetch (:4150–4167) reuses an
initialized object via `_Py_NewReference`, and last-resort static singleton
is shared across raises. They must retain the domain ledger's actual reuse
and immortal/static-root rules. Ordinary exception age cannot be inferred
from “new” API spelling; no new allocation-failure policy is proposed here.

## Saved exception state: root storage versus heap owner

`_PyErr_SetRaisedException` (`Python/errors.c`:27–32) writes tstate
`current_exception`; thread root enumeration (`Python/gc_trace.c`:1470–1478)
visits that and every `exc_info` chain entry on each collection. These are
root publications while actually linked, not exception-owned fields.

`_PyErr_SetHandledException` (`errors.c`:621–624), `PyErr_SetExcInfo`:641,
`PUSH_EXC_INFO`/`POP_EXCEPT` (`bytecodes.c`:3860–3872/:1739–1745) instead
write through an `_PyErr_StackItem *`. The slot may be bottom tstate storage
or **gen/coro `gi_exc_state.exc_value`** (`genobject.c`:89 traversal), so a
uniform “thread state means no barrier” rule is false. The slot/owner
interface is missing: either prove active root-chain membership over each
write and remember the generator before unlink/suspension, or expose its
actual heap owner to an owner-aware store. Do not infer the owner from
FRAME_EXECUTING. Existing send/throw/yield linkage and pre-link intervals
are already enumerated in the frame ledger; no duplicate frame protocol is
introduced here. `previous_item` is a root-chain link into embedded C state,
not a strong PyObject target to pass to a heap-value barrier. Clear/unlink
must preserve the existing slot-lifetime proof.

## Resize, opaque storage and scoped completeness

- `_PyTuple_Resize` (tupleobject.c:992–1060) can collect/move and then calls
  `_Py_NewReferenceNoTotal`; growth zeroes added slots afterward. Preserve
  age/remembering across the *complete* resize/re-reference sequence, not
  just allocator realloc, and prove safe old-prefix/new-slot visibility
  before promotion. Failure/reuse must follow the existing actual-free
  ledger; uniqueness and GC_UNTRACK are not permission to reset age.
- `Objects/structseq.c` is a required tuple-layout adjunct: SetItem :92–102
  writes raw slots, constructor :70–88 zeros **real** fields while Py_SIZE
  shows only visible fields, and constructor copy :427/:430 bypasses tuple
  setters. `structseq_traverse`:119 uses real size. Its future owner/oracle
  must enumerate hidden fields too; do not apply plain tuple visible-size
  proof to struct sequences.
- Code `_co_exec`/typed feedback are NOSCAN storage (`Python/ir.c`:250);
  feedback object/shape pointers are version-guarded borrowed observations
  (`pycore_feedback.h`:115–150), not code-owned strong references. Monitoring
  storage is raw event/line/tool data (`pycore_instruments.h`:99–117).
  `co_extra` is a void-pointer extension contract, not a declared strong
  Python field or a suitable new owner registry. External clients keeping
  Python references there require their own explicit root/port contract;
  this ledger does not certify arbitrary contents as nonowning.
- Bounded layout/physical-store coverage: tuple/cell/function/code objects,
  all built-in exception layouts in exceptions.c, traceback links, the
  named generic member/dict helpers, and named bytecode/error-state stores.
  The source index binds all inspected files. It is not an exhaustive
  cross-repository write census: raw C layout users, struct-sequence call
  sites, subclass allocators/fields, classmethod/staticmethod descriptors in
  funcobject.c, other iterator owners, shapes/types, C roots and ports still
  need independently closed inventories. Generated bytecode stores must
  derive from DSL edits. No minor GC activation is implied.

## Future independent young-oracle omissions

Use the full old-owner field scan independent of remembered bits, before
dead-memory access. Useful one-change copied negatives are: an incremental
tuple fill after collecting iterator/marshal input; raw code-input tuple
interning; old-cell public/swap store; CREATE-watcher promotion followed by
SET_FUNCTION_ATTRIBUTE; callable annotation result cache; old-code lazy
cached locals; old-exception args replacement after iterable conversion;
cause/context/traceback and Unicode reason stores; a writable exception
object member with boxing; and generator saved-exception unlink without
its owner handoff. Include a hidden structseq field in the oracle's layout
controls. These must fail a named missing-edge/initialization invariant,
not crash or merely lose contents. Existing tuple/cell/function/code,
exceptions/traceback, generator and allocation/error tests remain semantic
companions; no new run or barrier implementation is authorized here.
