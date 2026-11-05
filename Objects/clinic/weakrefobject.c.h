/*[clinic input]
preserve
[clinic start generated code]*/

#ifndef Py_VALUE_CORE
#  error "value-emission Clinic output requires Py_VALUE_CORE"
#endif

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()

static Py_hash_t
weakref_hash_impl(PyWeakReference *self);

static Py_hash_t
weakref_hash(PyObject *self)
{
    Py_hash_t return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = weakref_hash_impl((PyWeakReference *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=c11de7d1bf969284 input=a9049054013a1b77]*/
