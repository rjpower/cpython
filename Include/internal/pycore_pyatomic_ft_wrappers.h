// This header file provides wrappers around the atomic operations found in
// `pyatomic.h` that are only atomic in free-threaded builds.
//
// These are intended to be used in places where atomics are required in
// free-threaded builds, but not in the default build, and we don't want to
// introduce the potential performance overhead of an atomic operation in the
// default build.
//
// All usages of these macros should be replaced with unconditionally atomic or
// non-atomic versions, and this file should be removed, once the dust settles
// on free threading.
#ifndef Py_ATOMIC_FT_WRAPPERS_H
#define Py_ATOMIC_FT_WRAPPERS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#error "this header requires Py_BUILD_CORE define"
#endif

#define FT_ATOMIC_LOAD_PTR(value) value
#define FT_ATOMIC_STORE_PTR(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_SSIZE(value) value
#define FT_ATOMIC_LOAD_SSIZE_ACQUIRE(value) value
#define FT_ATOMIC_LOAD_SSIZE_RELAXED(value) value
#define FT_ATOMIC_LOAD_PTR_ACQUIRE(value) value
#define FT_ATOMIC_LOAD_PTR_CONSUME(value) value
#define FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(value) value
#define FT_ATOMIC_LOAD_PTR_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT8(value) value
#define FT_ATOMIC_STORE_UINT8(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_INT8_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT8_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT16_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT32_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT64_ACQUIRE(value) value
#define FT_ATOMIC_LOAD_UINT64_RELAXED(value) value
#define FT_ATOMIC_LOAD_ULONG_RELAXED(value) value
#define FT_ATOMIC_STORE_PTR_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_PTR_RELEASE(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINTPTR_RELEASE(value, new_value) value = new_value
#define FT_ATOMIC_STORE_INT8_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_INT8_RELEASE(value, new_value) value = new_value
#define FT_ATOMIC_STORE_SSIZE_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_SSIZE_RELEASE(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINT8_RELEASE(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINT8_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINT16_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINT32_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_AND_UINT64(value, new_value) (void)(value &= new_value)
#define FT_ATOMIC_OR_UINT64(value, new_value) (void)(value |= new_value)
#define FT_ATOMIC_ADD_UINT64(value, new_value) (void)(value += new_value)
#define FT_ATOMIC_LOAD_CHAR_RELAXED(value) value
#define FT_ATOMIC_STORE_CHAR_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_UCHAR_RELAXED(value) value
#define FT_ATOMIC_STORE_UCHAR_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_SHORT_RELAXED(value) value
#define FT_ATOMIC_STORE_SHORT_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_USHORT_RELAXED(value) value
#define FT_ATOMIC_STORE_USHORT_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_INT(value) value
#define FT_ATOMIC_STORE_INT(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_INT_RELAXED(value) value
#define FT_ATOMIC_STORE_INT_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_UINT(value) value
#define FT_ATOMIC_LOAD_UINT_RELAXED(value) value
#define FT_ATOMIC_STORE_UINT_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_LONG_RELAXED(value) value
#define FT_ATOMIC_STORE_LONG_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_ULONG_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_SSIZE_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_FLOAT_RELAXED(value) value
#define FT_ATOMIC_STORE_FLOAT_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_DOUBLE_RELAXED(value) value
#define FT_ATOMIC_STORE_DOUBLE_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_LLONG_RELAXED(value) value
#define FT_ATOMIC_STORE_LLONG_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_ULLONG_RELAXED(value) value
#define FT_ATOMIC_STORE_ULLONG_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_ADD_SSIZE(value, new_value) (void)(value += new_value)
#define FT_MUTEX_LOCK(lock) do {} while (0)
#define FT_MUTEX_LOCK_FLAGS(lock, flags) do {} while (0)
#define FT_MUTEX_UNLOCK(lock) do {} while (0)


#ifdef __cplusplus
}
#endif
#endif /* !Py_ATOMIC_FT_WRAPPERS_H */
