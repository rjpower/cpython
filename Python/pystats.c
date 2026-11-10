#include "Python.h"

#include "pycore_dict.h"          // PyDict_SetValueString()
#include "pycore_ir.h"            // generated handler metadata
#include "pycore_value.h"         // PyValue_FromObjectBorrow()
#include "pycore_opcode_metadata.h" // _PyOpcode_Caches
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pylifecycle.h"     // _PyOS_URandomNonblock()
#include "pycore_tstate.h"
#include "pycore_initconfig.h"      // _PyStatus_OK()
#include "pycore_pystate.h"         // _PyThreadState_GET()
#include "pycore_pystats.h"          // _PyStats_ChildCreated()
#include "pycore_runtime.h"         // NUM_GENERATIONS
#include "osdefs.h"                 // MAXPATHLEN

#include <stdlib.h> // rand()
#ifndef MS_WINDOWS
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#ifdef Py_STATS

static uint64_t stats_resets;
static void run_marker_create(PyInterpreterState *interp);
static void run_marker_finish(PyInterpreterState *interp, const char *outcome);
static void box_owners_clear(PyInterpreterState *interp);

PyStats *
_PyStats_GetLocal(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate) {
        return tstate->pystats;
    }
    return NULL;
}

#ifdef Py_GIL_DISABLED
#define STATS_LOCK(interp) PyMutex_Lock(&interp->pystats_mutex)
#define STATS_UNLOCK(interp) PyMutex_Unlock(&interp->pystats_mutex)
#else
#define STATS_LOCK(interp)
#define STATS_UNLOCK(interp)
#endif


#define ADD_STAT_TO_DICT(res, field) \
    do { \
        PyObject *val = PyLong_FromUnsignedLongLong(stats->field); \
        if (val == NULL) { \
            Py_DECREF(res); \
            return NULL; \
        } \
        if (PyDict_SetValueString(res, #field, PyValue_FromObjectBorrow(val)) == -1) { \
            Py_DECREF(res); \
            Py_DECREF(val); \
            return NULL; \
        } \
        Py_DECREF(val); \
    } while(0);

static PyObject*
stats_to_dict(SpecializationStats *stats)
{
    PyObject *res = PyDict_New();
    if (res == NULL) {
        return NULL;
    }
    ADD_STAT_TO_DICT(res, success);
    ADD_STAT_TO_DICT(res, failure);
    ADD_STAT_TO_DICT(res, hit);
    ADD_STAT_TO_DICT(res, deferred);
    ADD_STAT_TO_DICT(res, miss);
    ADD_STAT_TO_DICT(res, deopt);
    PyObject *failure_kinds = PyTuple_New(SPECIALIZATION_FAILURE_KINDS);
    if (failure_kinds == NULL) {
        Py_DECREF(res);
        return NULL;
    }
    for (int i = 0; i < SPECIALIZATION_FAILURE_KINDS; i++) {
        PyObject *stat = PyLong_FromUnsignedLongLong(stats->failure_kinds[i]);
        if (stat == NULL) {
            Py_DECREF(res);
            Py_DECREF(failure_kinds);
            return NULL;
        }
        PyTuple_SET_VALUE(failure_kinds, i, PyValue_FromObjectSteal(stat));
    }
    if (PyDict_SetValueString(res, "failure_kinds",
                              PyValue_FromObjectBorrow(failure_kinds))) {
        Py_DECREF(res);
        Py_DECREF(failure_kinds);
        return NULL;
    }
    Py_DECREF(failure_kinds);
    return res;
}
#undef ADD_STAT_TO_DICT

static int
add_stat_dict(
    PyStats *src,
    PyObject *res,
    int opcode,
    const char *name) {

    SpecializationStats *stats = &src->opcode_stats[opcode].specialization;
    PyObject *d = stats_to_dict(stats);
    if (d == NULL) {
        return -1;
    }
    int err = PyDict_SetValueString(res, name, PyValue_FromObjectBorrow(d));
    Py_DECREF(d);
    return err;
}

PyObject*
_Py_GetSpecializationStats(void) {
    PyThreadState *tstate = _PyThreadState_GET();
    PyStats *src = FT_ATOMIC_LOAD_PTR_RELAXED(tstate->interp->pystats_struct);
    if (src == NULL) {
        Py_RETURN_NONE;
    }
    PyObject *stats = PyDict_New();
    if (stats == NULL) {
        return NULL;
    }
    int err = 0;
    err += add_stat_dict(src, stats, CONTAINS_OP, "contains_op");
    err += add_stat_dict(src, stats, LOAD_SUPER_ATTR, "load_super_attr");
    err += add_stat_dict(src, stats, LOAD_ATTR, "load_attr");
    err += add_stat_dict(src, stats, LOAD_GLOBAL, "load_global");
    err += add_stat_dict(src, stats, STORE_SUBSCR, "store_subscr");
    err += add_stat_dict(src, stats, STORE_ATTR, "store_attr");
    err += add_stat_dict(src, stats, POP_JUMP_IF_FALSE, "pop_jump_if_false");
    err += add_stat_dict(src, stats, POP_JUMP_IF_TRUE, "pop_jump_if_true");
    err += add_stat_dict(src, stats, GET_ITER, "get_iter");
    err += add_stat_dict(src, stats, CALL_FUNCTION_EX, "call_function_ex");
    err += add_stat_dict(src, stats, CALL, "call");
    err += add_stat_dict(src, stats, CALL_KW, "call_kw");
    err += add_stat_dict(src, stats, BINARY_OP, "binary_op");
    err += add_stat_dict(src, stats, COMPARE_OP, "compare_op");
    err += add_stat_dict(src, stats, UNPACK_SEQUENCE, "unpack_sequence");
    err += add_stat_dict(src, stats, FOR_ITER, "for_iter");
    err += add_stat_dict(src, stats, TO_BOOL, "to_bool");
    err += add_stat_dict(src, stats, SEND, "send");
    if (err < 0) {
        Py_DECREF(stats);
        return NULL;
    }
    return stats;
}


#define PRINT_STAT(i, field) \
    if (stats[i].field) { \
        fprintf(out, "    opcode[%s]." #field " : %" PRIu64 "\n", handler_name(i), stats[i].field); \
    }

static const char *
handler_name(int i)
{
    return i < IR_FIRST_ONLY_HANDLER
        ? _PyOpcode_OpName[i]
        : _PyIR_OnlyNames[i - IR_FIRST_ONLY_HANDLER];
}

static void
print_spec_stats(FILE *out, OpcodeStats *stats)
{
    int entries = 0;
    for (int i = 0; i < IR_NUM_HANDLERS; i++) {
        entries += (_PyIR_HandlerFlags[i] & _PyIR_H_METADATA) != 0;
    }
    fprintf(out, "Handler entry coverage version : 2\n");
    fprintf(out, "Handler entry dispatch mode : 1\n");
    fprintf(out, "Handler domain count : %d\n", IR_NUM_HANDLERS);
    fprintf(out, "Handler metadata entries : %d\n", entries);
    for (int i = 0; i < IR_NUM_HANDLERS; i++) {
        if (!(_PyIR_HandlerFlags[i] & _PyIR_H_METADATA)) {
            continue;
        }
        const char *name = handler_name(i);
        assert(name != NULL);
        fprintf(out, "opcode[%s].handler_id : %d\n", name, i);
        fprintf(out, "opcode[%s].base_id : %d\n", name, _PyIR_HandlerBase[i]);
        fprintf(out, "opcode[%s].handler_flags : %d\n", name, _PyIR_HandlerFlags[i]);
        if (_PyIR_HandlerFlags[i] & _PyIR_H_ADAPTIVE) {
            fprintf(out, "opcode[%s].specializable : 1\n", name);
        }
        PRINT_STAT(i, specialization.success);
        PRINT_STAT(i, specialization.failure);
        PRINT_STAT(i, specialization.hit);
        PRINT_STAT(i, specialization.deferred);
        PRINT_STAT(i, specialization.miss);
        PRINT_STAT(i, specialization.deopt);
        PRINT_STAT(i, execution_count);
        PRINT_STAT(i, boxes);
        for (int j = 0; j < SPECIALIZATION_FAILURE_KINDS; j++) {
            uint64_t val = stats[i].specialization.failure_kinds[j];
            if (val) {
                fprintf(out, "    opcode[%s].specialization.failure_kinds[%d] : %"
                    PRIu64 "\n", name, j, val);
            }
        }
        for (int j = 0; j < IR_NUM_HANDLERS; j++) {
            if (stats[i].pair_count[j]) {
                assert(_PyIR_HandlerFlags[j] & _PyIR_H_DISPATCH);
                fprintf(out, "opcode[%s].pair_count[%s] : %" PRIu64 "\n",
                        name, handler_name(j), stats[i].pair_count[j]);
            }
        }
    }
}
#undef PRINT_STAT


static void
print_call_stats(FILE *out, CallStats *stats)
{
    fprintf(out, "Calls to PyEval_EvalDefault: %" PRIu64 "\n", stats->pyeval_calls);
    fprintf(out, "Calls to Python functions inlined: %" PRIu64 "\n", stats->inlined_py_calls);
    fprintf(out, "Keyword binding plan hits: %" PRIu64 "\n", stats->keyword_plan_hits);
    fprintf(out, "Frames pushed: %" PRIu64 "\n", stats->frames_pushed);
    fprintf(out, "Frame objects created: %" PRIu64 "\n", stats->frame_objects_created);
    for (int i = 0; i < EVAL_CALL_KINDS; i++) {
        fprintf(out, "Calls via PyEval_EvalFrame[%d] : %" PRIu64 "\n", i, stats->eval_calls[i]);
    }
}

static void
print_object_stats(FILE *out, ObjectStats *stats)
{
    fprintf(out, "Object allocations from freelist: %" PRIu64 "\n", stats->from_freelist);
    fprintf(out, "Object frees to freelist: %" PRIu64 "\n", stats->to_freelist);
    fprintf(out, "Object allocations: %" PRIu64 "\n", stats->allocations);
    fprintf(out, "Object allocations to 512 bytes: %" PRIu64 "\n", stats->allocations512);
    fprintf(out, "Object allocations to 4 kbytes: %" PRIu64 "\n", stats->allocations4k);
    fprintf(out, "Object allocations over 4 kbytes: %" PRIu64 "\n", stats->allocations_big);
    fprintf(out, "Object frees: %" PRIu64 "\n", stats->frees);
    fprintf(out, "Object inline values: %" PRIu64 "\n", stats->inline_values);
    fprintf(out, "Object interpreter mortal increfs: %" PRIu64 "\n", stats->interpreter_increfs);
    fprintf(out, "Object interpreter mortal decrefs: %" PRIu64 "\n", stats->interpreter_decrefs);
    fprintf(out, "Object mortal increfs: %" PRIu64 "\n", stats->increfs);
    fprintf(out, "Object mortal decrefs: %" PRIu64 "\n", stats->decrefs);
    fprintf(out, "Object interpreter immortal increfs: %" PRIu64 "\n", stats->interpreter_immortal_increfs);
    fprintf(out, "Object interpreter immortal decrefs: %" PRIu64 "\n", stats->interpreter_immortal_decrefs);
    fprintf(out, "Object immortal increfs: %" PRIu64 "\n", stats->immortal_increfs);
    fprintf(out, "Object immortal decrefs: %" PRIu64 "\n", stats->immortal_decrefs);
    fprintf(out, "Object materialize dict (on request): %" PRIu64 "\n", stats->dict_materialized_on_request);
    fprintf(out, "Object materialize dict (new key): %" PRIu64 "\n", stats->dict_materialized_new_key);
    fprintf(out, "Object materialize dict (too big): %" PRIu64 "\n", stats->dict_materialized_too_big);
    fprintf(out, "Object materialize dict (str subclass): %" PRIu64 "\n", stats->dict_materialized_str_subclass);
    fprintf(out, "Object method cache hits: %" PRIu64 "\n", stats->type_cache_hits);
    fprintf(out, "Object method cache misses: %" PRIu64 "\n", stats->type_cache_misses);
    fprintf(out, "Object method cache too big: %" PRIu64 "\n", stats->type_cache_too_big);
    fprintf(out, "Object method cache dunder hits: %" PRIu64 "\n", stats->type_cache_dunder_hits);
    fprintf(out, "Object method cache dunder misses: %" PRIu64 "\n", stats->type_cache_dunder_misses);
    fprintf(out, "Object method cache invalidations: %" PRIu64 "\n", stats->type_cache_invalidations);
    fprintf(out, "Object method cache resizes: %" PRIu64 "\n", stats->type_cache_resizes);
    fprintf(out, "Object inline ints created: %" PRIu64 "\n", stats->inline_ints_created);
    fprintf(out, "Object inline floats created: %" PRIu64 "\n", stats->inline_floats_created);
    fprintf(out, "Object inline int boxes: %" PRIu64 "\n", stats->inline_int_boxes);
    fprintf(out, "Object inline float boxes: %" PRIu64 "\n", stats->inline_float_boxes);
    fprintf(out, "Object inline array boxes: %" PRIu64 "\n", stats->inline_array_boxes);
}

static const char *const box_site_names[BOX_SITE_KINDS] = {
    [BOX_SITE_UNATTRIBUTED] = "unattributed",
    [BOX_SITE_NUMBER] = "number",
    [BOX_SITE_COMPARE] = "compare",
    [BOX_SITE_REPR] = "repr",
    [BOX_SITE_SEQUENCE] = "sequence",
    [BOX_SITE_ITERATION] = "iteration",
    [BOX_SITE_ATTRIBUTE] = "attribute",
    [BOX_SITE_CALL] = "call",
    [BOX_SITE_LIST] = "list",
    [BOX_SITE_TUPLE] = "tuple",
    [BOX_SITE_DICT] = "dict",
    [BOX_SITE_SET] = "set",
    [BOX_SITE_CELL] = "cell/structseq",
    [BOX_SITE_CALL_ADAPTER] = "call adapter",
    [BOX_SITE_PARSER] = "parser",
    [BOX_SITE_INTERPRETER] = "interpreter",
    [BOX_SITE_FIELD] = "field",
    [BOX_SITE_FRAME] = "frame",
    [BOX_SITE_COMPILER] = "compiler",
    [BOX_SITE_TEST] = "test",
};

static const char *const box_kind_names[BOX_KINDS] = {
    [BOX_KIND_CORE] = "core",
    [BOX_KIND_THUNK] = "thunk",
    [BOX_KIND_SLOT_ADAPTER] = "slot adapter",
    [BOX_KIND_CALL_ADAPTER] = "call adapter",
};

/* The box table (pyjit/design/value-api.md, section 6.1): one line per site
   class and per kind, so that every dump has the same rows. */
static void
print_box_stats(FILE *out, ObjectStats *stats)
{
    for (int i = 0; i < BOX_KINDS; i++) {
        assert(box_kind_names[i] != NULL);
        fprintf(out, "Boxing by kind (%s): %" PRIu64 "\n",
                box_kind_names[i], stats->box_kind[i]);
    }
    for (int i = 0; i < BOX_SITE_KINDS; i++) {
        assert(box_site_names[i] != NULL);
        fprintf(out, "Boxing by site (%s): %" PRIu64 "\n",
                box_site_names[i], stats->box[i]);
    }
    fprintf(out, "Entry unboxing: %" PRIu64 "\n", stats->entry_unbox);
    fprintf(out, "Object box cached: %" PRIu64 "\n", stats->box_cached);
}

static void
print_gc_stats(FILE *out, GCStats *stats)
{
    for (int i = 0; i < NUM_GENERATIONS; i++) {
        fprintf(out, "GC[%d] collections: %" PRIu64 "\n", i, stats[i].collections);
        fprintf(out, "GC[%d] object visits: %" PRIu64 "\n", i, stats[i].object_visits);
        fprintf(out, "GC[%d] objects collected: %" PRIu64 "\n", i, stats[i].objects_collected);
        fprintf(out, "GC[%d] objects reachable from roots: %" PRIu64 "\n", i, stats[i].objects_transitively_reachable);
        fprintf(out, "GC[%d] objects not reachable from roots: %" PRIu64 "\n", i, stats[i].objects_not_transitively_reachable);
    }
}

#ifdef Py_GIL_DISABLED
static void
print_ft_stats(FILE *out, FTStats *stats)
{
    fprintf(out, "Mutex sleeps (mutex_sleeps): %" PRIu64 "\n", stats->mutex_sleeps);
    fprintf(out, "QSBR polls (qsbr_polls): %" PRIu64 "\n", stats->qsbr_polls);
    fprintf(out, "World stops (world_stops): %" PRIu64 "\n", stats->world_stops);
}
#endif

static void
print_rare_event_stats(FILE *out, RareEventStats *stats)
{
    fprintf(out, "Rare event (set_class): %" PRIu64 "\n", stats->set_class);
    fprintf(out, "Rare event (set_bases): %" PRIu64 "\n", stats->set_bases);
    fprintf(out, "Rare event (set_eval_frame_func): %" PRIu64 "\n", stats->set_eval_frame_func);
    fprintf(out, "Rare event (builtin_dict): %" PRIu64 "\n", stats->builtin_dict);
    fprintf(out, "Rare event (func_modification): %" PRIu64 "\n", stats->func_modification);
    fprintf(out, "Rare event (watched_dict_modification): %" PRIu64 "\n", stats->watched_dict_modification);
    fprintf(out, "Rare event (watched_globals_modification): %" PRIu64 "\n", stats->watched_globals_modification);
}

static void
print_stats(FILE *out, PyStats *stats)
{
    print_spec_stats(out, stats->opcode_stats);
    print_call_stats(out, &stats->call_stats);
    print_object_stats(out, &stats->object_stats);
    print_box_stats(out, &stats->object_stats);
    print_gc_stats(out, stats->gc_stats);
#ifdef Py_GIL_DISABLED
    print_ft_stats(out, &stats->ft_stats);
#endif
    print_rare_event_stats(out, &stats->rare_event_stats);
}

#ifdef Py_GIL_DISABLED

static void
merge_specialization_stats(SpecializationStats *dest, const SpecializationStats *src)
{
    dest->success += src->success;
    dest->failure += src->failure;
    dest->hit += src->hit;
    dest->deferred += src->deferred;
    dest->miss += src->miss;
    dest->deopt += src->deopt;
    for (int i = 0; i < SPECIALIZATION_FAILURE_KINDS; i++) {
        dest->failure_kinds[i] += src->failure_kinds[i];
    }
}

static void
merge_opcode_stats_array(OpcodeStats *dest, const OpcodeStats *src)
{
    for (int i = 0; i < IR_NUM_HANDLERS; i++) {
        merge_specialization_stats(&dest[i].specialization, &src[i].specialization);
        dest[i].execution_count += src[i].execution_count;
        dest[i].boxes += src[i].boxes;
        for (int j = 0; j < IR_NUM_HANDLERS; j++) {
            dest[i].pair_count[j] += src[i].pair_count[j];
        }
    }
}

static void
merge_call_stats(CallStats *dest, const CallStats *src)
{
    dest->inlined_py_calls += src->inlined_py_calls;
    dest->keyword_plan_hits += src->keyword_plan_hits;
    dest->pyeval_calls += src->pyeval_calls;
    dest->frames_pushed += src->frames_pushed;
    dest->frame_objects_created += src->frame_objects_created;
    for (int i = 0; i < EVAL_CALL_KINDS; i++) {
        dest->eval_calls[i] += src->eval_calls[i];
    }
}

static void
merge_object_stats(ObjectStats *dest, const ObjectStats *src)
{
    dest->increfs += src->increfs;
    dest->decrefs += src->decrefs;
    dest->interpreter_increfs += src->interpreter_increfs;
    dest->interpreter_decrefs += src->interpreter_decrefs;
    dest->immortal_increfs += src->immortal_increfs;
    dest->immortal_decrefs += src->immortal_decrefs;
    dest->interpreter_immortal_increfs += src->interpreter_immortal_increfs;
    dest->interpreter_immortal_decrefs += src->interpreter_immortal_decrefs;
    dest->allocations += src->allocations;
    dest->allocations512 += src->allocations512;
    dest->allocations4k += src->allocations4k;
    dest->allocations_big += src->allocations_big;
    dest->frees += src->frees;
    dest->to_freelist += src->to_freelist;
    dest->from_freelist += src->from_freelist;
    dest->inline_values += src->inline_values;
    dest->dict_materialized_on_request += src->dict_materialized_on_request;
    dest->dict_materialized_new_key += src->dict_materialized_new_key;
    dest->dict_materialized_too_big += src->dict_materialized_too_big;
    dest->dict_materialized_str_subclass += src->dict_materialized_str_subclass;
    dest->type_cache_hits += src->type_cache_hits;
    dest->type_cache_misses += src->type_cache_misses;
    dest->type_cache_dunder_hits += src->type_cache_dunder_hits;
    dest->type_cache_dunder_misses += src->type_cache_dunder_misses;
    dest->type_cache_too_big += src->type_cache_too_big;
    dest->type_cache_invalidations += src->type_cache_invalidations;
    dest->type_cache_resizes += src->type_cache_resizes;
    dest->inline_ints_created += src->inline_ints_created;
    dest->inline_floats_created += src->inline_floats_created;
    dest->inline_int_boxes += src->inline_int_boxes;
    dest->inline_float_boxes += src->inline_float_boxes;
    dest->inline_array_boxes += src->inline_array_boxes;
    for (int i = 0; i < BOX_SITE_KINDS; i++) {
        dest->box[i] += src->box[i];
    }
    dest->entry_unbox += src->entry_unbox;
    dest->box_cached += src->box_cached;
    for (int i = 0; i < BOX_KINDS; i++) {
        dest->box_kind[i] += src->box_kind[i];
    }
    dest->object_visits += src->object_visits;
}

static void
merge_ft_stats(FTStats *dest, const FTStats *src)
{
    dest->mutex_sleeps = src->mutex_sleeps;
    dest->qsbr_polls = src->qsbr_polls;
    dest->world_stops = src->world_stops;
}

static void
merge_rare_event_stats(RareEventStats *dest, const RareEventStats *src)
{
    dest->set_class += src->set_class;
    dest->set_bases += src->set_bases;
    dest->set_eval_frame_func += src->set_eval_frame_func;
    dest->builtin_dict += src->builtin_dict;
    dest->func_modification += src->func_modification;
    dest->watched_dict_modification += src->watched_dict_modification;
    dest->watched_globals_modification += src->watched_globals_modification;
}

static void
merge_gc_stats_array(GCStats *dest, const GCStats *src)
{
    for (int i = 0; i < NUM_GENERATIONS; i++) {
        dest[i].collections += src[i].collections;
        dest[i].object_visits += src[i].object_visits;
        dest[i].objects_collected += src[i].objects_collected;
        dest[i].objects_transitively_reachable += src[i].objects_transitively_reachable;
        dest[i].objects_not_transitively_reachable += src[i].objects_not_transitively_reachable;
    }
}

void
stats_zero_thread(_PyThreadStateImpl *tstate)
{
    // Zero the thread local stat counters
    if (tstate->pystats_struct) {
        memset(tstate->pystats_struct, 0, sizeof(PyStats));
    }
}

// merge stats for a single thread into the global structure
void
stats_merge_thread(_PyThreadStateImpl *tstate)
{
    PyStats *src = tstate->pystats_struct;
    PyStats *dest = ((PyThreadState *)tstate)->interp->pystats_struct;

    if (src == NULL || dest == NULL) {
        return;
    }

    // Merge each category of stats using the helper functions.
    merge_opcode_stats_array(dest->opcode_stats, src->opcode_stats);
    merge_call_stats(&dest->call_stats, &src->call_stats);
    merge_object_stats(&dest->object_stats, &src->object_stats);
    merge_ft_stats(&dest->ft_stats, &src->ft_stats);
    merge_rare_event_stats(&dest->rare_event_stats, &src->rare_event_stats);
    merge_gc_stats_array(dest->gc_stats, src->gc_stats);
}
#endif // Py_GIL_DISABLED

// toggle stats collection on or off for all threads
static int
stats_toggle_on_off(PyThreadState *tstate, int on)
{
    bool changed = false;
    PyInterpreterState *interp = tstate->interp;
    STATS_LOCK(interp);
    if (on && interp->pystats_struct == NULL) {
        PyStats *s = PyMem_RawCalloc(1, sizeof(PyStats));
        if (s == NULL) {
            STATS_UNLOCK(interp);
            return -1;
        }
        FT_ATOMIC_STORE_PTR_RELAXED(interp->pystats_struct, s);
    }
    if (tstate->interp->pystats_enabled != on) {
        FT_ATOMIC_STORE_INT_RELAXED(interp->pystats_enabled, on);
        changed = true;
    }
    STATS_UNLOCK(interp);
    if (!changed) {
        return 0;
    }
    _PyEval_StopTheWorld(interp);
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, ts) {
        PyStats *s = NULL;
        if (interp->pystats_enabled) {
#ifdef Py_GIL_DISABLED
            _PyThreadStateImpl *ts_impl = (_PyThreadStateImpl *)ts;
            if (ts_impl->pystats_struct == NULL) {
                // first activation for this thread, allocate structure
                ts_impl->pystats_struct = PyMem_RawCalloc(1, sizeof(PyStats));
            }
            s = ts_impl->pystats_struct;
#else
            s = ts->interp->pystats_struct;
#endif
        }
        ts->pystats = s;
    }
    _PyEval_StartTheWorld(interp);
    return 0;
}

// zero stats for all threads and for the interpreter
static void
stats_zero_all(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        return;
    }
    if (FT_ATOMIC_LOAD_PTR_RELAXED(tstate->interp->pystats_struct) == NULL) {
        return;
    }
    PyInterpreterState *interp = tstate->interp;
    _PyEval_StopTheWorld(interp);
#ifdef Py_GIL_DISABLED
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, ts) {
        stats_zero_thread((_PyThreadStateImpl *)ts);
    }
#endif
    if (interp->pystats_struct) {
        memset(interp->pystats_struct, 0, sizeof(PyStats));
    }
    _PyEval_StartTheWorld(interp);
}

// merge stats for all threads into the per-interpreter structure
static void
stats_merge_all(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
        return;
    }
    if (FT_ATOMIC_LOAD_PTR_RELAXED(tstate->interp->pystats_struct) == NULL) {
        return;
    }
    PyInterpreterState *interp = tstate->interp;
    _PyEval_StopTheWorld(interp);
#ifdef Py_GIL_DISABLED
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, ts) {
        stats_merge_thread((_PyThreadStateImpl *)ts);
        stats_zero_thread((_PyThreadStateImpl *)ts);
    }
#endif
    _PyEval_StartTheWorld(interp);
}

int
_Py_StatsOn(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    int res = stats_toggle_on_off(tstate, 1);
    if (res == 0) {
        run_marker_create(tstate->interp);
    }
    return res;
}

void
_Py_StatsOff(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    stats_toggle_on_off(tstate, 0);
    stats_resets++;
}

static void
stats_clear_all(void)
{
    stats_zero_all();
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate != NULL) {
        box_owners_clear(tstate->interp);
    }
}

// sys._stats_clear(): a reset, which the dump lists.
void
_Py_StatsClear(void)
{
    stats_clear_all();
    stats_resets++;
}

/* The owners of the boxes, and the accounting of the processes that make them
   (pyjit/design/proposals/value-api-g.md, section 1.4). */

typedef struct {
    const void *owner, *pc;   // inside the owning code
    uint64_t count;
    uint8_t kind, site;
} _PyBoxOwnerEntry;

#define BOX_OWNERS_BITS 16
#define BOX_OWNERS_SIZE (1 << BOX_OWNERS_BITS)
#define BOX_OWNERS_PROBE 64

struct _PyBoxOwners {
    uint64_t dropped[BOX_KINDS];
    _PyBoxOwnerEntry entries[BOX_OWNERS_SIZE];
};

struct _PyBoxAccum {
    ObjectStats objects;
    struct _PyBoxOwners *owners;
};

// ObjectStats is a struct of uint64_t counters.
static_assert(sizeof(ObjectStats) % sizeof(uint64_t) == 0,
              "ObjectStats holds only uint64_t counters");
static void
object_stats_add(ObjectStats *dest, const ObjectStats *src)
{
    uint64_t *d = (uint64_t *)dest;
    const uint64_t *s = (const uint64_t *)src;
    for (size_t i = 0; i < sizeof(ObjectStats) / sizeof(uint64_t); i++) {
        d[i] += s[i];
    }
}


static void
box_owners_add(struct _PyBoxOwners *owners, uint8_t site, uint8_t kind,
               const void *owner, const void *pc, uint64_t count)
{
    uint64_t h = (uint64_t)(uintptr_t)owner * 0x9E3779B97F4A7C15ULL
                 ^ (uint64_t)(uintptr_t)pc * 0xC2B2AE3D27D4EB4FULL
                 ^ (uint64_t)(kind << 8 | site);
    h ^= h >> 29;
    for (int i = 0; i < BOX_OWNERS_PROBE; i++) {
        _PyBoxOwnerEntry *e = &owners->entries[(h + i) & (BOX_OWNERS_SIZE - 1)];
        if (e->count == 0) {
            e->owner = owner;
            e->pc = pc;
            e->kind = kind;
            e->site = site;
            e->count = count;
            return;
        }
        if (e->owner == owner && e->pc == pc && e->kind == kind
            && e->site == site)
        {
            e->count += count;
            return;
        }
    }
    owners->dropped[kind] += count;
}

static void
box_owners_merge(struct _PyBoxOwners *dest, const struct _PyBoxOwners *src)
{
    for (int k = 0; k < BOX_KINDS; k++) {
        dest->dropped[k] += src->dropped[k];
    }
    for (size_t i = 0; i < BOX_OWNERS_SIZE; i++) {
        const _PyBoxOwnerEntry *e = &src->entries[i];
        if (e->count != 0) {
            box_owners_add(dest, e->site, e->kind, e->owner, e->pc, e->count);
        }
    }
}

void
_PyStats_BoxOwner(uint8_t site, uint8_t kind, const void *owner,
                  const void *pc)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->box_owners == NULL) {
        interp->box_owners = PyMem_RawCalloc(1, sizeof(struct _PyBoxOwners));
        if (interp->box_owners == NULL) {
            return;
        }
    }
    box_owners_add(interp->box_owners, site, kind, owner, pc, 1);
}

static void
box_owners_clear(PyInterpreterState *interp)
{
    if (interp->box_owners != NULL) {
        memset(interp->box_owners, 0, sizeof(struct _PyBoxOwners));
    }
}

// A deleted interpreter hands its counters and table to the runtime.
void
_PyStats_InterpFini(PyInterpreterState *interp)
{
    if (interp != _PyInterpreterState_Main()
        && (interp->pystats_struct != NULL || interp->box_owners != NULL))
    {
        PyMutex_Lock(&_PyRuntime.pystats_accum_mutex);
        struct _PyBoxAccum *accum = _PyRuntime.pystats_accum;
        if (accum == NULL) {
            accum = PyMem_RawCalloc(1, sizeof(struct _PyBoxAccum));
            _PyRuntime.pystats_accum = accum;
        }
        if (accum != NULL) {
            if (interp->pystats_struct != NULL) {
                object_stats_add(&accum->objects,
                                 &interp->pystats_struct->object_stats);
            }
            if (interp->box_owners != NULL) {
                if (accum->owners == NULL) {
                    accum->owners = PyMem_RawCalloc(
                        1, sizeof(struct _PyBoxOwners));
                }
                if (accum->owners != NULL) {
                    box_owners_merge(accum->owners, interp->box_owners);
                }
            }
        }
        PyMutex_Unlock(&_PyRuntime.pystats_accum_mutex);
    }
    PyMem_RawFree(interp->box_owners);
    interp->box_owners = NULL;
}

// Add the accumulator to the interpreter that dumps, then empty it.
static void
accum_into(PyInterpreterState *interp, PyStats *stats)
{
    PyMutex_Lock(&_PyRuntime.pystats_accum_mutex);
    struct _PyBoxAccum *accum = _PyRuntime.pystats_accum;
    if (accum != NULL) {
        object_stats_add(&stats->object_stats, &accum->objects);
        memset(&accum->objects, 0, sizeof(accum->objects));
        if (accum->owners != NULL) {
            if (interp->box_owners == NULL) {
                interp->box_owners = PyMem_RawCalloc(
                    1, sizeof(struct _PyBoxOwners));
            }
            if (interp->box_owners != NULL) {
                box_owners_merge(interp->box_owners, accum->owners);
            }
            memset(accum->owners, 0, sizeof(struct _PyBoxOwners));
        }
    }
    PyMutex_Unlock(&_PyRuntime.pystats_accum_mutex);
}

/* ---- The dump directory, run markers ---- */

#define STATS_DIR_DEFAULT_POSIX "/tmp/py_stats"

// The dump directory: -X pystats_dir, else PYTHON_PYSTATS_DIR, else the
// shared default. Both are read under -E and -I.
static void
stats_dump_dir(PyInterpreterState *interp, char *buf, size_t size)
{
# ifdef MS_WINDOWS
    const char *dirname = "c:\\temp\\py_stats";
# else
    const char *dirname = STATS_DIR_DEFAULT_POSIX;
# endif
    const char *env_dir = getenv("PYTHON_PYSTATS_DIR");
    if (env_dir != NULL && env_dir[0] != '\0') {
        dirname = env_dir;
    }
    const PyWideStringList *xoptions = &interp->config.xoptions;
    for (Py_ssize_t i = 0; i < xoptions->length; i++) {
        const wchar_t *item = xoptions->items[i];
        if (wcsncmp(item, L"pystats_dir=", 12) == 0 && item[12] != L'\0') {
            char tmp[MAXPATHLEN + 1];
            size_t n = wcstombs(tmp, item + 12, sizeof(tmp));
            if (n != (size_t)-1 && n < sizeof(tmp)) {
                PyOS_snprintf(buf, size, "%s", tmp);
                return;
            }
        }
    }
    PyOS_snprintf(buf, size, "%s", dirname);
}

/* A unique 40-digit name. It cannot use os.urandom's source, which needs a
   thread state, and the marker is made during interpreter creation. The
   process id and the clock make it unique among the processes of a run. */
static void
stats_random_stem(char *hex_name)
{
    static unsigned int serial;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    unsigned long long ns = (unsigned long long)now.tv_sec * 1000000000ULL
                            + (unsigned long long)now.tv_nsec;
    unsigned int pid =
#ifndef MS_WINDOWS
        (unsigned int)getpid();
#else
        0;
#endif
    snprintf(hex_name, 41, "%016llx%08x%08x%08x", ns, pid, serial++,
             (unsigned int)(uintptr_t)hex_name);
}

static uint64_t child_receipt_errors;

#ifndef MS_WINDOWS
// The run marker of this process: <stem>.run, replaced by its dump.
static char run_stem[41];
static pid_t run_pid;
static uint64_t child_receipt_serial;
static unsigned long long run_image_dev, run_image_ino;

static int
receipt_write(int fd, const char *text, size_t length)
{
    while (length != 0) {
        ssize_t written = write(fd, text, length);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return -1;
        }
        text += written;
        length -= (size_t)written;
    }
    return 0;
}

#define RECEIPT_TEXT(FD, S) receipt_write((FD), (S), strlen(S))

void
_PyStats_CopyIntervalStem(char out[41])
{
    /* A configured process remains accountable while collection is toggled
       off. Only a process that never allocated stats is outside this gate. */
    if (_PyInterpreterState_GET()->pystats_struct == NULL) {
        out[0] = '\0';
        return;
    }
    if (run_pid == getpid() && run_stem[0] != '\0') {
        memcpy(out, run_stem, 41);
    }
    else {
        out[0] = '\0';
        child_receipt_errors++;
    }
}

/* This path runs in the parent, after fork locks have been released. It uses
   libc I/O only, so receipt failure cannot change the Python API result. */
void
_PyStats_ChildCreated(pid_t child, const char *source,
                      const char *const *candidates, size_t candidate_count,
                      const char parent_stem[41])
{
    int saved_errno = errno;
    if (parent_stem[0] == '\0') {
        errno = saved_errno;
        return;
    }
    char dir[MAXPATHLEN + 1], path[MAXPATHLEN + 1], tmp[MAXPATHLEN + 1];
    char item[512];
    int fd = -1;
    unsigned long long serial = ++child_receipt_serial;
    if (child <= 0 || candidate_count > 4096) {
        goto failed;
    }
    stats_dump_dir(_PyInterpreterState_GET(), dir, sizeof(dir));
    int n = PyOS_snprintf(path, sizeof(path), "%s/%s.%llu.child", dir,
                          parent_stem, serial);
    int n2 = PyOS_snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n <= 0 || (size_t)n >= sizeof(path) ||
        n2 <= 0 || (size_t)n2 >= sizeof(tmp)) {
        goto failed;
    }
    fd = open(tmp, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        goto failed;
    }
    n = PyOS_snprintf(item, sizeof(item),
        "{\"version\":1,\"parent_pid\":%d,\"child_pid\":%d,"
        "\"parent_stem\":\"%s\",\"serial\":%llu,\"source\":\"%s\","
        "\"parent_image_dev\":%llu,\"parent_image_ino\":%llu,"
        "\"candidate_count\":%zu,\"candidates\":[",
        (int)getpid(), (int)child, parent_stem, serial, source,
        run_image_dev, run_image_ino, candidate_count);
    if (n <= 0 || (size_t)n >= sizeof(item) ||
        receipt_write(fd, item, (size_t)n) < 0) {
        goto failed;
    }
    for (size_t i = 0; i < candidate_count; i++) {
        if (candidates[i] == NULL ||
            RECEIPT_TEXT(fd, i == 0 ? "\"" : ",\"") < 0) {
            goto failed;
        }
        const unsigned char *p = (const unsigned char *)candidates[i];
        for (; *p != '\0'; p++) {
            char escaped[7];
            size_t length;
            if (*p == '"' || *p == '\\') {
                escaped[0] = '\\'; escaped[1] = (char)*p; length = 2;
            }
            else if (*p < 0x20 || *p >= 0x7f) {
                static const char hex[] = "0123456789abcdef";
                escaped[0] = '\\'; escaped[1] = 'u';
                escaped[2] = '0'; escaped[3] = '0';
                escaped[4] = hex[*p >> 4]; escaped[5] = hex[*p & 15];
                length = 6;
            }
            else {
                escaped[0] = (char)*p; length = 1;
            }
            if (receipt_write(fd, escaped, length) < 0) {
                goto failed;
            }
        }
        if (RECEIPT_TEXT(fd, "\"") < 0) {
            goto failed;
        }
    }
    if (RECEIPT_TEXT(fd, "]}\n") < 0) {
        goto failed;
    }
    if (close(fd) < 0) {
        fd = -1;
        goto failed;
    }
    fd = -1;
    if (rename(tmp, path) < 0) {
        goto failed;
    }
    errno = saved_errno;
    return;
failed:
    child_receipt_errors++;
    if (fd >= 0) {
        (void)close(fd);
    }
    errno = saved_errno;
}

#undef RECEIPT_TEXT

/* Keep argv boundaries and control bytes on one marker line. */
static void
run_marker_argv(FILE *out, FILE *cmd)
{
    fputc('[', out);
    int c;
    int open = 0;
    int first = 1;
    while ((c = fgetc(cmd)) != EOF) {
        if (!open) {
            if (!first) {
                fputc(',', out);
            }
            fputc('"', out);
            open = 1;
            first = 0;
        }
        if (c == '\0') {
            fputc('"', out);
            open = 0;
        }
        else if (c == '"' || c == '\\') {
            fputc('\\', out);
            fputc(c, out);
        }
        else if (c < 0x20 || c >= 0x7f) {
            fprintf(out, "\\u%04x", c);
        }
        else {
            fputc(c, out);
        }
    }
    if (open) {
        fputc('"', out);
    }
    fputc(']', out);
}

static void
run_marker_create(PyInterpreterState *interp)
{
    if (run_pid == getpid() && run_stem[0] != '\0') {
        return;
    }
    char dir[MAXPATHLEN + 1], path[MAXPATHLEN + 1];
    stats_dump_dir(interp, dir, sizeof(dir));
    char stem[41];
    stats_random_stem(stem);
    int len = PyOS_snprintf(path, sizeof(path), "%s/%s.run", dir, stem);
    if (len <= 0 || (size_t)len >= sizeof(path)) {
        return;
    }
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        return;
    }
    const char *tag = getenv("PYTHON_PYSTATS_TAG");
    struct stat image;
    int image_ok = stat("/proc/self/exe", &image) == 0;
    run_image_dev = image_ok ? (unsigned long long)image.st_dev : 0;
    run_image_ino = image_ok ? (unsigned long long)image.st_ino : 0;
    if (!image_ok) {
        child_receipt_errors++;
    }
    fprintf(f, "version: 2\npid: %d\nppid: %d\ntag: %s\nargv: ",
            (int)getpid(), (int)getppid(), tag != NULL ? tag : "");
    FILE *cmd = fopen("/proc/self/cmdline", "r");
    if (cmd != NULL) {
        run_marker_argv(f, cmd);
        fclose(cmd);
    }
    fprintf(f, "\nimage_dev: %llu\nimage_ino: %llu\n",
            run_image_dev, run_image_ino);
    fclose(f);
    memcpy(run_stem, stem, sizeof(stem));
    run_pid = getpid();
}

static void
run_marker_finish(PyInterpreterState *interp, const char *outcome)
{
    if (run_pid == getpid() && run_stem[0] != '\0') {
        char dir[MAXPATHLEN + 1], path[MAXPATHLEN + 1];
        char finished[MAXPATHLEN + 1];
        stats_dump_dir(interp, dir, sizeof(dir));
        int n = PyOS_snprintf(path, sizeof(path), "%s/%s.run", dir, run_stem);
        int n2 = PyOS_snprintf(finished, sizeof(finished), "%s/%s.%s",
                              dir, run_stem, outcome);
        if (n > 0 && (size_t)n < sizeof(path) &&
            n2 > 0 && (size_t)n2 < sizeof(finished)) {
            /* Keep the start-time pid, ppid, tag and argv after a dump. A
               failed rename leaves the .run file for the gate to reject. */
            (void)rename(path, finished);
        }
    }
    run_stem[0] = '\0';
}
#else
static void run_marker_create(PyInterpreterState *interp) { (void)interp; }
static void run_marker_finish(PyInterpreterState *interp, const char *outcome)
{ (void)interp; (void)outcome; }
#endif

void
_Py_StatsMarkerCreate(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->pystats_enabled) {
        run_marker_create(interp);
    }
}

/* ---- Objects: where a stored address lives ---- */

#ifdef __linux__
#include <link.h>
#include <sys/stat.h>
#include <fcntl.h>

#define STATS_OBJ_SEGMENTS 16

typedef struct {
    char *path;
    uintptr_t bias;
    char build_id[129];
    uintptr_t lo[STATS_OBJ_SEGMENTS], hi[STATS_OBJ_SEGMENTS];
    int nseg;
    int index;
} stats_obj;

typedef struct {
    stats_obj *v;
    size_t n, cap;
} stats_objs;

static int
stats_objs_collect(struct dl_phdr_info *info, size_t size, void *data)
{
    stats_objs *objs = data;
    if (objs->n == objs->cap) {
        size_t cap = objs->cap ? objs->cap * 2 : 64;
        // Stats reporting must still resolve owners when Python's allocator
        // is deliberately failing, as in test.support.inject_memory_error().
        stats_obj *v = realloc(objs->v, cap * sizeof(stats_obj));
        if (v == NULL) {
            return 1;
        }
        objs->v = v;
        objs->cap = cap;
    }
    stats_obj *o = &objs->v[objs->n];
    memset(o, 0, sizeof(*o));
    o->index = -1;
    o->bias = info->dlpi_addr;
    char exe[MAXPATHLEN + 1];
    const char *name = info->dlpi_name;
    if (name == NULL || name[0] == '\0') {
        ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (n < 0) {
            n = 0;
        }
        exe[n] = '\0';
        name = exe;
    }
    size_t len = strlen(name) + 1;
    o->path = malloc(len);
    if (o->path == NULL) {
        return 1;
    }
    memcpy(o->path, name, len);
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
        if (ph->p_type == PT_LOAD && o->nseg < STATS_OBJ_SEGMENTS) {
            o->lo[o->nseg] = info->dlpi_addr + ph->p_vaddr;
            o->hi[o->nseg] = o->lo[o->nseg] + ph->p_memsz;
            o->nseg++;
        }
        else if (ph->p_type == PT_NOTE) {
            const char *p = (const char *)(info->dlpi_addr + ph->p_vaddr);
            const char *end = p + ph->p_memsz;
            while (p + sizeof(ElfW(Nhdr)) <= end) {
                const ElfW(Nhdr) *nh = (const ElfW(Nhdr) *)p;
                const char *nm = p + sizeof(*nh);
                const char *desc = nm + ((nh->n_namesz + 3) & ~3u);
                if (nh->n_type == NT_GNU_BUILD_ID && nh->n_namesz == 4
                    && memcmp(nm, "GNU", 4) == 0 && nh->n_descsz <= 64)
                {
                    for (unsigned j = 0; j < nh->n_descsz; j++) {
                        sprintf(o->build_id + 2*j, "%02x",
                                (unsigned char)desc[j]);
                    }
                }
                p = desc + ((nh->n_descsz + 3) & ~3u);
            }
        }
    }
    objs->n++;
    return 0;
}

static stats_obj *
stats_objs_find(stats_objs *objs, uintptr_t addr)
{
    for (size_t i = 0; i < objs->n; i++) {
        stats_obj *o = &objs->v[i];
        for (int s = 0; s < o->nseg; s++) {
            if (o->lo[s] <= addr && addr < o->hi[s]) {
                return o;
            }
        }
    }
    return NULL;
}

// Copy an object outside the build to <dir>/objects/<build-id>, once, so
// that a test's temporary extension stays resolvable after it is deleted.
static void
stats_preserve_object(const char *dir, const stats_obj *o, const char *exedir)
{
    if (o->build_id[0] == '\0'
        || strncmp(o->path, exedir, strlen(exedir)) == 0)
    {
        return;
    }
    char objdir[MAXPATHLEN + 1], dest[MAXPATHLEN + 1], tmp[MAXPATHLEN + 1];
    PyOS_snprintf(objdir, sizeof(objdir), "%s/objects", dir);
    mkdir(objdir, 0777);
    PyOS_snprintf(dest, sizeof(dest), "%s/%s", objdir, o->build_id);
    if (access(dest, F_OK) == 0) {
        return;
    }
    PyOS_snprintf(tmp, sizeof(tmp), "%s.%d.tmp", dest, (int)getpid());
    int in = open(o->path, O_RDONLY);
    if (in < 0) {
        return;
    }
    int out = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out >= 0) {
        char chunk[65536];
        ssize_t n;
        int ok = 1;
        while ((n = read(in, chunk, sizeof(chunk))) > 0) {
            if (write(out, chunk, n) != n) {
                ok = 0;
                break;
            }
        }
        close(out);
        if (ok && n == 0) {
            rename(tmp, dest);
        }
        else {
            unlink(tmp);
        }
    }
    close(in);
}
#endif  // __linux__

// "IDX+0xOFF" for an address, assigning object indices; "?" in no object.
#ifdef __linux__
static void
format_addr(stats_objs *objs, int *next, uintptr_t addr, char *buf,
            size_t size)
{
    stats_obj *o = stats_objs_find(objs, addr);
    if (o == NULL) {
        PyOS_snprintf(buf, size, "?");
        return;
    }
    if (o->index < 0) {
        o->index = (*next)++;
    }
    PyOS_snprintf(buf, size, "%d+0x%" PRIxPTR, o->index, addr - o->bias);
}
#endif

static void
print_box_owners(FILE *out, PyInterpreterState *interp, const char *dir)
{
    for (int k = 0; k < BOX_KINDS; k++) {
        fprintf(out, "Box owners dropped (%s): %" PRIu64 "\n",
                box_kind_names[k],
                interp->box_owners != NULL ? interp->box_owners->dropped[k] : 0);
    }
    fprintf(out, "Stats resets: %" PRIu64 "\n", stats_resets);
    stats_resets = 0;
    if (interp->box_owners == NULL) {
        return;
    }
#ifdef __linux__
    stats_objs objs = {0};
    dl_iterate_phdr(stats_objs_collect, &objs);
    int next = 0;
    char a[64], b[64];
    for (int pass = 0; pass < 2; pass++) {
        for (size_t i = 0; i < BOX_OWNERS_SIZE; i++) {
            const _PyBoxOwnerEntry *e = &interp->box_owners->entries[i];
            if (e->count == 0) {
                continue;
            }
            uintptr_t owner = (uintptr_t)e->owner;
            if (owner > 1) {
                format_addr(&objs, &next, owner, a, sizeof(a));
            }
            else {
                PyOS_snprintf(a, sizeof(a), owner == 0 ? "-" : "foreign");
            }
            format_addr(&objs, &next, (uintptr_t)e->pc, b, sizeof(b));
            if (pass == 1) {
                fprintf(out, "Box owner (%s, %s) %s at %s: %" PRIu64 "\n",
                        box_kind_names[e->kind], box_site_names[e->site],
                        a, b, e->count);
            }
        }
        if (pass == 0) {
            char exe[MAXPATHLEN + 1];
            ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
            exe[n < 0 ? 0 : n] = '\0';
            char *slash = strrchr(exe, '/');
            if (slash != NULL) {
                *slash = '\0';
            }
            for (size_t j = 0; j < objs.n; j++) {
                stats_obj *o = &objs.v[j];
                if (o->index >= 0) {
                    fprintf(out, "Box object [%d]: %s build-id=%s\n",
                            o->index, o->path, o->build_id);
                    stats_preserve_object(dir, o, exe);
                }
            }
        }
    }
    for (size_t j = 0; j < objs.n; j++) {
        free(objs.v[j].path);
    }
    free(objs.v);
#else
    for (size_t i = 0; i < BOX_OWNERS_SIZE; i++) {
        const _PyBoxOwnerEntry *e = &interp->box_owners->entries[i];
        if (e->count != 0) {
            fprintf(out, "Box owner (%s, %s) ? at ?: %" PRIu64 "\n",
                    box_kind_names[e->kind], box_site_names[e->site],
                    e->count);
        }
    }
#endif
}

static int
mem_is_zero(unsigned char *ptr, size_t size)
{
    for (size_t i=0; i < size; i++) {
        if (*ptr != 0) {
            return 0;
        }
        ptr++;
    }
    return 1;
}

int
_Py_PrintSpecializationStats(int to_file)
{
    assert(to_file);
    stats_merge_all();
    PyThreadState *tstate = _PyThreadState_GET();
    PyInterpreterState *interp = tstate->interp;
    STATS_LOCK(interp);
    PyStats *stats = interp->pystats_struct;
    if (stats == NULL) {
        STATS_UNLOCK(interp);
        return 0;
    }
    accum_into(interp, stats);
#define MEM_IS_ZERO(DATA) mem_is_zero((unsigned char*)DATA, sizeof(*(DATA)))
    int is_zero = (
        MEM_IS_ZERO(stats->gc_stats)  // is a pointer
        && MEM_IS_ZERO(&stats->opcode_stats)
        && MEM_IS_ZERO(&stats->call_stats)
        && MEM_IS_ZERO(&stats->object_stats)
    );
#undef MEM_IS_ZERO
    STATS_UNLOCK(interp);
    if (is_zero) {
        // gh-108753: -X pystats command line was used, but then _stats_off()
        // and _stats_clear() have been called: in this case, avoid printing
        // useless "all zeros" statistics.
        run_marker_finish(interp, "zero");
        return 0;
    }

    FILE *out = stderr;
    char dir[MAXPATHLEN + 1] = "";
    char tmp_path[MAXPATHLEN + 1] = "", final_path[MAXPATHLEN + 1] = "";
    stats_dump_dir(interp, dir, sizeof(dir));
    /* The file is named by the run marker's stem, which it replaces, so
       that a process that never dumps is the marker left over. It is written
       as <stem>.tmp and renamed, so that a reader never sees half a dump. */
    char stem[41];
#ifndef MS_WINDOWS
    if (run_pid == getpid() && run_stem[0] != '\0') {
        memcpy(stem, run_stem, sizeof(stem));
    }
    else
#endif
    {
        stats_random_stem(stem);
    }
    int len = PyOS_snprintf(tmp_path, sizeof(tmp_path), "%s/%s.tmp", dir,
                            stem);
    int len2 = PyOS_snprintf(final_path, sizeof(final_path), "%s/%s.txt", dir,
                             stem);
    FILE *fout = (len > 0 && (size_t)len < sizeof(tmp_path)
                  && len2 > 0 && (size_t)len2 < sizeof(final_path))
                 ? fopen(tmp_path, "w") : NULL;
    if (fout) {
        out = fout;
    }
    STATS_LOCK(interp);
    fprintf(out, "Stats process: pid=%d tag=%s\n",
#ifndef MS_WINDOWS
            (int)getpid(),
#else
            0,
#endif
            getenv("PYTHON_PYSTATS_TAG") != NULL
                ? getenv("PYTHON_PYSTATS_TAG") : "");
    fprintf(out, "Stats child receipt errors: %llu\n",
            (unsigned long long)child_receipt_errors);
    print_stats(out, stats);
    print_box_owners(out, interp, dir);
    STATS_UNLOCK(interp);
    if (out != stderr) {
        fclose(out);
        if (rename(tmp_path, final_path) == 0) {
            run_marker_finish(interp, "done");
        }
    }
    return 1;
}

// Dump, then clear without counting a reset: nothing is lost.
int
_Py_StatsDumpClear(void)
{
    int res = _Py_PrintSpecializationStats(1);
    stats_clear_all();
    return res;
}

// A forked child counts for itself: the parent's counts are its parent's.
void
_Py_StatsAfterFork(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->pystats_struct != NULL) {
        memset(interp->pystats_struct, 0, sizeof(PyStats));
    }
    box_owners_clear(interp);
    PyMutex_Lock(&_PyRuntime.pystats_accum_mutex);
    if (_PyRuntime.pystats_accum != NULL) {
        if (_PyRuntime.pystats_accum->owners != NULL) {
            memset(_PyRuntime.pystats_accum->owners, 0,
                   sizeof(struct _PyBoxOwners));
        }
        memset(&_PyRuntime.pystats_accum->objects, 0,
               sizeof(ObjectStats));
    }
    PyMutex_Unlock(&_PyRuntime.pystats_accum_mutex);
    stats_resets = 0;
#ifndef MS_WINDOWS
    run_stem[0] = '\0';
    child_receipt_serial = 0;
    child_receipt_errors = 0;
#endif
    if (interp->pystats_enabled) {
        run_marker_create(interp);
    }
}

PyStatus
_PyStats_InterpInit(PyInterpreterState *interp)
{
    if (interp->config._pystats) {
        // start with pystats enabled, can be disabled via sys._stats_off()
        // this needs to be set before the first tstate is created
        interp->pystats_enabled = 1;
        interp->pystats_struct = PyMem_RawCalloc(1, sizeof(PyStats));
        if (interp->pystats_struct == NULL) {
            return _PyStatus_ERR("out-of-memory while initializing interpreter");
        }
        run_marker_create(interp);
    }
    return _PyStatus_OK();
}

bool
_PyStats_ThreadInit(PyInterpreterState *interp, _PyThreadStateImpl *tstate)
{
#ifdef Py_GIL_DISABLED
    if (FT_ATOMIC_LOAD_INT_RELAXED(interp->pystats_enabled)) {
        assert(interp->pystats_struct != NULL);
        tstate->pystats_struct = PyMem_RawCalloc(1, sizeof(PyStats));
        if (tstate->pystats_struct == NULL) {
            return false;
        }
    }
#endif
    return true;
}

void
_PyStats_ThreadFini(_PyThreadStateImpl *tstate)
{
#ifdef Py_GIL_DISABLED
    STATS_LOCK(((PyThreadState *)tstate)->interp);
    stats_merge_thread(tstate);
    STATS_UNLOCK(((PyThreadState *)tstate)->interp);
    PyMem_RawFree(tstate->pystats_struct);
#endif
}

void
_PyStats_Attach(_PyThreadStateImpl *tstate_impl)
{
    PyStats *s;
    PyThreadState *tstate = (PyThreadState *)tstate_impl;
    PyInterpreterState *interp = tstate->interp;
    if (FT_ATOMIC_LOAD_INT_RELAXED(interp->pystats_enabled)) {
#ifdef Py_GIL_DISABLED
        s = ((_PyThreadStateImpl *)tstate)->pystats_struct;
#else
        s = tstate->interp->pystats_struct;
#endif
    }
    else {
        s = NULL;
    }
    tstate->pystats = s;
}

void
_PyStats_Detach(_PyThreadStateImpl *tstate_impl)
{
    ((PyThreadState *)tstate_impl)->pystats = NULL;
}

#endif // Py_STATS
