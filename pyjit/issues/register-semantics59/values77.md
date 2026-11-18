# Value-operation rules and the value-slot table

Record, 2026-11-18. Evidence: `E59/values76-*`, `E59/values77-*`,
`E59/slots77-*`, `E59/ownership-review77`.

## Rules

The rules are contracts for every value-form function, not only the hot ones
(`E59/values76-rules.md`):

1. **Callers own, callees borrow.** Interpreter temporaries own their values.
   Locals are read in place, and `f_locals` writes keep an overwritten value
   alive until the frame dies. A callee therefore needs no reference of its
   own, unless a C caller passes a value borrowed from storage that the
   callee's Python code can rewrite.
2. **Public entries validate; internal entries assert.**
3. **A value's pointer space is tested once,** with inline cases on cold
   paths unless they are the common case.

## Implementation

`pyjit/values76` (`71d5742f906`..`0d18fff7a07`):

- Rule 1 removes callee-owned counts from 92 functions in `dictobject.c`,
  `typeobject.c` and 18 other files.
- Rule 2 adds internal dictionary entries and splits the dictionary
  iterators by kind.
- Rule 3 makes `PyValue_TYPE` test the pointer space first.

`pyjit/values77` (`73e511d646a`..`35b4666037f`) applies the remaining rows:

- typed internal entries for list, tuple and set, and the rest of the
  dictionary entries;
- the rule 3 splits in isinstance, truth, hash, index, member stores and
  JSON keys;
- a native call scope that borrows its callable, receiver and argument.

The full inventory, with every row kept and its reason, is
`E59/values76-inventory.md`.

The counts stay where the rule does not fit:

- a type read from `ob_type` and passed to Python that can assign
  `__class__`;
- a setter that `property.__init__` can replace;
- `_PyObject_LookupSpecialMethod`, which consumes its receiver;
- fields of a mutable object read by a call that runs Python
  (`OSError.__str__`, now retained at the call site).

Three proofs in the inventory were wrong and were caught during
implementation:

- `PyEval_EvalCodeEx()` stores kwdefaults unchecked, so `func_kwdefaults`
  keeps the public entry.
- `object.__getattribute__(o, 1)` reached the generic attribute slot with a
  non-str name and crashed once the second name test was removed. The slot
  wrappers now test the name.
- GCC already folds `PyValue_IsObject(x) && f(PyValue_AsKnownObjectBorrow(x))`
  into one range test, so about 150 such sites were left alone.

Instructions per loop (`values76-measure.sh`, before the frame-chain fix):

| Kernel | values76 change | values77 change |
| --- | ---: | ---: |
| deltablue | −0.2% | −0.07% |
| raytrace | −0.05% | −0.02% |
| json_dumps | −0.5% | −0.4% |
| pickle | +0.05% | −0.5% |
| xml_etree_process | −0.5% | −0.06% |
| deepcopy | −1.2% | −0.1% |

values76 shrinks code by 12.8 KB.

## Ownership review

An adversarial review hunted for use-after-free on the merged tree, on a debug
build with `PYTHONMALLOC=debug -X dev`, and compared each case with upstream.
It ran about 5,400 cases, also under GC stress (`gc.set_threshold(1, 1, 1)`
plus clearing `__del__` hooks):

- containers whose elements' hooks clear them;
- dunders and descriptors that delete themselves or reassign `__class__`;
- class-creation hooks;
- local rebinding through `f_locals` and trace callbacks;
- non-str attribute names;
- evil `str` keys in type, module and instance dictionaries.

It found no difference from upstream except one in our favour: upstream
crashes in `OSError.__str__` when a field's `__repr__` reassigns the others.
It did not reach the module-state dictionaries in `Modules/` or dictionary
watchers. Its harnesses are kept in `E59/ownership-review77/harness`.

## The value-slot table (reverted)

`pyjit/slots77` (`d9054a5b0e4`) implemented `E59/values76-slot-design.md`:

- every value slot takes its owner type;
- `PyType_Ready` fills each empty field with one of 53 generated adapters;
- dispatchers call the value slot without a NULL test.

It was instruction-neutral (DeltaBlue +0.02%). Its size savings came from
moving the boxing code out of the dispatchers: `_PyValue_GetAttr` 883 → 258
bytes, `_PyValue_SetAttr` 1,520 → 419.

The table has to be kept in step with every object-slot write. A design
review (`E59/slots77-notes.md` lists the implementer's 15 deviations) and the
full suite found four ways it falls out of step:

1. `binary_op1` and `ternary_op` read both slots before the first call, as
   upstream does. The adapter re-reads its owner's slot at call time. If the
   first call assigns `w.__class__` and collects the old type, the second call
   reads freed memory: a SIGSEGV in both builds, where upstream raises
   TypeError. Without the free, the call reaches a `__radd__` installed during
   the first call, which upstream does not call.
2. An object slot stored after `PyType_Ready` leaves the value field 0, so a
   release dispatcher calls NULL.
3. A replaced object slot keeps its old native pair even after
   `PyType_Modified`. This was already true before the change.
4. A type that is never readied, such as the `co_branches()` iterator, has 0
   fields, so `next()` calls NULL. `test_code` and `test_monitoring` crash.

`3c8abb64776` reverts the merge. The replacement keeps the object slot as the
definition, and a value field stays an optional native pair, valid only while
it matches. A 0 field calls a cold, out-of-line adapter that takes the object
slot the dispatcher already read. That removes the owner parameter and the
adapter table, and cases 1, 2 and 4 cannot arise. `PyType_Modified` clears
fields that no longer match (case 3). The work is on `pyjit/slots78`.

The review also found two upstream differences that predate the change:

- `do_richcompare` caches both operand types across its three steps, where
  upstream re-reads them.
- `list(it)` re-reads `tp_iternext` on every item, where upstream caches
  it.

Neither crashed.
