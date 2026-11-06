# Decimal value-entry census, W5 candidate

This is a source census of the current unmerged W5 candidate, not a claim that the shared runtime contains these layers. The candidate includes queued rich comparison and Context getset, attribute, initializer and selective Clinic changes. Line numbers may change during integration. Compiler checks only reject currently marked APIs; they cannot prove that unmarked calls or object-form entries avoid boxing.

A clang-21 `Py_VALUE_CORE` syntax trial using the bulk tree's currently marked headers now passes with zero diagnostics; raw log: `build/checkpoint-bulk-build-20261105/decimal-current-census4.log`. This does not cover unmarked APIs. The earlier four diagnostics are addressed by queued IEEEContext and formatting patches. Normal release build and `EXTRA_FUNCTIONALITY` syntax compile pass; CONFIG_32 requires a real 32-bit mpdecimal sysroot.

The generated Clinic header currently has 12 object-form method definitions without `METH_VALUE`:

- Context: `clear_traps`, `clear_flags`, `copy`, `__copy__`, and `__reduce__`.
- Module: `getcontext`.
- Decimal: `canonical`, `conjugate`, `copy_abs`, `copy_negate`, `__copy__` and `__reduce__`.

The queued integer, formatting and miscellaneous method packets removed fourteen previously reported definitions from this list. Context `__reduce__` also creates primitive constructor arguments through `Py_BuildValue`; it needs a value-tuple port. Other no-argument entries return None or concrete heap objects but still need boundary review.

The C file additionally declares object-form `SignalDict.copy`, `ContextManager.__enter__`, and module `__getattr__`. Some return only concrete heap objects or have no polymorphic inputs, so each must be audited rather than mechanically changed. Conversely integer-returning `Etiny`, `Etop`, `adjusted`, `radix` and `__sizeof__`, and scalar `IEEEContext` need value entries to avoid inline-result or argument projection. Conditional `_unsafe_*` methods require their configured-platform behavior to be retained.

Decimal's arithmetic, conversion and rich-comparison number slots have paired value entries in this candidate. Remaining object-form slots include Decimal repr/str/hash and generic attribute access; Context repr and its constructor object boundary; SignalDict rich comparison, iteration, mapping operations and attribute access; and ContextManager lifetime and attribute paths. Audit them according to actual operand/result types and identify true object boundaries before porting. The three EXTRA_FUNCTIONALITY Context `_allcr`, `_traps`, `_flags` descriptors, the six ordinary descriptors and custom attribute value slots are queued in W5 and not yet qualified. The EXTRA descriptor packet has a separate module build and focused runtime probes.

A source scan also identifies object-form call, dictionary and numeric operations in Context allocation/template setup, the non-contextvar TLS branch, formatting fallback and module/type initialization. The raw matches are listed below for audit; they include concrete-object uses and do not alone establish a boxing defect. Check each reachable call and generated wrapper against ownership and attribution rules, including unmarked APIs and branch-specific builds. Runtime raw counters and startup, kernel and suite gates remain necessary.

```text
940:    return PyLong_FromSsize_t(mpd_get##mem(CTX(self)));     \
947:    return PyLong_FromUnsignedLong(mpd_get##mem(CTX(self)));    \
962:    return PyLong_FromLong(mpd_getcr(CTX(self)));
979:    return PyLong_FromSsize_t(mpd_etiny(CTX(self)));
995:    return PyLong_FromSsize_t(mpd_etop(CTX(self)));
1332:    x = PyLong_AsSsize_t(value);
1522:    self->traps = PyObject_CallObject((PyObject *)state->PyDecSignalDict_Type, NULL);
1528:    self->flags = PyObject_CallObject((PyObject *)state->PyDecSignalDict_Type, NULL);
1731:    context = PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL);
1751:        PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL);
1975:    int found = PyDict_GetItemRef(dict, modstate->tls_context_key,
1997:        if (PyDict_SetItem(dict, modstate->tls_context_key, tl_context) < 0) {
2078:    if (PyDict_SetItem(dict, state->tls_context_key, v) < 0) {
3998:    int found = PyDict_GetItemRef(dict, keyobj, &value);
4038:    pydec = PyObject_CallOneArg(state->PyDecimal, u);
4044:    result = PyObject_CallMethod(pydec, "__format__", "(OO)", fmt, context);
4696:        coeff = _PyTuple_PackValues(1, (PyValueBorrow[]){
5668:    return PyLong_FromSsize_t(retval);
6657:    return PyLong_FromSize_t(res);
8865:    ASSIGN_PTR(Number, PyObject_GetAttrString(numbers, "Number"));
8867:    ASSIGN_PTR(obj, PyObject_CallMethod(Number, "register", "(O)",
8871:    ASSIGN_PTR(state->Rational, PyObject_GetAttrString(numbers, "Rational"));
8878:    ASSIGN_PTR(obj, PyObject_CallMethod(collections, "namedtuple", "(ss)",
8893:    ASSIGN_PTR(MutableMapping, PyObject_GetAttrString(collections_abc,
8897:                   (PyTypeObject *)PyObject_CallFunction(
8934:            base = _PyTuple_PackValues(2, (PyValueBorrow[]){
8939:            base = _PyTuple_PackValues(2, (PyValueBorrow[]){
8944:            base = _PyTuple_PackValues(2, (PyValueBorrow[]){
8949:            base = _PyTuple_PackValues(3, (PyValueBorrow[]){
8955:            base = _PyTuple_PackValues(1, (PyValueBorrow[]){
8988:            base = _PyTuple_PackValues(2, (PyValueBorrow[]){
8993:            base = _PyTuple_PackValues(1, (PyValueBorrow[]){
9009:               PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL));
9025:               PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL));
9032:               PyObject_CallObject((PyObject *)state->PyDecContext_Type, NULL));
9041:                               PyLong_FromSsize_t(ssize_cm->val)));
```
