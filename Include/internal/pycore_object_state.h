#ifndef Py_INTERNAL_OBJECT_STATE_H
#define Py_INTERNAL_OBJECT_STATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist_state.h"  // _Py_freelists
#include "pycore_hashtable.h"       // _Py_hashtable_t


/* Reference tracer state */
struct _reftracer_runtime_state {
    PyRefTracer tracer_func;
    void* tracer_data;
};


struct _py_object_runtime_state {
    int _not_used;
};

struct _py_object_state {
    struct _Py_freelists freelists;
    int _not_used;
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OBJECT_STATE_H */
