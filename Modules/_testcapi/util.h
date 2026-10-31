#define NULLABLE(x) do {                    \
        if (x == Py_None) {                 \
            x = NULL;                       \
        }                                   \
    } while (0);

#define RETURN_INT(value) do {              \
        int _ret = (value);                 \
        if (_ret == -1) {                   \
            assert(PyErr_Occurred());       \
            return NULL;                    \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyLong_FromLong(_ret);       \
    } while (0)

#define RETURN_UINT(value) do {             \
        unsigned int _ret = (value);        \
        if (_ret == (unsigned int)-1) {     \
            assert(PyErr_Occurred());       \
            return NULL;                    \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyLong_FromUnsignedLong(_ret); \
    } while (0)

#define RETURN_SIZE(value) do {             \
        Py_ssize_t _ret = (value);          \
        if (_ret == -1) {                   \
            assert(PyErr_Occurred());       \
            return NULL;                    \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyLong_FromSsize_t(_ret);    \
    } while (0)

/* RETURN_INT(), RETURN_UINT() and RETURN_SIZE() for a function that returns
   a PyValue. */
#define RETURN_INT_VALUE(value) do {        \
        int _ret = (value);                 \
        if (_ret == -1) {                   \
            assert(PyErr_Occurred());       \
            return PyValue_NULL;            \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyValue_FromResult(PyLong_FromLong(_ret)); \
    } while (0)

#define RETURN_UINT_VALUE(value) do {       \
        unsigned int _ret = (value);        \
        if (_ret == (unsigned int)-1) {     \
            assert(PyErr_Occurred());       \
            return PyValue_NULL;            \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyValue_FromResult(PyLong_FromUnsignedLong(_ret)); \
    } while (0)

#define RETURN_SIZE_VALUE(value) do {       \
        Py_ssize_t _ret = (value);          \
        if (_ret == -1) {                   \
            assert(PyErr_Occurred());       \
            return PyValue_NULL;            \
        }                                   \
        assert(!PyErr_Occurred());          \
        return PyValue_FromResult(PyLong_FromSsize_t(_ret)); \
    } while (0)

/* Marker to check that pointer value was set. */
static const char uninitialized[] = "uninitialized";
#define UNINITIALIZED_PTR ((void *)uninitialized)
/* Marker to check that Py_ssize_t value was set. */
#define UNINITIALIZED_SIZE ((Py_ssize_t)236892191)
/* Marker to check that integer value was set. */
#define UNINITIALIZED_INT (63256717)
/*
 * Marker to indicate that a NULL parameter would not be allowed
 * at runtime but that the test interface will check that it is
 * indeed the case.
 *
 * Use this macro only if passing NULL to the C API would raise
 * a catchable exception (and not a fatal exception that would
 * crash the interpreter).
 */
 #define NULL_WOULD_RAISE(NAME)    NAME

/* The items of a tuple as a new array of objects, for the C APIs that take
 * one: a tuple holds values, and an inline value is boxed.  Free the array
 * with PyMem_Free().  NULL with an exception set on failure. */
static inline PyObject **
tuple_objects(PyObject *tuple)
{
    Py_ssize_t n = PyTuple_GET_SIZE(tuple);
    PyObject **items = PyMem_New(PyObject *, n > 0 ? n : 1);
    if (items == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        items[i] = PyTuple_GET_ITEM(tuple, i);
    }
    return items;
}
