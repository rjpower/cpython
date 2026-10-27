#ifndef Py_INTERNAL_STATS_H
#define Py_INTERNAL_STATS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_structs.h"     //

/* What a box of an inline value is for: the classes of boxing sites that the
   statistics count (object_stats.value_boxes).  Other boxes are counted only
   in the totals. */
#define _PY_BOX_CONTAINER_STORE 0   /* stored into a list, tuple, dict, set
                                       or cell */
#define _PY_BOX_ATTRIBUTE_STORE 1   /* stored as an attribute */
#define _PY_BOX_CALL            2   /* an argument of a call into C */
#define _PY_BOX_RETURN          3   /* the eval loop's result to C */
#define _PY_BOX_OTHER           4   /* not counted by class */
#define _PY_BOX_SITES           64


#ifdef Py_STATS

#define STAT_INC(opname, name) _Py_STATS_EXPR(opcode_stats[opname].specialization.name++)
#define STAT_DEC(opname, name) _Py_STATS_EXPR(opcode_stats[opname].specialization.name--)
#define OPCODE_EXE_INC(opname) _Py_STATS_EXPR(opcode_stats[opname].execution_count++)
#define BOX_STAT_INC(opname, kind) \
    do { \
        _Py_STATS_EXPR(opcode_stats[opname].boxes++); \
        VALUE_BOX_STAT_INC(kind); \
    } while (0)
#define VALUE_BOX_STAT_INC(kind) _Py_STATS_EXPR(object_stats.value_boxes[kind]++)
#define CALL_STAT_INC(name) _Py_STATS_EXPR(call_stats.name++)
#define OBJECT_STAT_INC(name) _Py_STATS_EXPR(object_stats.name++)
#define OBJECT_STAT_INC_COND(name, cond) _Py_STATS_COND_EXPR(cond, object_stats.name++)
#define EVAL_CALL_STAT_INC(name) _Py_STATS_EXPR(call_stats.eval_calls[name]++)
#define EVAL_CALL_STAT_INC_IF_FUNCTION(name, callable) _Py_STATS_COND_EXPR(PyFunction_Check(callable), call_stats.eval_calls[name]++)
#define GC_STAT_ADD(gen, name, n) _Py_STATS_EXPR(gc_stats[(gen)].name += (n))
#define RARE_EVENT_STAT_INC(name) _Py_STATS_EXPR(rare_event_stats.name++)
#define OPCODE_DEFERRED_INC(opname) _Py_STATS_COND_EXPR(opcode==opname, opcode_stats[opname].specialization.deferred++)

#define FT_STAT_MUTEX_SLEEP_INC()
#define FT_STAT_QSBR_POLL_INC()
#define FT_STAT_WORLD_STOP_INC()

// Export for '_opcode' shared extension
PyAPI_FUNC(PyObject*) _Py_GetSpecializationStats(void);

#else
#define STAT_INC(opname, name) ((void)0)
#define STAT_DEC(opname, name) ((void)0)
#define OPCODE_EXE_INC(opname) ((void)0)
#define BOX_STAT_INC(opname, kind) ((void)0)
#define VALUE_BOX_STAT_INC(kind) ((void)0)
#define CALL_STAT_INC(name) ((void)0)
#define OBJECT_STAT_INC(name) ((void)0)
#define OBJECT_STAT_INC_COND(name, cond) ((void)0)
#define EVAL_CALL_STAT_INC(name) ((void)0)
#define EVAL_CALL_STAT_INC_IF_FUNCTION(name, callable) ((void)0)
#define GC_STAT_ADD(gen, name, n) ((void)0)
#define RARE_EVENT_STAT_INC(name) ((void)0)
#define OPCODE_DEFERRED_INC(opname) ((void)0)
#define FT_STAT_MUTEX_SLEEP_INC()
#define FT_STAT_QSBR_POLL_INC()
#define FT_STAT_WORLD_STOP_INC()
#endif  // !Py_STATS


#define RARE_EVENT_INTERP_INC(interp, name) \
    do { \
        /* saturating add */ \
        uint8_t val = FT_ATOMIC_LOAD_UINT8_RELAXED(interp->rare_events.name); \
        if (val < UINT8_MAX) { \
            FT_ATOMIC_STORE_UINT8(interp->rare_events.name, val + 1); \
        } \
        RARE_EVENT_STAT_INC(name); \
    } while (0); \

#define RARE_EVENT_INC(name) \
    do { \
        PyInterpreterState *interp = PyInterpreterState_Get(); \
        RARE_EVENT_INTERP_INC(interp, name); \
    } while (0); \

PyStatus _PyStats_InterpInit(PyInterpreterState *);
bool _PyStats_ThreadInit(PyInterpreterState *, _PyThreadStateImpl *);
void _PyStats_ThreadFini(_PyThreadStateImpl *);
void _PyStats_Attach(_PyThreadStateImpl *);
void _PyStats_Detach(_PyThreadStateImpl *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STATS_H */
