#ifndef Py_INTERNAL_AUDIT_H
#define Py_INTERNAL_AUDIT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* Runtime audit hook state */

typedef struct _Py_AuditHookEntry {
    struct _Py_AuditHookEntry *next;
    Py_AuditHookFunction hookCFunction;
    void *userData;
} _Py_AuditHookEntry;


extern int _PySys_Audit(
    PyThreadState *tstate,
    const char *event,
    const char *argFormat,
    ...);

// True if some audit hook, or a DTrace probe, will see an event. A caller
// that must build an argument only for the event tests it first.
extern int _PySys_AuditEnabled(PyThreadState *tstate);

// _PySys_ClearAuditHooks() must not be exported: use extern rather than
// PyAPI_FUNC(). We want minimal exposure of this function.
extern void _PySys_ClearAuditHooks(PyThreadState *tstate);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_AUDIT_H */
