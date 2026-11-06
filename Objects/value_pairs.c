/* The pair table of the value slots (pyjit/design/proposals/
   value-api-v2a0.md, section 4.3): object functions that many types share,
   with their value sides.

   This unit holds the table and nothing else: no function body
   (Tools/build/check_value_api.py, rule pair-table). It defines
   Py_VALUE_PAIR_TABLE, so that Py_VALUE_COMPAT (Include/pyport.h) marks
   nothing here: it is the one unit that takes the address of a function
   that the waves mark, and it does so through the real, type-checked
   declarations (value-api-m1b-m2.md, A8). */

#define Py_VALUE_PAIR_TABLE 1

#include "Python.h"
#include "pycore_typeobject.h"    // _Py_slot_tp_getattro()
#include "pycore_value_ops.h"     // _PyObject_GenericGetAttrValue()
#include "pycore_value_call.h"    // _PyValue_InstanceVectorcall()
#include "pycore_value_slots.h"   // _PyValueSlotPair

const _PyValueSlotPair _PyValueSlot_Pairs[] = {
    {_Py_VALUE_SLOT_INDEX_init_, (void *)_Py_object_init,
     _Py_VALUE_SLOT_INIT(init_, _Py_object_init_value)},
    {_Py_VALUE_SLOT_INDEX_init_, (void *)_Py_slot_tp_init,
     _Py_VALUE_SLOT_INIT(init_, _Py_slot_tp_init_value)},
    {_Py_VALUE_SLOT_INDEX_new_, (void *)_Py_slot_tp_new,
     _Py_VALUE_SLOT_INIT(new_, _Py_slot_tp_new_value)},
    {_Py_VALUE_SLOT_INDEX_hash, (void *)PyObject_HashNotImplemented,
     _Py_VALUE_SLOT_INIT(hash, _PyValue_HashNotImplemented)},
    {_Py_VALUE_SLOT_INDEX_hash, (void *)PyObject_GenericHash,
     _Py_VALUE_SLOT_INIT(hash, _PyValue_GenericHash)},
    {_Py_VALUE_SLOT_INDEX_getattro, (void *)PyObject_GenericGetAttr,
     _Py_VALUE_SLOT_INIT(getattro, _PyObject_GenericGetAttrValue)},
    {_Py_VALUE_SLOT_INDEX_setattro, (void *)PyObject_GenericSetAttr,
     _Py_VALUE_SLOT_INIT(setattro, _PyObject_GenericSetAttrValue)},
    /* The getattr hook of a Python class (Objects/typeobject.c) installs
       this pair in place of itself (A7). */
    {_Py_VALUE_SLOT_INDEX_getattro, (void *)_Py_slot_tp_getattro,
     _Py_VALUE_SLOT_INIT(getattro, _Py_slot_tp_getattro_value)},
    {_Py_VALUE_SLOT_INDEX_iternext, (void *)_PyObject_NextNotImplemented,
     _Py_VALUE_SLOT_INIT(iternext, _PyValue_NextNotImplemented)},
    {_Py_VALUE_SLOT_INDEX_call, (void *)PyVectorcall_Call,
     _Py_VALUE_SLOT_INIT(call, _PyValue_InstanceVectorcall)},
};

const size_t _PyValueSlot_PairCount = Py_ARRAY_LENGTH(_PyValueSlot_Pairs);
