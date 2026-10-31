#ifndef Py_INTERNAL_RANGE_H
#define Py_INTERNAL_RANGE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    long start;
    long step;
    long len;
} _PyRangeIterObject;

/* v in r for a range r: 1, 0, or -1 if the bounds of r are not all compact
   ints, without an exception set. */
extern int _PyRange_ContainsInt32(PyObject *r, int32_t v);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_RANGE_H */
