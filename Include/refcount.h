#ifndef _Py_REFCOUNT_H
#define _Py_REFCOUNT_H
#ifdef __cplusplus
extern "C" {
#endif


#if !defined(_Py_OPAQUE_PYOBJECT)
/*
Nothing is reference counted: the tracing collector frees unreachable objects,
and the header has no count.  An immortal object, one that the collector never
frees, has _Py_IMMORTAL_FLAGS in ob_flags; a statically allocated one also has
_Py_STATICALLY_ALLOCATED_FLAG.  The header word of a static object of type
type is _Py_STATIC_HEADER_INIT(type); the flags are added, not or-ed, so that
the word is an address constant.
*/
#define _Py_STATIC_FLAG_BITS ((uintptr_t)(_Py_STATICALLY_ALLOCATED_FLAG | _Py_IMMORTAL_FLAGS))
#define _Py_STATIC_HEADER_INIT(type) \
    ((uintptr_t)(type) + (_Py_STATIC_FLAG_BITS << 48))
#endif  // _Py_OPAQUE_PYOBJECT

// Py_REFCNT() implementation for the stable ABI
PyAPI_FUNC(Py_ssize_t) Py_REFCNT(PyObject *ob);

#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x030e0000
    // Stable ABI implements Py_REFCNT() as a function call
    // on limited C API version 3.14 and newer, and on abi3t.
#elif defined(_Py_OPAQUE_PYOBJECT)
    // Py_REFCNT() is also a function call in abi3t.
#else
    static inline Py_ssize_t _Py_REFCNT(PyObject *ob) {
        // Nothing is counted.  The constant is large so that code testing
        // Py_REFCNT(x) == 1 before mutating x in place takes its safe path.
        (void)ob;
        return _Py_GC_REFCNT;
    }
    #if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
    #  define Py_REFCNT(ob) _Py_REFCNT(_PyObject_CAST(ob))
    #else
    #  define Py_REFCNT(ob) _Py_REFCNT(ob)
    #endif
#endif

#ifndef _Py_OPAQUE_PYOBJECT
static inline Py_ALWAYS_INLINE int _Py_IsImmortal(PyObject *op)
{
    return (op->ob_flags & _Py_IMMORTAL_FLAGS) != 0;
}
#define _Py_IsImmortal(op) _Py_IsImmortal(_PyObject_CAST(op))


static inline Py_ALWAYS_INLINE int _Py_IsStaticImmortal(PyObject *op)
{
    return (op->ob_flags & _Py_STATICALLY_ALLOCATED_FLAG) != 0;
}
#define _Py_IsStaticImmortal(op) _Py_IsStaticImmortal(_PyObject_CAST(op))
#endif // !defined(_Py_OPAQUE_PYOBJECT)

// Py_SET_REFCNT() implementation for stable ABI
PyAPI_FUNC(void) _Py_SetRefcnt(PyObject *ob, Py_ssize_t refcnt);

static inline void Py_SET_REFCNT(PyObject *ob, Py_ssize_t refcnt) {
    assert(refcnt >= 0);
    (void)ob;
    (void)refcnt;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_SET_REFCNT(ob, refcnt) Py_SET_REFCNT(_PyObject_CAST(ob), (refcnt))
#endif


/*
Py_INCREF(op) and Py_DECREF(op), and the functions and macros built on them,
remain for source compatibility and do nothing: the tracing collector owns
memory, and an object lives while it is reachable.
*/


PyAPI_FUNC(void) _Py_Dealloc(PyObject *);


/*
These are provided as conveniences to Python runtime embedders, so that
they can have object code that is not dependent on Python compilation flags.
*/
PyAPI_FUNC(void) Py_IncRef(PyObject *);
PyAPI_FUNC(void) Py_DecRef(PyObject *);

// Similar to Py_IncRef() and Py_DecRef() but the argument must be non-NULL.
// Private functions used by Py_INCREF() and Py_DECREF().
PyAPI_FUNC(void) _Py_IncRef(PyObject *);
PyAPI_FUNC(void) _Py_DecRef(PyObject *);

static inline Py_ALWAYS_INLINE void Py_INCREF(PyObject *op)
{
    // The tracing collector owns memory; there is nothing to count.
    (void)op;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_INCREF(op) Py_INCREF(_PyObject_CAST(op))
#endif

#if !defined(Py_LIMITED_API)
#endif  // Py_LIMITED_API

// The tracing collector frees unreachable objects; there is nothing to count.
static inline Py_ALWAYS_INLINE void Py_DECREF(PyObject *op)
{
    (void)op;
}
#define Py_DECREF(op) Py_DECREF(_PyObject_CAST(op))



/* Py_CLEAR(op) sets op to NULL, and Py_SETREF(dst, src) and
 * Py_XSETREF(dst, src) set dst to src.  They once released the old value
 * after the store, so that code it ran could not see it; nothing is
 * released now, so they are plain assignments.  Each evaluates its
 * arguments once.
 */
#define Py_CLEAR(op) \
    do { \
        (op) = _Py_NULL; \
    } while (0)

#define Py_SETREF(dst, src) \
    do { \
        (dst) = (src); \
    } while (0)

#define Py_XSETREF(dst, src) Py_SETREF(dst, src)


/* Function to use in case the object pointer can be NULL: */
static inline void Py_XINCREF(PyObject *op)
{
    if (op != _Py_NULL) {
        Py_INCREF(op);
    }
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_XINCREF(op) Py_XINCREF(_PyObject_CAST(op))
#endif

static inline void Py_XDECREF(PyObject *op)
{
    if (op != _Py_NULL) {
        Py_DECREF(op);
    }
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_XDECREF(op) Py_XDECREF(_PyObject_CAST(op))
#endif

// Create a new strong reference to an object:
// increment the reference count of the object and return the object.
PyAPI_FUNC(PyObject*) Py_NewRef(PyObject *obj);

// Similar to Py_NewRef(), but the object can be NULL.
PyAPI_FUNC(PyObject*) Py_XNewRef(PyObject *obj);

static inline PyObject* _Py_NewRef(PyObject *obj)
{
    Py_INCREF(obj);
    return obj;
}

static inline PyObject* _Py_XNewRef(PyObject *obj)
{
    Py_XINCREF(obj);
    return obj;
}

// Py_NewRef() and Py_XNewRef() are exported as functions for the stable ABI.
// Names overridden with macros by static inline functions for best
// performances.
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_NewRef(obj) _Py_NewRef(_PyObject_CAST(obj))
#  define Py_XNewRef(obj) _Py_XNewRef(_PyObject_CAST(obj))
#else
#  define Py_NewRef(obj) _Py_NewRef(obj)
#  define Py_XNewRef(obj) _Py_XNewRef(obj)
#endif


#ifdef __cplusplus
}
#endif
#endif   // !_Py_REFCOUNT_H
