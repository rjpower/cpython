#ifndef Py_INTERNAL_GC_WALK_H
#define Py_INTERNAL_GC_WALK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Precise C frames (boil/design/11-precise-roots.md, section 2).
 *
 * Py_GC_PRECISE on a function's definition, or on a prior declaration,
 * places it in section py_gc_precise.  The collector's stack walk scans no
 * frame whose pc lies in that section: the function keeps every object it
 * holds across a collecting call anchored where the collector sees it (a
 * frame slot, a pushed value span, a traced field, a static).  The build
 * runs root_check's C mode (Tools/cases_generator/root_check_c.py) over
 * every file that uses the marker, so a function marked before it passes
 * the check fails the build.
 *
 * Py_GC_ARGS_ANCHORED: a static precise function whose callers anchor its
 * arguments; the C mode checks every call, and that it is called only from
 * precise functions and never through its address.
 *
 * Py_GC_SLOTS on a PyValue * parameter: it points into a frame's slots,
 * which the collector visits, so a load through it is anchored until a
 * store through any PyValue *. */

#if defined(__ELF__) && (defined(__GNUC__) || defined(__clang__))
#  define Py_GC_PRECISE __attribute__((section("py_gc_precise")))
#else
#  define Py_GC_PRECISE
#endif
#define Py_GC_ARGS_ANCHORED
#define Py_GC_SLOTS

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_WALK_H */
