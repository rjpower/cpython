#ifndef Py_INTERNAL_PYSTATS_H
#define Py_INTERNAL_PYSTATS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_STATS
extern int _Py_StatsOn(void);
extern void _Py_StatsOff(void);
extern void _Py_StatsClear(void);
extern int _Py_PrintSpecializationStats(int to_file);
/* The stats dump and clear of sys._stats_dump(), os._exit() and os.exec*();
   the dump is not a reset. */
extern int _Py_StatsDumpClear(void);
// A new run marker, for a process that goes on counting after a dump.
extern void _Py_StatsMarkerCreate(void);
/* The owner table of the box primitive (pyjit/design/proposals/value-api-g.md,
   section 1.4): a box of kind KIND under site class SITE, owned by the code
   at OWNER (NULL: none), made at PC. */
extern void _PyStats_BoxOwner(uint8_t site, uint8_t kind,
                              const void *owner, const void *pc);
// Interpreter deletion and fork: the process accounting of section 1.4.
extern void _PyStats_InterpFini(PyInterpreterState *interp);
extern void _Py_StatsAfterFork(void);
/* Linux stats only: snapshot the parent interval before after-fork hooks,
   then publish one receipt after a successful native child creation. */
PyAPI_FUNC(void) _PyStats_CopyIntervalStem(char out[41]);
PyAPI_FUNC(void) _PyStats_ChildCreated(pid_t child, const char *source,
                                  const char *const *candidates,
                                  size_t candidate_count,
                                  const char parent_stem[41]);
#endif

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYSTATS_H
