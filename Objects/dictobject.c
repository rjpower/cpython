/* Dictionary object implementation using a hash table */

/* The distribution includes a separate file, Objects/dictnotes.txt,
   describing explorations into dictionary design and optimization.
   It covers typical dictionary use patterns, the parameters for
   tuning dictionaries, and several ideas for possible optimizations.
*/

/* PyDictKeysObject

This implements the dictionary's hashtable.

As of Python 3.6, this is compact and ordered. Basic idea is described here:
* https://mail.python.org/pipermail/python-dev/2012-December/123028.html
* https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html

layout:

+---------------------+
| dk_refcnt           |
| dk_log2_size        |
| dk_log2_index_bytes |
| dk_kind             |
| dk_version          |
| dk_usable           |
| dk_nentries         |
+---------------------+
| dk_indices[]        |
|                     |
+---------------------+
| dk_entries[]        |
|                     |
+---------------------+

dk_indices is actual hashtable.  It holds index in entries, or DKIX_EMPTY(-1)
or DKIX_DUMMY(-2).
Size of indices is dk_size.  Type of each index in indices varies with dk_size:

* int8  for          dk_size <= 128
* int16 for 256   <= dk_size <= 2**15
* int32 for 2**16 <= dk_size <= 2**31
* int64 for 2**32 <= dk_size

dk_entries is array of PyDictKeyEntry when dk_kind == DICT_KEYS_GENERAL or
PyDictUnicodeEntry otherwise. Its length is USABLE_FRACTION(dk_size).

NOTE: Since negative value is used for DKIX_EMPTY and DKIX_DUMMY, type of
dk_indices entry is signed integer and int16 is used for table which
dk_size == 256.
*/


/*
The DictObject can be in one of two forms.

Either:
  A combined table:
    ma_values == NULL, dk_refcnt == 1.
    Values are stored in the me_value field of the PyDictKeyEntry.
Or:
  A split table:
    ma_values != NULL, dk_refcnt >= 1
    Values are stored in the ma_values array.
    Only string (unicode) keys are allowed.

There are four kinds of slots in the table (slot is index, and
DK_ENTRIES(keys)[index] if index >= 0):

1. Unused.  index == DKIX_EMPTY
   Does not hold an active (key, value) pair now and never did.  Unused can
   transition to Active upon key insertion.  This is each slot's initial state.

2. Active.  index >= 0, me_key != NULL and me_value != NULL
   Holds an active (key, value) pair.  Active can transition to Dummy or
   Pending upon key deletion (for combined and split tables respectively).
   This is the only case in which me_value != NULL.

3. Dummy.  index == DKIX_DUMMY  (combined only)
   Previously held an active (key, value) pair, but that was deleted and an
   active pair has not yet overwritten the slot.  Dummy can transition to
   Active upon key insertion.  Dummy slots cannot be made Unused again
   else the probe sequence in case of collision would have no way to know
   they were once active.
   In free-threaded builds dummy slots are not re-used to allow lock-free
   lookups to proceed safely.

4. Pending. index >= 0, key != NULL, and value == NULL  (split only)
   Not yet inserted in split-table.
*/

/*
Preserving insertion order

It's simple for combined table.  Since dk_entries is mostly append only, we can
get insertion order by just iterating dk_entries.

One exception is .popitem().  It removes last item in dk_entries and decrement
dk_nentries to achieve amortized O(1).  Since there are DKIX_DUMMY remains in
dk_indices, we can't increment dk_usable even though dk_nentries is
decremented.

To preserve the order in a split table, a bit vector is used to record the
insertion order. When a key is inserted the bit vector is shifted up by 4 bits
and the index of the key is stored in the low 4 bits.
As a consequence of this, split keys have a maximum size of 16.
*/

/* PyDict_MINSIZE is the starting size for any new dict.
 * 8 allows dicts with no more than 5 active entries; experiments suggested
 * this suffices for the majority of dicts (consisting mostly of usually-small
 * dicts created to pass keyword arguments).
 * Making this 8, rather than 4 reduces the number of resizes for most
 * dictionaries, without any significant extra memory use.
 */
#define PyDict_LOG_MINSIZE 3
#define PyDict_MINSIZE 8

#include "Python.h"
#include "pycore_bitutils.h"      // _Py_bit_length
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_code.h"          // stats
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION, Py_END_CRITICAL_SECTION
#include "pycore_dict.h"          // export _PyDict_SizeOf()
#include "pycore_freelist.h"      // _PyFreeListState_GET()
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_long_value.h"    // _PyLong_ValueHash()
#include "pycore_list.h"          // PyList_GET_VALUE()
#include "pycore_object.h"        // _PyObject_GC_TRACK(), _PyDebugAllocatorStats()
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_SSIZE_RELAXED
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include "pycore_value_ops.h"
#include "pycore_tuple.h"         // _PyTuple_Recycle()
#include "pycore_unicodeobject.h" // _PyUnicode_InternImmortal()
#include "pycore_value_api.h"     // _PyValue_AsObjectAt()
#include "pycore_value_call.h"    // _PyValue_CallOneArg()
#include "pycore_value_slots.h"   // _PyValue_THUNK_EXIT()

#include "stringlib/eq.h"                // unicode_eq()
#include <stdbool.h>

// Forward declarations
static PyObject* frozendict_new(PyTypeObject *type, PyObject *args,
                                PyObject *kwds);
static PyObject* frozendict_new_untracked(PyTypeObject *type);
static PyObject* dict_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
static PyObject* dict_new_untracked(PyTypeObject *type);
static int dict_merge(PyObject *a, PyObject *b, int override, PyObject **dupkey);
static int dict_contains(PyObject *op, PyObject *key);
static int dict_merge_from_seq2(PyObject *d, PyObject *seq2, int override);


/*[clinic input]
value_emission
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=eb7b17088bfd7215]*/

/*[clinic input]
class dict "PyDictObject *" "&PyDict_Type"
class frozendict "PyFrozenDictObject *" "&PyFrozenDict_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=5dfa93bac68e7c54]*/


/*
To ensure the lookup algorithm terminates, there must be at least one Unused
slot (NULL key) in the table.
To avoid slowing down lookups on a near-full table, we resize the table when
it's USABLE_FRACTION (currently two-thirds) full.
*/


#define ASSERT_DICT_LOCKED(op)
#define ASSERT_WORLD_STOPPED_OR_DICT_LOCKED(op)
#define ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(op)
#define LOCK_KEYS(keys)
#define UNLOCK_KEYS(keys)
#define ASSERT_KEYS_LOCKED(keys)
#define LOAD_SHARED_KEY(key) key
#define STORE_SHARED_KEY(key, value) key = value
#define INCREF_KEYS(dk)  dk->dk_refcnt++
#define DECREF_KEYS(dk)  dk->dk_refcnt--
#define LOAD_KEYS_NENTRIES(keys) keys->dk_nentries
#define INCREF_KEYS_FT(dk)
#define DECREF_KEYS_FT(dk, shared)
#define LOCK_KEYS_IF_SPLIT(keys, kind)
#define UNLOCK_KEYS_IF_SPLIT(keys, kind)
#define IS_DICT_SHARED(mp) (false)
#define SET_DICT_SHARED(mp)
#define LOAD_INDEX(keys, size, idx) ((const int##size##_t*)(keys->dk_indices))[idx]
#define STORE_INDEX(keys, size, idx, value) ((int##size##_t*)(keys->dk_indices))[idx] = (int##size##_t)value

static inline void split_keys_entry_added(PyDictKeysObject *keys)
{
    keys->dk_usable--;
    keys->dk_nentries++;
}

static inline void
set_keys(PyDictObject *mp, PyDictKeysObject *keys)
{
    mp->ma_keys = keys;
}

static inline void
set_values(PyDictObject *mp, PyDictValues *values)
{
    mp->ma_values = values;
}

static inline Py_ssize_t
load_keys_nentries(PyDictObject *mp)
{
    return mp->ma_keys->dk_nentries;
}



#ifndef NDEBUG
// Check if it's possible to modify a dictionary.
// Usage: assert(can_modify_dict(mp)).
static inline int
can_modify_dict(PyDictObject *mp)
{
    if (PyFrozenDict_Check(mp)) {
        // gh-151722: A frozendict must not be tracked by the GC
        // when it's being modified.
        assert(!_PyObject_GC_IS_TRACKED(mp));

        // No locking required to modify a newly created frozendict
        // since it's only accessible from the current thread.
        assert(PyUnstable_Object_IsUniquelyReferenced(_PyObject_CAST(mp)));
    }
    else {
        // Locking is only required if the dictionary is not
        // uniquely referenced.
        ASSERT_DICT_LOCKED(mp);
    }
    return 1;
}
#endif

#define _PyAnyDict_CAST(op) \
    (assert(PyAnyDict_Check(op)), _Py_CAST(PyDictObject*, op))

#define GET_USED(ep) FT_ATOMIC_LOAD_SSIZE_RELAXED((ep)->ma_used)

#define STORE_KEY(ep, key) FT_ATOMIC_STORE_PTR_RELEASE((ep)->me_key, key)
#define STORE_VALUE(ep, value) FT_ATOMIC_STORE_PTR_RELEASE((ep)->me_value, value)
#define STORE_SPLIT_VALUE(mp, idx, value) FT_ATOMIC_STORE_PTR_RELEASE(mp->ma_values->values[idx], value)
#define STORE_HASH(ep, hash) FT_ATOMIC_STORE_SSIZE_RELAXED((ep)->me_hash, hash)
#define STORE_KEYS_USABLE(keys, usable) FT_ATOMIC_STORE_SSIZE_RELAXED(keys->dk_usable, usable)
#define STORE_KEYS_NENTRIES(keys, nentries) FT_ATOMIC_STORE_SSIZE_RELAXED(keys->dk_nentries, nentries)
#define STORE_USED(mp, used) FT_ATOMIC_STORE_SSIZE_RELAXED(mp->ma_used, used)

#define PERTURB_SHIFT 5


#define value_is_exact_unicode(V) (value_is_exact_unicode)(PyValue_Borrow(V))
static inline int
(value_is_exact_unicode)(PyValueBorrow value)
{
    return PyValue_IsObject(value) &&
           PyUnicode_CheckExact(PyValue_AsKnownObjectBorrow(value));
}

/* A proven Unicode key is already canonical. Transfer its existing owner
   when a combined table changes from Unicode metadata to generic values. */
static inline PyValue
dict_take_unicode_key(PyObject *key)
{
    assert(PyUnicode_CheckExact(key));
    return PyValue_PrepareOwned(key);
}

/*
Major subtleties ahead:  Most hash schemes depend on having a "good" hash
function, in the sense of simulating randomness.  Python doesn't:  its most
important hash functions (for ints) are very regular in common
cases:

  >>>[hash(i) for i in range(4)]
  [0, 1, 2, 3]

This isn't necessarily bad!  To the contrary, in a table of size 2**i, taking
the low-order i bits as the initial table index is extremely fast, and there
are no collisions at all for dicts indexed by a contiguous range of ints. So
this gives better-than-random behavior in common cases, and that's very
desirable.

OTOH, when collisions occur, the tendency to fill contiguous slices of the
hash table makes a good collision resolution strategy crucial.  Taking only
the last i bits of the hash code is also vulnerable:  for example, consider
the list [i << 16 for i in range(20000)] as a set of keys.  Since ints are
their own hash codes, and this fits in a dict of size 2**15, the last 15 bits
 of every hash code are all 0:  they *all* map to the same table index.

But catering to unusual cases should not slow the usual ones, so we just take
the last i bits anyway.  It's up to collision resolution to do the rest.  If
we *usually* find the key we're looking for on the first try (and, it turns
out, we usually do -- the table load factor is kept under 2/3, so the odds
are solidly in our favor), then it makes best sense to keep the initial index
computation dirt cheap.

The first half of collision resolution is to visit table indices via this
recurrence:

    j = ((5*j) + 1) mod 2**i

For any initial j in range(2**i), repeating that 2**i times generates each
int in range(2**i) exactly once (see any text on random-number generation for
proof).  By itself, this doesn't help much:  like linear probing (setting
j += 1, or j -= 1, on each loop trip), it scans the table entries in a fixed
order.  This would be bad, except that's not the only thing we do, and it's
actually *good* in the common cases where hash keys are consecutive.  In an
example that's really too small to make this entirely clear, for a table of
size 2**3 the order of indices is:

    0 -> 1 -> 6 -> 7 -> 4 -> 5 -> 2 -> 3 -> 0 [and here it's repeating]

If two things come in at index 5, the first place we look after is index 2,
not 6, so if another comes in at index 6 the collision at 5 didn't hurt it.
Linear probing is deadly in this case because there the fixed probe order
is the *same* as the order consecutive keys are likely to arrive.  But it's
extremely unlikely hash codes will follow a 5*j+1 recurrence by accident,
and certain that consecutive hash codes do not.

The other half of the strategy is to get the other bits of the hash code
into play.  This is done by initializing a (unsigned) vrbl "perturb" to the
full hash code, and changing the recurrence to:

    perturb >>= PERTURB_SHIFT;
    j = (5*j) + 1 + perturb;
    use j % 2**i as the next table index;

Now the probe sequence depends (eventually) on every bit in the hash code,
and the pseudo-scrambling property of recurring on 5*j+1 is more valuable,
because it quickly magnifies small differences in the bits that didn't affect
the initial index.  Note that because perturb is unsigned, if the recurrence
is executed often enough perturb eventually becomes and remains 0.  At that
point (very rarely reached) the recurrence is on (just) 5*j+1 again, and
that's certain to find an empty slot eventually (since it generates every int
in range(2**i), and we make sure there's always at least one empty slot).

Selecting a good value for PERTURB_SHIFT is a balancing act.  You want it
small so that the high bits of the hash code continue to affect the probe
sequence across iterations; but you want it large so that in really bad cases
the high-order hash bits have an effect on early iterations.  5 was "the
best" in minimizing total collisions across experiments Tim Peters ran (on
both normal and pathological cases), but 4 and 6 weren't significantly worse.

Historical: Reimer Behrends contributed the idea of using a polynomial-based
approach, using repeated multiplication by x in GF(2**n) where an irreducible
polynomial for each table size was chosen such that x was a primitive root.
Christian Tismer later extended that to use division by x instead, as an
efficient way to get the high bits of the hash code into play.  This scheme
also gave excellent collision statistics, but was more expensive:  two
if-tests were required inside the loop; computing "the next" index took about
the same number of operations but without as much potential parallelism
(e.g., computing 5*j can go on at the same time as computing 1+perturb in the
above, and then shifting perturb can be done while the table index is being
masked); and the PyDictObject struct required a member to hold the table's
polynomial.  In Tim's experiments the current scheme ran faster, produced
equally good collision statistics, needed less code & used less memory.

*/

static int dictresize(PyDictObject *mp, uint8_t log_newsize, int unicode);

static PyObject* dict_iter(PyObject *dict);

static int
setitem_lock_held(PyDictObject *mp, PyObject *key, PyObject *value);

static int (dict_setdefault_value_lock_held)(PyDictObject *, PyValueBorrow,
                                           PyValueBorrow, Py_hash_t,
                                           PyValue *);
#define dict_setdefault_value_lock_held(V0, V1, V2, V3, V4) \
    (dict_setdefault_value_lock_held)(V0, PyValue_Borrow(V1), \
                                      PyValue_Borrow(V2), V3, V4)

#ifndef NDEBUG
static int _PyObject_InlineValuesConsistencyCheck(PyObject *obj);
#endif

#include "clinic/dictobject.c.h"


static inline Py_hash_t
unicode_get_hash(PyObject *o)
{
    return PyUnstable_Unicode_GET_CACHED_HASH(o);
}

/* Print summary info about the state of the optimized allocator */
void
_PyDict_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out, "free PyDictObject",
                           _Py_FREELIST_SIZE(dicts),
                           _PyType_PreHeaderSize(&PyDict_Type) + sizeof(PyDictObject));
    _PyDebugAllocatorStats(out, "free PyDictKeysObject",
                           _Py_FREELIST_SIZE(dictkeys),
                           sizeof(PyDictKeysObject));
}

#define DK_MASK(dk) (DK_SIZE(dk)-1)

#define _Py_DICT_IMMORTAL_INITIAL_REFCNT PY_SSIZE_T_MIN

static void free_keys_object(PyDictKeysObject *keys, bool use_qsbr);

/* PyDictKeysObject has refcounts like PyObject does, so we have the
   following two functions to mirror what Py_INCREF() and Py_DECREF() do.
   (Keep in mind that PyDictKeysObject isn't actually a PyObject.)
   Likewise a PyDictKeysObject can be immortal (e.g. Py_EMPTY_KEYS),
   so we apply a naive version of what Py_INCREF() and Py_DECREF() do
   for immortal objects. */

static inline void
dictkeys_incref(PyDictKeysObject *dk)
{
    if (FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) < 0) {
        assert(FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) == _Py_DICT_IMMORTAL_INITIAL_REFCNT);
        return;
    }
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyThreadState_GET());
#endif
    INCREF_KEYS(dk);
}

static inline void
dictkeys_decref(PyDictKeysObject *dk, bool use_qsbr)
{
    if (FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) < 0) {
        assert(FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) == _Py_DICT_IMMORTAL_INITIAL_REFCNT);
        return;
    }
    assert(FT_ATOMIC_LOAD_SSIZE(dk->dk_refcnt) > 0);
#ifdef Py_REF_DEBUG
    _Py_DecRefTotal(_PyThreadState_GET());
#endif
    if (DECREF_KEYS(dk) == 1) {
        if (DK_IS_UNICODE(dk)) {
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dk);
            Py_ssize_t i, n;
            for (i = 0, n = dk->dk_nentries; i < n; i++) {
                PyObject *key = entries[i].me_key;
                // a move: cleared below
                PyValue value = entries[i].me_value;
                entries[i].me_key = NULL;
                entries[i].me_value = PyValue_NULL;
                Py_XDECREF(key);
                PyValue_XCLOSE(value);
            }
        }
        else {
            PyDictKeyEntry *entries = DK_ENTRIES(dk);
            Py_ssize_t i, n;
            for (i = 0, n = dk->dk_nentries; i < n; i++) {
                // a move: cleared below
                PyValue key = entries[i].me_key;
                // a move: cleared below
                PyValue value = entries[i].me_value;
                entries[i].me_key = PyValue_NULL;
                entries[i].me_value = PyValue_NULL;
                PyValue_XCLOSE(key);
                PyValue_XCLOSE(value);
            }
        }
        free_keys_object(dk, use_qsbr);
    }
}

/* lookup indices.  returns DKIX_EMPTY, DKIX_DUMMY, or ix >=0 */
static inline Py_ssize_t
dictkeys_get_index(const PyDictKeysObject *keys, Py_ssize_t i)
{
    int log2size = DK_LOG_SIZE(keys);
    Py_ssize_t ix;

    if (log2size < 8) {
        ix = LOAD_INDEX(keys, 8, i);
    }
    else if (log2size < 16) {
        ix = LOAD_INDEX(keys, 16, i);
    }
#if SIZEOF_VOID_P > 4
    else if (log2size >= 32) {
        ix = LOAD_INDEX(keys, 64, i);
    }
#endif
    else {
        ix = LOAD_INDEX(keys, 32, i);
    }
    assert(ix >= DKIX_DUMMY);
    return ix;
}

/* Return an owned value only when the first probe proves an identity hit.
 * Declining leaves the inputs, table, and exception state untouched. */
PyValue
_PyDict_TryGetFirstProbeValue(PyValueBorrow container, PyValueBorrow key)
{
#ifdef Py_GIL_DISABLED
    return PyValue_NULL;
#else
    if (!PyValue_IsObject(container) || !PyValue_IsInlineInt(key)) {
        return PyValue_NULL;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(container);
    if (!PyDict_CheckExact(object)) {
        return PyValue_NULL;
    }
    PyDictObject *dict = (PyDictObject *)object;
    PyDictKeysObject *keys = dict->ma_keys;
    if (keys->dk_kind != DICT_KEYS_GENERAL || dict->ma_values != NULL) {
        return PyValue_NULL;
    }
    Py_hash_t hash = _PyLong_ValueHash(key);
    Py_ssize_t index = dictkeys_get_index(keys, (size_t)hash & DK_MASK(keys));
    if (index < 0) {
        return PyValue_NULL;
    }
    assert(index < keys->dk_nentries);
    PyDictKeyEntry *entry = &DK_ENTRIES(keys)[index];
    if (!PyValue_Is(entry->me_key, key) || PyValue_IsNull(entry->me_value)) {
        return PyValue_NULL;
    }
    assert(_PyValue_IsHome(entry->me_value));
    return PyValue_XDUP(entry->me_value);
#endif
}

/* write to indices. */
static inline void
dictkeys_set_index(PyDictKeysObject *keys, Py_ssize_t i, Py_ssize_t ix)
{
    int log2size = DK_LOG_SIZE(keys);

    assert(ix >= DKIX_DUMMY);
    assert(keys->dk_version == 0);

    if (log2size < 8) {
        assert(ix <= 0x7f);
        STORE_INDEX(keys, 8, i, ix);
    }
    else if (log2size < 16) {
        assert(ix <= 0x7fff);
        STORE_INDEX(keys, 16, i, ix);
    }
#if SIZEOF_VOID_P > 4
    else if (log2size >= 32) {
        STORE_INDEX(keys, 64, i, ix);
    }
#endif
    else {
        assert(ix <= 0x7fffffff);
        STORE_INDEX(keys, 32, i, ix);
    }
}


/* USABLE_FRACTION is the maximum dictionary load.
 * Increasing this ratio makes dictionaries more dense resulting in more
 * collisions.  Decreasing it improves sparseness at the expense of spreading
 * indices over more cache lines and at the cost of total memory consumed.
 *
 * USABLE_FRACTION must obey the following:
 *     (0 < USABLE_FRACTION(n) < n) for all n >= 2
 *
 * USABLE_FRACTION should be quick to calculate.
 * Fractions around 1/2 to 2/3 seem to work well in practice.
 */
#define USABLE_FRACTION(n) (((n) << 1)/3)

/* Find the smallest dk_size >= minsize. */
static inline uint8_t
calculate_log2_keysize(Py_ssize_t minsize)
{
#if SIZEOF_LONG == SIZEOF_SIZE_T
    minsize = Py_MAX(minsize, PyDict_MINSIZE);
    return _Py_bit_length(minsize - 1);
#elif defined(_MSC_VER)
    // On 64bit Windows, sizeof(long) == 4. We cannot use _Py_bit_length.
    minsize = Py_MAX(minsize, PyDict_MINSIZE);
    unsigned long msb;
    _BitScanReverse64(&msb, (uint64_t)minsize - 1);
    return (uint8_t)(msb + 1);
#else
    uint8_t log2_size;
    for (log2_size = PyDict_LOG_MINSIZE;
            (((Py_ssize_t)1) << log2_size) < minsize;
            log2_size++)
        ;
    return log2_size;
#endif
}

/* estimate_keysize is reverse function of USABLE_FRACTION.
 *
 * This can be used to reserve enough size to insert n entries without
 * resizing.
 */
static inline uint8_t
estimate_log2_keysize(Py_ssize_t n)
{
    return calculate_log2_keysize((n*3 + 1) / 2);
}


/* GROWTH_RATE. Growth rate upon hitting maximum load.
 * Currently set to used*3.
 * This means that dicts double in size when growing without deletions,
 * but have more head room when the number of deletions is on a par with the
 * number of insertions.  See also bpo-17563 and bpo-33205.
 *
 * GROWTH_RATE was set to used*4 up to version 3.2.
 * GROWTH_RATE was set to used*2 in version 3.3.0
 * GROWTH_RATE was set to used*2 + capacity/2 in 3.4.0-3.6.0.
 */
#define GROWTH_RATE(d) ((d)->ma_used*3)

/* This immutable, empty PyDictKeysObject is used for PyDict_Clear()
 * (which cannot fail and thus can do no allocation).
 *
 * See https://github.com/python/cpython/pull/127568#discussion_r1868070614
 * for the rationale of using dk_log2_index_bytes=3 instead of 0.
 */
static PyDictKeysObject empty_keys_struct = {
        _Py_DICT_IMMORTAL_INITIAL_REFCNT, /* dk_refcnt */
        0, /* dk_log2_size */
        3, /* dk_log2_index_bytes */
        DICT_KEYS_UNICODE, /* dk_kind */
        1, /* dk_version */
        0, /* dk_usable (immutable) */
        0, /* dk_nentries */
        {DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY,
         DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY}, /* dk_indices */
};

#define Py_EMPTY_KEYS &empty_keys_struct

/* Uncomment to check the dict content in _PyDict_CheckConsistency() */
// #define DEBUG_PYDICT

#ifdef DEBUG_PYDICT
#  define ASSERT_CONSISTENT(op) assert(_PyDict_CheckConsistency((PyObject *)(op), 1))
#else
#  define ASSERT_CONSISTENT(op) assert(_PyDict_CheckConsistency((PyObject *)(op), 0))
#endif

static inline int
get_index_from_order(PyDictObject *mp, Py_ssize_t i)
{
    assert(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
    assert(i < mp->ma_values->size);
    uint8_t *array = get_insertion_order_array(mp->ma_values);
    return array[i];
}

#ifdef DEBUG_PYDICT
static void
dump_entries(PyDictKeysObject *dk)
{
    for (Py_ssize_t i = 0; i < dk->dk_nentries; i++) {
        if (DK_IS_UNICODE(dk)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(dk)[i];
            printf("key=%p value=%#llx\n", (void *)ep->me_key,
                   (unsigned long long)_PyValue_Word(ep->me_value));
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(dk)[i];
            printf("key=%#llx hash=%lx value=%#llx\n",
                   (unsigned long long)_PyValue_Word(ep->me_key), ep->me_hash,
                   (unsigned long long)_PyValue_Word(ep->me_value));
        }
    }
}
#endif

int
_PyDict_CheckConsistency(PyObject *op, int check_content)
{
    ASSERT_WORLD_STOPPED_OR_DICT_LOCKED(op);

#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG(op, Py_STRINGIFY(expr)); } } while (0)

    assert(op != NULL);
    CHECK(PyAnyDict_Check(op));
    PyDictObject *mp = (PyDictObject *)op;

    PyDictKeysObject *keys = mp->ma_keys;
    int splitted = _PyDict_HasSplitTable(mp);
    Py_ssize_t usable = USABLE_FRACTION(DK_SIZE(keys));

    // In the free-threaded build, shared keys may be concurrently modified,
    // so use atomic loads.
    Py_ssize_t dk_usable = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(keys->dk_usable);
    Py_ssize_t dk_nentries = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(keys->dk_nentries);

    CHECK(0 <= mp->ma_used && mp->ma_used <= usable);
    CHECK(0 <= dk_usable && dk_usable <= usable);
    CHECK(0 <= dk_nentries && dk_nentries <= usable);
    CHECK(dk_usable + dk_nentries <= usable);

    if (!splitted) {
        /* combined table */
        CHECK(keys->dk_kind != DICT_KEYS_SPLIT);
        CHECK(keys->dk_refcnt == 1 || keys == Py_EMPTY_KEYS);
    }
    else {
        CHECK(keys->dk_kind == DICT_KEYS_SPLIT);
        CHECK(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
        if (mp->ma_values->embedded) {
            CHECK(mp->ma_values->embedded == 1);
            CHECK(mp->ma_values->valid == 1);
        }
    }

    if (check_content) {
        LOCK_KEYS_IF_SPLIT(keys, keys->dk_kind);
        for (Py_ssize_t i=0; i < DK_SIZE(keys); i++) {
            Py_ssize_t ix = dictkeys_get_index(keys, i);
            CHECK(DKIX_DUMMY <= ix && ix <= usable);
        }

        if (keys->dk_kind == DICT_KEYS_GENERAL) {
            PyDictKeyEntry *entries = DK_ENTRIES(keys);
            for (Py_ssize_t i=0; i < usable; i++) {
                PyDictKeyEntry *entry = &entries[i];
                PyValueBorrow key = PyValue_Borrow(entry->me_key);

                if (!PyValue_IsNull(key)) {
                    CHECK(PyValue_IsLanguageValue(key) && _PyValue_IsHome(key));
                    /* test_dict fails if PyObject_Hash() is called again */
                    CHECK(entry->me_hash != -1);
                    CHECK(PyValue_IsLanguageValue(entry->me_value) &&
                          _PyValue_IsHome(entry->me_value));

                    if (value_is_exact_unicode(key)) {
                        Py_hash_t hash = unicode_get_hash(PyValue_AsKnownObjectBorrow(key));
                        CHECK(entry->me_hash == hash);
                    }
                }
            }
        }
        else {
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            for (Py_ssize_t i=0; i < usable; i++) {
                PyDictUnicodeEntry *entry = &entries[i];
                PyObject *key = entry->me_key;

                if (key != NULL) {
                    CHECK(PyUnicode_CheckExact(key));
                    Py_hash_t hash = unicode_get_hash(key);
                    CHECK(hash != -1);
                    if (!splitted) {
                        CHECK(PyValue_IsLanguageValue(entry->me_value) &&
                              _PyValue_IsHome(entry->me_value));
                    }
                }

                if (splitted) {
                    CHECK(PyValue_IsNull(entry->me_value));
                }
            }
        }

        if (splitted) {
            CHECK(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
            /* splitted table */
            int duplicate_check = 0;
            for (Py_ssize_t i=0; i < mp->ma_used; i++) {
                int index = get_index_from_order(mp, i);
                CHECK((duplicate_check & (1<<index)) == 0);
                duplicate_check |= (1<<index);
                CHECK(PyValue_IsLanguageValue(mp->ma_values->values[index]) &&
                      _PyValue_IsHome(mp->ma_values->values[index]));
            }
        }
        UNLOCK_KEYS_IF_SPLIT(keys, keys->dk_kind);
    }
    return 1;

#undef CHECK
}


static inline int
get_log2_bytes(uint8_t log2_size)
{
    int log2_bytes;
    assert(log2_size >= PyDict_LOG_MINSIZE);

    if (log2_size < 8) {
        log2_bytes = log2_size;
    }
    else if (log2_size < 16) {
        log2_bytes = log2_size + 1;
    }
#if SIZEOF_VOID_P > 4
    else if (log2_size >= 32) {
        log2_bytes = log2_size + 3;
    }
#endif
    else {
        log2_bytes = log2_size + 2;
    }

    return log2_bytes;
}

static inline void
init_keys_object(PyDictKeysObject* dk, uint8_t log2_size, int log2_bytes, int kind,
                 Py_ssize_t usable, Py_ssize_t entry_size)
{
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyThreadState_GET());
#endif
    dk->dk_refcnt = 1;
    dk->dk_log2_size = log2_size;
    dk->dk_log2_index_bytes = log2_bytes;
    dk->dk_kind = kind;
    dk->dk_nentries = 0;
    dk->dk_usable = usable;
    dk->dk_version = 0;
    memset(&dk->dk_indices[0], 0xff, ((size_t)1 << log2_bytes));
    if (kind == DICT_KEYS_GENERAL) {
        PyDictKeyEntry *entries = DK_ENTRIES(dk);
        for (Py_ssize_t i = 0; i < usable; i++) {
            entries[i].me_hash = 0;
            entries[i].me_key = PyValue_NULL;
            entries[i].me_value = PyValue_NULL;
        }
    }
    else {
        PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dk);
        for (Py_ssize_t i = 0; i < usable; i++) {
            entries[i].me_key = NULL;
            entries[i].me_value = PyValue_NULL;
        }
    }
}

static PyDictKeysObject*
new_keys_object(uint8_t log2_size, bool unicode)
{
    Py_ssize_t usable = USABLE_FRACTION((size_t)1<<log2_size);
    size_t entry_size = unicode ? sizeof(PyDictUnicodeEntry) : sizeof(PyDictKeyEntry);

    int log2_bytes = get_log2_bytes(log2_size);

    PyDictKeysObject *dk = NULL;
    if (log2_size == PyDict_LOG_MINSIZE && unicode) {
        dk = _Py_FREELIST_POP_MEM(dictkeys);
    }
    if (dk == NULL) {
        dk = PyMem_Malloc(sizeof(PyDictKeysObject)
                          + ((size_t)1 << log2_bytes)
                          + entry_size * usable);
        if (dk == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }
    init_keys_object(dk, log2_size, log2_bytes,
                     unicode ? DICT_KEYS_UNICODE : DICT_KEYS_GENERAL,
                     usable, entry_size);
    return dk;
}

static void
free_keys_object(PyDictKeysObject *keys, bool use_qsbr)
{
    void *ptr = keys;
    if (keys->dk_kind == DICT_KEYS_SPLIT) {
        ptr = _PyDictKeys_AsSharedKeys(keys);
    }
    if (DK_LOG_SIZE(keys) == PyDict_LOG_MINSIZE && keys->dk_kind == DICT_KEYS_UNICODE) {
        _Py_FREELIST_FREE(dictkeys, keys, PyMem_Free);
    }
    else {
        PyMem_Free(ptr);
    }
}

static size_t
values_size_from_count(size_t count)
{
    assert(count >= 1);
    size_t suffix_size = _Py_SIZE_ROUND_UP(count, sizeof(PyObject *));
    assert(suffix_size < 128);
    assert(suffix_size % sizeof(PyObject *) == 0);
    return offsetof(PyDictValues, values) + count * sizeof(PyValue) + suffix_size;
}

#define CACHED_KEYS(tp) (((PyHeapTypeObject*)tp)->ht_cached_keys)

static inline PyDictValues*
new_values(size_t size)
{
    size_t n = values_size_from_count(size);
    PyDictValues *res = (PyDictValues *)PyMem_Malloc(n);
    if (res == NULL) {
        return NULL;
    }
    res->embedded = 0;
    res->size = 0;
    _PyDictValues_SetShape(res, _Py_SHAPE_NONE);
    assert(size < 256);
    res->capacity = (uint8_t)size;
    res->valid = 1;
    for (size_t i = 0; i < size; i++) res->values[i] = PyValue_NULL;
    return res;
}

static inline void
free_values(PyDictValues *values, bool use_qsbr)
{
    assert(values->embedded == 0);
    PyMem_Free(values);
}

static inline PyObject *
new_dict_impl(PyDictObject *mp, PyDictKeysObject *keys,
              PyDictValues *values, Py_ssize_t used,
              int free_values_on_failure, int frozendict, int gc_track)
{
    assert(keys != NULL);
    if (mp == NULL) {
        dictkeys_decref(keys, false);
        if (free_values_on_failure) {
            free_values(values, false);
        }
        return NULL;
    }

    mp->ma_keys = keys;
    mp->ma_values = values;
    mp->ma_used = used;
    mp->_ma_watcher_tag = 0;
    if (frozendict) {
        ((PyFrozenDictObject *)mp)->ma_hash = -1;
    }
    ASSERT_CONSISTENT(mp);
    if (gc_track) {
        _PyObject_GC_TRACK(mp);
    }
    return (PyObject *)mp;
}

/* Consumes a reference to the keys object */
static PyObject*
new_dict(PyDictKeysObject *keys, PyDictValues *values,
         Py_ssize_t used, int free_values_on_failure)
{
    PyDictObject *mp = _Py_FREELIST_POP(PyDictObject, dicts);
    if (mp == NULL) {
        mp = PyObject_GC_New(PyDictObject, &PyDict_Type);
    }
    assert(mp == NULL || Py_IS_TYPE(mp, &PyDict_Type));

    return new_dict_impl(mp, keys, values, used, free_values_on_failure, 0, 1);
}

/* Consumes a reference to the keys object */
static PyObject*
new_dict_untracked(PyDictKeysObject *keys, PyDictValues *values,
                   Py_ssize_t used, int free_values_on_failure)
{
    PyDictObject *mp = _Py_FREELIST_POP(PyDictObject, dicts);
    if (mp == NULL) {
        mp = PyObject_GC_New(PyDictObject, &PyDict_Type);
    }
    assert(mp == NULL || Py_IS_TYPE(mp, &PyDict_Type));

    return new_dict_impl(mp, keys, values, used, free_values_on_failure, 0, 0);
}

/* Consumes a reference to the keys object */
static PyObject*
new_frozendict_untracked(PyDictKeysObject *keys, PyDictValues *values,
                         Py_ssize_t used, int free_values_on_failure)
{
    PyDictObject *mp = PyObject_GC_New(PyDictObject, &PyFrozenDict_Type);
    return new_dict_impl(mp, keys, values, used, free_values_on_failure, 1, 0);
}

static PyObject *
new_dict_with_shared_keys(PyDictKeysObject *keys)
{
    dictkeys_incref(keys);
    PyDictValues *values = new_values(shared_keys_usable_size(keys));
    if (values == NULL) {
        PyErr_NoMemory();
        dictkeys_decref(keys, false);
        return NULL;
    }
    return new_dict(keys, values, 0, 1);
}


static PyDictKeysObject *
clone_combined_dict_keys(PyDictObject *orig)
{
    assert(PyAnyDict_Check(orig));
    assert(Py_TYPE(orig)->tp_iter == dict_iter);
    assert(orig->ma_values == NULL);
    assert(orig->ma_keys != Py_EMPTY_KEYS);
    assert(orig->ma_keys->dk_refcnt == 1);

    size_t keys_size = _PyDict_KeysSize(orig->ma_keys);
    PyDictKeysObject *keys = PyMem_Malloc(keys_size);
    if (keys == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memcpy(keys, orig->ma_keys, keys_size);
    keys->dk_refcnt = 1;
    keys->dk_version = 0;
    for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
        if (DK_IS_UNICODE(keys)) {
            PyDictUnicodeEntry *entry = &DK_UNICODE_ENTRIES(keys)[i];
            if (entry->me_key != NULL) {
                Py_INCREF(entry->me_key);
            }
            if (!PyValue_IsNull(entry->me_value)) {
                entry->me_value = PyValue_DUP(entry->me_value);
            }
            _PyValue_CheckHome(entry->me_value);
        }
        else {
            PyDictKeyEntry *entry = &DK_ENTRIES(keys)[i];
            if (!PyValue_IsNull(entry->me_key)) {
                entry->me_key = PyValue_DUP(entry->me_key);
            }
            if (!PyValue_IsNull(entry->me_value)) {
                entry->me_value = PyValue_DUP(entry->me_value);
            }
            _PyValue_CheckHome(entry->me_key);
            _PyValue_CheckHome(entry->me_value);
        }
    }
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyThreadState_GET());
#endif
    return keys;
}

PyObject *
PyDict_New(void)
{
    /* We don't incref Py_EMPTY_KEYS here because it is immortal. */
    return new_dict(Py_EMPTY_KEYS, NULL, 0, 0);
}

/* Search index of hash table from offset of entry table */
static Py_ssize_t
lookdict_index(PyDictKeysObject *k, Py_hash_t hash, Py_ssize_t index)
{
    size_t mask = DK_MASK(k);
    size_t perturb = (size_t)hash;
    size_t i = (size_t)hash & mask;

    for (;;) {
        Py_ssize_t ix = dictkeys_get_index(k, i);
        if (ix == index) {
            return i;
        }
        if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    Py_UNREACHABLE();
}

static inline Py_ALWAYS_INLINE Py_ssize_t
do_lookup(PyDictObject *mp, PyDictKeysObject *dk, PyValueBorrow key, Py_hash_t hash,
          int (*check_lookup)(PyDictObject *, PyDictKeysObject *, void *, Py_ssize_t ix, PyValueBorrow key, Py_hash_t))
{
    void *ep0 = _DK_ENTRIES(dk);
    size_t mask = DK_MASK(dk);
    size_t perturb = hash;
    size_t i = (size_t)hash & mask;
    Py_ssize_t ix;
    for (;;) {
        ix = dictkeys_get_index(dk, i);
        if (ix >= 0) {
            int cmp = check_lookup(mp, dk, ep0, ix, key, hash);
            if (cmp < 0) {
                return cmp;
            } else if (cmp) {
                return ix;
            }
        }
        else if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);

        // Manual loop unrolling
        ix = dictkeys_get_index(dk, i);
        if (ix >= 0) {
            int cmp = check_lookup(mp, dk, ep0, ix, key, hash);
            if (cmp < 0) {
                return cmp;
            } else if (cmp) {
                return ix;
            }
        }
        else if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    Py_UNREACHABLE();
}

static inline int
compare_unicode_generic(PyDictObject *mp, PyDictKeysObject *dk,
                        void *ep0, Py_ssize_t ix, PyValueBorrow key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep = &((PyDictUnicodeEntry *)ep0)[ix];
    assert(ep->me_key != NULL);
    assert(PyUnicode_CheckExact(ep->me_key));
    assert(!value_is_exact_unicode(key));

    if (unicode_get_hash(ep->me_key) == hash) {
        PyObject *startkey = ep->me_key;
        Py_INCREF(startkey);
        int cmp = _PyValue_RichCompareBool(PyValue_FromObjectBorrow(startkey), key, Py_EQ);
        Py_DECREF(startkey);
        if (cmp < 0) {
            return DKIX_ERROR;
        }
        if (dk == mp->ma_keys && ep->me_key == startkey) {
            return cmp;
        }
        else {
            /* The dict was mutated, restart */
            return DKIX_KEY_CHANGED;
        }
    }
    return 0;
}

// Search non-Unicode key from Unicode table
static Py_ssize_t
unicodekeys_lookup_generic(PyDictObject *mp, PyDictKeysObject* dk, PyValueBorrow key, Py_hash_t hash)
{
    return do_lookup(mp, dk, key, hash, compare_unicode_generic);
}

static inline int
compare_unicode_unicode(PyDictObject *mp, PyDictKeysObject *dk,
                        void *ep0, Py_ssize_t ix, PyValueBorrow key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep = &((PyDictUnicodeEntry *)ep0)[ix];
    PyObject *key_object = PyValue_AsKnownObjectBorrow(key);
    PyObject *ep_key = FT_ATOMIC_LOAD_PTR_CONSUME(ep->me_key);
    assert(ep_key != NULL);
    assert(PyUnicode_CheckExact(ep_key));
    if (ep_key == key_object ||
            (unicode_get_hash(ep_key) == hash && unicode_eq(ep_key, key_object))) {
        return 1;
    }
    return 0;
}

static Py_ssize_t _Py_HOT_FUNCTION
unicodekeys_lookup_unicode(PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    return do_lookup(NULL, dk, PyValue_FromObjectBorrow(key), hash, compare_unicode_unicode);
}

static inline int
compare_generic(PyDictObject *mp, PyDictKeysObject *dk,
                void *ep0, Py_ssize_t ix, PyValueBorrow key, Py_hash_t hash)
{
    PyDictKeyEntry *ep = &((PyDictKeyEntry *)ep0)[ix];
    assert(!PyValue_IsNull(ep->me_key));
    if (PyValue_Is(ep->me_key, key)) return 1;
    if (ep->me_hash == hash) {
        PyValue startkey = PyValue_DUP(ep->me_key);
        uintptr_t startkey_word = _PyValue_Word(startkey);
        int cmp = _PyValue_RichCompareBool(startkey, key, Py_EQ);
        PyValue_CLOSE(startkey);
        if (cmp < 0) return DKIX_ERROR;
        if (dk == mp->ma_keys && _PyValue_Word(ep->me_key) == startkey_word) return cmp;
        return DKIX_KEY_CHANGED;
    }
    return 0;
}

static Py_ssize_t
dictkeys_generic_lookup(PyDictObject *mp, PyDictKeysObject* dk, PyValueBorrow key, Py_hash_t hash)
{
    return do_lookup(mp, dk, key, hash, compare_generic);
}

static bool
check_keys_unicode(PyDictKeysObject *dk, PyObject *key)
{
    return PyUnicode_CheckExact(key) && (dk->dk_kind != DICT_KEYS_GENERAL);
}

static Py_ssize_t
hash_unicode_key(PyObject *key)
{
    assert(PyUnicode_CheckExact(key));
    Py_hash_t hash = unicode_get_hash(key);
    if (hash == -1) {
        hash = _PyValue_Hash(PyValue_FromObjectBorrow(key));
        assert(hash != -1);
    }
    return hash;
}


static Py_ssize_t
unicodekeys_lookup_split(PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    Py_ssize_t ix;
    assert(dk->dk_kind == DICT_KEYS_SPLIT);
    assert(PyUnicode_CheckExact(key));

    ix = unicodekeys_lookup_unicode(dk, key, hash);
    return ix;
}

/* Lookup a string in a (all unicode) dict keys.
 * Returns DKIX_ERROR if key is not a string,
 * or if the dict keys is not all strings.
 * If the keys is present then return the index of key.
 * If the key is not present then return DKIX_EMPTY.
 */
Py_ssize_t
_PyDictKeys_StringLookup(PyDictKeysObject* dk, PyObject *key)
{
    if (!check_keys_unicode(dk, key)) {
        return DKIX_ERROR;
    }
    Py_hash_t hash = hash_unicode_key(key);
    return unicodekeys_lookup_unicode(dk, key, hash);
}

Py_ssize_t
_PyDictKeys_StringLookupAndVersion(PyDictKeysObject *dk, PyObject *key, uint32_t *version)
{
    if (!check_keys_unicode(dk, key)) {
        return DKIX_ERROR;
    }
    Py_ssize_t ix;
    Py_hash_t hash = hash_unicode_key(key);
    LOCK_KEYS(dk);
    ix = unicodekeys_lookup_unicode(dk, key, hash);
    *version = _PyDictKeys_GetVersionForCurrentState(_PyInterpreterState_GET(), dk);
    UNLOCK_KEYS(dk);
    return ix;
}

/* Like _PyDictKeys_StringLookup() but only works on split keys.  Note
 * that in free-threaded builds this locks the keys object as required.
 */
Py_ssize_t
_PyDictKeys_StringLookupSplit(PyDictKeysObject* dk, PyObject *key)
{
    assert(dk->dk_kind == DICT_KEYS_SPLIT);
    assert(PyUnicode_CheckExact(key));
    Py_hash_t hash = unicode_get_hash(key);
    if (hash == -1) {
        hash = _PyValue_Hash(PyValue_FromObjectBorrow(key));
        if (hash == -1) {
            PyErr_Clear();
            return DKIX_ERROR;
        }
    }
    return unicodekeys_lookup_split(dk, key, hash);
}

/*
The basic lookup function used by all operations.
This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.
Open addressing is preferred over chaining since the link overhead for
chaining would be substantial (100% with typical malloc overhead).

The initial probe index is computed as hash mod the table size. Subsequent
probe indices are computed as explained earlier.

All arithmetic on hash should ignore overflow.

_Py_dict_lookup() is general-purpose, and may return DKIX_ERROR if (and only if) a
comparison raises an exception.
When the key isn't found a DKIX_EMPTY is returned.
*/
Py_ssize_t
(_Py_dict_lookup)(PyDictObject *mp, PyValueBorrow key, Py_hash_t hash,
                  PyValueBorrow *value_addr)
{
    PyDictKeysObject *dk;
    DictKeysKind kind;
    Py_ssize_t ix;

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(mp);
start:
    dk = mp->ma_keys;
    kind = dk->dk_kind;

    if (kind != DICT_KEYS_GENERAL) {
        if (value_is_exact_unicode(key)) {
            ix = unicodekeys_lookup_unicode(dk, PyValue_AsKnownObjectBorrow(key), hash);
        }
        else {
            INCREF_KEYS_FT(dk);
            LOCK_KEYS_IF_SPLIT(dk, kind);

            ix = unicodekeys_lookup_generic(mp, dk, key, hash);

            UNLOCK_KEYS_IF_SPLIT(dk, kind);
            DECREF_KEYS_FT(dk, IS_DICT_SHARED(mp));
            if (ix == DKIX_KEY_CHANGED) {
                goto start;
            }
        }

        if (ix >= 0) {
            if (kind == DICT_KEYS_SPLIT) {
                *value_addr = PyValue_Borrow(mp->ma_values->values[ix]);
            }
            else {
                *value_addr = PyValue_Borrow(DK_UNICODE_ENTRIES(dk)[ix].me_value);
            }
        }
        else {
            *value_addr = PyValue_Borrow(PyValue_NULL);
        }
    }
    else {
        ix = dictkeys_generic_lookup(mp, dk, key, hash);
        if (ix == DKIX_KEY_CHANGED) {
            goto start;
        }
        if (ix >= 0) {
            *value_addr = PyValue_Borrow(DK_ENTRIES(dk)[ix].me_value);
        }
        else {
            *value_addr = PyValue_Borrow(PyValue_NULL);
        }
    }

    return ix;
}

int
(_PyDict_GetValue_KnownHash)(PyObject *op, PyValueBorrow key, Py_hash_t hash,
                           PyValue *result)
{
    *result = PyValue_NULL;
    if (!PyAnyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(PyValue_IsLanguageValue(key));
    assert(hash != -1);

    /* Equality can remove the query's last external owner. Capture the found
     * value before releasing our query pin, which can itself reenter. */
    PyValue query = PyValue_XDUP(key);
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t ix = _Py_dict_lookup((PyDictObject *)op, query, hash, &value);
    PyValue owned = PyValue_NULL;
    int status = 0;
    if (ix == DKIX_ERROR) {
        status = -1;
    }
    else if (!PyValue_IsNull(value)) {
        assert(ix >= 0);
        assert(PyValue_IsLanguageValue(value));
        _PyValue_RecheckHome(value);
        owned = PyValue_DUP(value);
        status = 1;
    }
    PyValue_XCLOSE(query);
    *result = owned;
    return status;
}


static inline void
ensure_shared_on_resize(PyDictObject *mp)
{
}

static inline void
ensure_shared_on_keys_version_assignment(PyDictObject *mp)
{
    ASSERT_DICT_LOCKED((PyObject *) mp);
}



Py_ssize_t
_Py_dict_lookup_threadsafe_value(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyValue *value_addr)
{
    PyValueBorrow value;
    Py_ssize_t ix = _Py_dict_lookup(mp, PyValue_FromObjectBorrow(key), hash, &value);
    *value_addr = PyValue_XDUP(value);
    return ix;
}


// Looks up the unicode key `key` in the dictionary. Note that `*method` may
// already contain a valid value! See _PyValue_GetMethod().
int
_PyDict_GetMethodValue(PyDictObject *mp, PyObject *key, PyValue *method)
{
    assert(PyUnicode_CheckExact(key));
    Py_hash_t hash = hash_unicode_key(key);
    PyValue value;
    Py_INCREF(mp);
    Py_ssize_t ix = _Py_dict_lookup_threadsafe_value(mp, key, hash, &value);
    Py_DECREF(mp);
    if (ix == DKIX_ERROR) {
        PyValue_CLEAR(*method);
        return -1;
    }
    if (!PyValue_IsNull(value)) {
        PyValue_XSETREF(*method, value);
        return 1;
    }
    return 0;
}

int
_PyDict_HasOnlyStringKeys(PyObject *dict)
{
    assert(PyDict_Check(dict));
    PyDictKeysObject *keys = ((PyDictObject *)dict)->ma_keys;
    if (keys->dk_kind != DICT_KEYS_GENERAL) {
        return 1;
    }
    /* No callbacks or owner cleanup may invalidate this local table scan. */
    PyDictKeyEntry *entries = DK_ENTRIES(keys);
    for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
        if (PyValue_IsNull(entries[i].me_value)) {
            continue;
        }
        PyValueBorrow key = PyValue_Borrow(entries[i].me_key);
        if (!PyValue_IsObject(key) || !PyUnicode_Check(PyValue_AsKnownObjectBorrow(key))) {
            return 0;
        }
    }
    return 1;
}

void
_PyDict_EnablePerThreadRefcounting(PyObject *op)
{
    assert(PyDict_Check(op));
}

static inline int
is_unusable_slot(Py_ssize_t ix)
{
    return ix >= 0;
}

/* Internal function to find slot for an item from its hash
   when it is known that the key is not present in the dict.
 */
static Py_ssize_t
find_empty_slot(PyDictKeysObject *keys, Py_hash_t hash)
{
    assert(keys != NULL);

    const size_t mask = DK_MASK(keys);
    size_t i = hash & mask;
    Py_ssize_t ix = dictkeys_get_index(keys, i);
    for (size_t perturb = hash; is_unusable_slot(ix);) {
        perturb >>= PERTURB_SHIFT;
        i = (i*5 + perturb + 1) & mask;
        ix = dictkeys_get_index(keys, i);
    }
    return i;
}

static int
insertion_resize(PyDictObject *mp, int unicode)
{
    return dictresize(mp, calculate_log2_keysize(GROWTH_RATE(mp)), unicode);
}

static inline int
insert_combined_dict(PyDictObject *mp, Py_hash_t hash,
                     PyValue key, PyValue value)
{
    /* Caller has revalidated absence and capacity without a subsequent call.
     * This pure commit consumes key and value only on success. */
    assert(mp->ma_values == NULL);
    assert(mp->ma_keys->dk_usable > 0);
    assert(PyValue_IsLanguageValue(key) && PyValue_IsLanguageValue(value));
    _PyValue_CheckHome(key);
    _PyValue_CheckHome(value);
    FT_ATOMIC_STORE_UINT32_RELAXED(mp->ma_keys->dk_version, 0);
    Py_ssize_t hashpos = find_empty_slot(mp->ma_keys, hash);
    Py_ssize_t ix = mp->ma_keys->dk_nentries;
    if (DK_IS_UNICODE(mp->ma_keys)) {
        assert(value_is_exact_unicode(key));
        PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[ix];
        STORE_KEY(ep, PyValue_AsKnownObjectSteal(key));
        STORE_VALUE(ep, value);
    }
    else {
        PyDictKeyEntry *ep = &DK_ENTRIES(mp->ma_keys)[ix];
        STORE_KEY(ep, key);
        STORE_VALUE(ep, value);
        STORE_HASH(ep, hash);
    }
    dictkeys_set_index(mp->ma_keys, hashpos, ix);
    STORE_KEYS_USABLE(mp->ma_keys, mp->ma_keys->dk_usable - 1);
    STORE_KEYS_NENTRIES(mp->ma_keys, ix + 1);
    STORE_USED(mp, mp->ma_used + 1);
    return 0;
}

static Py_ssize_t
insert_split_key(PyDictKeysObject *keys, PyObject *key, Py_hash_t hash)
{
    assert(PyUnicode_CheckExact(key));
    dictkeys_incref(keys);
    Py_ssize_t ix = unicodekeys_lookup_unicode(keys, key, hash);
    PyTypeObject *type = NULL;
    _PyTypeMutation scope = {0};
    int status = 0;
    if (ix == DKIX_EMPTY && keys->dk_usable > 0) {
        struct _instancekeysobject *shared = _PyDictKeys_AsSharedKeys(keys);
        type = shared->dsk_owning_type;
        Py_XINCREF(type);
        if (type != NULL) {
            status = _PyTypeMutation_Begin(&scope, type);
            if (status == 0) {
                _PyType_Modified_Unlocked(type);
            }
        }
        /* Type invalidation and its watchers can reenter. The pinned keys
         * remain live, but neither their capacity nor this lookup is cached. */
        if (status == 0) {
            ix = unicodekeys_lookup_unicode(keys, key, hash);
            if (ix == DKIX_EMPTY && keys->dk_usable > 0) {
                FT_ATOMIC_STORE_UINT32_RELAXED(keys->dk_version, 0);
                Py_ssize_t hashpos = find_empty_slot(keys, hash);
                ix = keys->dk_nentries;
                DK_UNICODE_ENTRIES(keys)[ix].me_key = Py_NewRef(key);
                dictkeys_set_index(keys, hashpos, ix);
                split_keys_entry_added(keys);
            }
        }
    }
    Py_XDECREF((PyObject *)type);
    dictkeys_decref(keys, false);
    _PyTypeMutation_End(&scope);
    return status < 0 ? DKIX_ERROR : ix;
}

int
_PyDict_InsertSplitValue(PyDictObject *mp, PyValue key, PyValue value,
                         Py_ssize_t ix)
{
    assert(value_is_exact_unicode(key));
    /* ix was selected before a possible callback. The checked provider owns
     * fresh inputs and re-probes instead of writing through that old index. */
    (void)ix;
    return _PyDict_SetValue_KnownHash((PyObject *)mp, key, value,
                                    hash_unicode_key(PyValue_AsKnownObjectBorrow(key)));
}

// Replace a value at an existing entry. Steals the new value reference.
static void
replace_value(PyDictObject *mp, PyValue key, Py_ssize_t ix,
              PyValueBorrow old_value, PyValue value)
{
    assert(can_modify_dict(mp));
    assert(PyValue_IsLanguageValue(value));
    _PyValue_CheckHome(value);
    /* The entry's owner moves out of the table and is closed below. It is
       still old_value, the value that the caller looked up. */
    PyValue owner;
    if (_PyDict_HasSplitTable(mp)) {
        owner = mp->ma_values->values[ix];
        STORE_SPLIT_VALUE(mp, ix, value);
    }
    else if (DK_IS_UNICODE(mp->ma_keys)) {
        owner = DK_UNICODE_ENTRIES(mp->ma_keys)[ix].me_value;
        STORE_VALUE(&DK_UNICODE_ENTRIES(mp->ma_keys)[ix], value);
    }
    else {
        owner = DK_ENTRIES(mp->ma_keys)[ix].me_value;
        STORE_VALUE(&DK_ENTRIES(mp->ma_keys)[ix], value);
    }
    assert(PyValue_IsLanguageValue(owner));
    assert(_PyValue_IsSameWord(owner, old_value));
    /* The replacement owner is published before this callback boundary. */
    PyValue_XCLOSE(owner);
}

/*
Internal routine to insert a new item into the table.
Used both by the internal resize routine and by the public insert routine.
Returns -1 if an error occurred, or 0 on success.
Consumes key and value references.
*/
static int
insertdict(PyDictObject *mp, PyValue key, Py_hash_t hash, PyValue value)
{
    assert(can_modify_dict(mp));
    assert(PyValue_IsLanguageValue(key) && PyValue_IsLanguageValue(value));
    int notified = 0, classified = 0;
    PyDict_WatchEvent event = PyDict_EVENT_ADDED;
    int status = -1;
    for (;;) {
        PyValueBorrow old_value = PyValue_Borrow(PyValue_NULL);
        Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &old_value);
        if (ix == DKIX_ERROR) {
            break;
        }
        if (!classified) {
            classified = 1;
            event = PyValue_IsNull(old_value) ?
                PyDict_EVENT_ADDED : PyDict_EVENT_MODIFIED;
        }
        if (PyValue_IsNull(old_value)) {
            if (_PyDict_HasSplitTable(mp) && value_is_exact_unicode(key)) {
                if (ix == DKIX_EMPTY) {
                    ix = insert_split_key(mp->ma_keys,
                                          PyValue_AsKnownObjectBorrow(key), hash);
                    if (ix == DKIX_ERROR) {
                        break;
                    }
                    /* Adding a shared key notifies type watchers, which can
                       run Python code, so look the key up again. */
                    if (ix >= 0) {
                        continue;
                    }
                    /* No room in the shared keys. Type watchers may have run
                       in insert_split_key() before it found that, so look the
                       key up again in the combined table. */
                    if (insertion_resize(mp, 1) < 0) {
                        break;
                    }
                    continue;
                }
            }
            else if (_PyDict_HasSplitTable(mp) ||
                     (DK_IS_UNICODE(mp->ma_keys) && !value_is_exact_unicode(key)) ||
                     mp->ma_keys->dk_usable <= 0) {
                /* Growth moves the entries and runs no Python code, so the
                   miss stays valid: insert into an empty slot without a
                   second lookup, which would call __eq__ again. */
                if (insertion_resize(mp, value_is_exact_unicode(key)) < 0) {
                    break;
                }
                assert(!_PyDict_HasSplitTable(mp));
                assert(mp->ma_keys->dk_usable > 0);
            }
        }
        if (!notified) {
            notified = 1;
            /* Keep the first lookup's event even if a type watcher changed
             * the dictionary. Required allocation still precedes notification. */
            /* Storing the value already present still reports MODIFIED, as
             * upstream does. */
            if (mp->_ma_watcher_tag & DICT_WATCHER_MASK) {
                _PyDict_NotifyEvent(event, mp, key, value);
                /* Dict watchers can run Python code that mutates the
                   dictionary, so look the key up again. */
                continue;
            }
        }
        if (!PyValue_IsNull(old_value)) {
            replace_value(mp, key, ix, old_value, value);
            value = PyValue_NULL;
        }
        else if (_PyDict_HasSplitTable(mp)) {
            assert(ix >= 0);
            _PyValue_CheckHome(value);
            STORE_SPLIT_VALUE(mp, ix, value);
            value = PyValue_NULL;
            _PyDictValues_AddToInsertionOrder(mp->ma_values, ix);
            STORE_USED(mp, mp->ma_used + 1);
        }
        else {
            insert_combined_dict(mp, hash, key, value);
            key = value = PyValue_NULL;
        }
        status = 0;
        break;
    }
    PyValue_XCLOSE(value);
    PyValue_XCLOSE(key);
    return status;
}

// Same as insertdict but specialized for ma_keys == Py_EMPTY_KEYS.
// Consumes key and value references.
static int
insert_to_emptydict(PyDictObject *mp, PyValue key, Py_hash_t hash,
                    PyValue value)
{
    return insertdict(mp, key, hash, value);
}

/*
Internal routine used by dictresize() to build a hashtable of entries.
*/
static void
build_indices_generic(PyDictKeysObject *keys, PyDictKeyEntry *ep, Py_ssize_t n)
{
    size_t mask = DK_MASK(keys);
    for (Py_ssize_t ix = 0; ix != n; ix++, ep++) {
        Py_hash_t hash = ep->me_hash;
        size_t i = hash & mask;
        for (size_t perturb = hash; dictkeys_get_index(keys, i) != DKIX_EMPTY;) {
            perturb >>= PERTURB_SHIFT;
            i = mask & (i*5 + perturb + 1);
        }
        dictkeys_set_index(keys, i, ix);
    }
}

static void
build_indices_unicode(PyDictKeysObject *keys, PyDictUnicodeEntry *ep, Py_ssize_t n)
{
    size_t mask = DK_MASK(keys);
    for (Py_ssize_t ix = 0; ix != n; ix++, ep++) {
        Py_hash_t hash = unicode_get_hash(ep->me_key);
        assert(hash != -1);
        size_t i = hash & mask;
        for (size_t perturb = hash; dictkeys_get_index(keys, i) != DKIX_EMPTY;) {
            perturb >>= PERTURB_SHIFT;
            i = mask & (i*5 + perturb + 1);
        }
        dictkeys_set_index(keys, i, ix);
    }
}

static void
invalidate_and_clear_inline_values(PyDictValues *values)
{
    assert(values->embedded);
    FT_ATOMIC_STORE_UINT8(values->valid, 0);
    _PyDictValues_SetShape(values, _Py_SHAPE_NONE);
    for (int i = 0; i < values->capacity; i++) {
        FT_ATOMIC_STORE_PTR_RELEASE(values->values[i], PyValue_NULL);
    }
}

/*
Restructure the table by allocating a new table and reinserting all
items again.  When entries have been deleted, the new table may
actually be smaller than the old one.
If a table is split (its keys and hashes are shared, its values are not),
then the values are temporarily copied into the table, it is resized as
a combined table, then the me_value slots in the old table are NULLed out.
After resizing, a table is always combined.

This function supports:
 - Unicode split -> Unicode combined or Generic
 - Unicode combined -> Unicode combined or Generic
 - Generic -> Generic
*/
static int
dictresize(PyDictObject *mp, uint8_t log2_newsize, int unicode)
{
    assert(can_modify_dict(mp));
    if (log2_newsize >= SIZEOF_SIZE_T * 8) {
        PyErr_NoMemory();
        return -1;
    }
    assert(log2_newsize >= PyDict_LOG_MINSIZE);

    if (!DK_IS_UNICODE(mp->ma_keys)) {
        unicode = 0;
    }
    /* Allocate a new table. */
    PyDictKeysObject *newkeys = new_keys_object(log2_newsize, unicode);
    if (newkeys == NULL) {
        return -1;
    }
    // New table must be large enough.
    assert(newkeys->dk_usable >= mp->ma_used);

    PyDictKeysObject *oldkeys = mp->ma_keys;
    PyDictValues *oldvalues = mp->ma_values;
    Py_ssize_t numentries = mp->ma_used;
    bool shared = IS_DICT_SHARED(mp);
    bool embedded = oldvalues != NULL && oldvalues->embedded;
    assert(oldvalues != NULL || oldkeys == Py_EMPTY_KEYS || oldkeys->dk_refcnt == 1);
    ensure_shared_on_resize(mp);

    if (oldvalues != NULL) {
        PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
        if (newkeys->dk_kind == DICT_KEYS_GENERAL) {
            PyDictKeyEntry *entries = DK_ENTRIES(newkeys);
            for (Py_ssize_t i = 0; i < numentries; i++) {
                int index = get_index_from_order(mp, i);
                PyObject *key = oldentries[index].me_key;
                entries[i].me_key = PyValue_DUP(PyValue_FromObjectBorrow(key));
                entries[i].me_hash = unicode_get_hash(key);
                entries[i].me_value = oldvalues->values[index];
                oldvalues->values[index] = PyValue_NULL;
            }
            build_indices_generic(newkeys, entries, numentries);
        }
        else {
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(newkeys);
            for (Py_ssize_t i = 0; i < numentries; i++) {
                int index = get_index_from_order(mp, i);
                entries[i].me_key = Py_NewRef(oldentries[index].me_key);
                entries[i].me_value = oldvalues->values[index];
                oldvalues->values[index] = PyValue_NULL;
            }
            build_indices_unicode(newkeys, entries, numentries);
        }
        if (embedded) {
            invalidate_and_clear_inline_values(oldvalues);
        }
    }
    else if (oldkeys->dk_kind == DICT_KEYS_GENERAL) {
        assert(newkeys->dk_kind == DICT_KEYS_GENERAL);
        PyDictKeyEntry *oldentries = DK_ENTRIES(oldkeys);
        PyDictKeyEntry *entries = DK_ENTRIES(newkeys);
        Py_ssize_t next = 0;
        for (Py_ssize_t i = 0; i < oldkeys->dk_nentries; i++) {
            if (!PyValue_IsNull(oldentries[i].me_value)) {
                entries[next++] = oldentries[i];
                oldentries[i].me_key = PyValue_NULL;
                oldentries[i].me_value = PyValue_NULL;
            }
        }
        assert(next == numentries);
        build_indices_generic(newkeys, entries, numentries);
    }
    else {
        PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
        Py_ssize_t next = 0;
        for (Py_ssize_t i = 0; i < oldkeys->dk_nentries; i++) {
            if (PyValue_IsNull(oldentries[i].me_value)) {
                continue;
            }
            if (newkeys->dk_kind == DICT_KEYS_GENERAL) {
                PyDictKeyEntry *entry = &DK_ENTRIES(newkeys)[next++];
                entry->me_hash = unicode_get_hash(oldentries[i].me_key);
                entry->me_key = dict_take_unicode_key(oldentries[i].me_key);
                entry->me_value = oldentries[i].me_value;
            }
            else {
                DK_UNICODE_ENTRIES(newkeys)[next++] = oldentries[i];
            }
            oldentries[i].me_key = NULL;
            oldentries[i].me_value = PyValue_NULL;
        }
        assert(next == numentries);
        if (newkeys->dk_kind == DICT_KEYS_GENERAL) {
            build_indices_generic(newkeys, DK_ENTRIES(newkeys), numentries);
        }
        else {
            build_indices_unicode(newkeys, DK_UNICODE_ENTRIES(newkeys), numentries);
        }
    }

    newkeys->dk_usable -= numentries;
    newkeys->dk_nentries = numentries;
    set_keys(mp, newkeys);
    set_values(mp, NULL);
    if (oldvalues != NULL && !embedded) {
        free_values(oldvalues, shared);
    }
    if (oldkeys != Py_EMPTY_KEYS) {
        dictkeys_decref(oldkeys, shared);
    }
    return 0;
}

static PyObject *
dict_new_presized(Py_ssize_t minused, bool unicode)
{
    const uint8_t log2_max_presize = 17;
    const Py_ssize_t max_presize = ((Py_ssize_t)1) << log2_max_presize;
    uint8_t log2_newsize;
    PyDictKeysObject *new_keys;

    if (minused <= USABLE_FRACTION(PyDict_MINSIZE)) {
        return PyDict_New();
    }
    /* There are no strict guarantee that returned dict can contain minused
     * items without resize.  So we create medium size dict instead of very
     * large dict or MemoryError.
     */
    if (minused > USABLE_FRACTION(max_presize)) {
        log2_newsize = log2_max_presize;
    }
    else {
        log2_newsize = estimate_log2_keysize(minused);
    }

    new_keys = new_keys_object(log2_newsize, unicode);
    if (new_keys == NULL)
        return NULL;
    return new_dict(new_keys, NULL, 0, 0);
}

PyObject *
_PyDict_NewPresized(Py_ssize_t minused)
{
    return dict_new_presized(minused, false);
}

PyObject *
_PyDict_FromValueItems(const PyValue *keys, Py_ssize_t key_stride,
                       const PyValue *values, Py_ssize_t value_stride,
                       Py_ssize_t count)
{
    if (count < 0 || key_stride < 0 || value_stride < 0 ||
        (count != 0 && (keys == NULL || values == NULL))) {
        PyErr_BadInternalCall();
        return NULL;
    }
    Py_ssize_t max_step = PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(PyValue);
    if (count > 1 && (key_stride > max_step / (count - 1) ||
                      value_stride > max_step / (count - 1))) {
        PyErr_SetString(PyExc_OverflowError, "dictionary value array stride overflow");
        return NULL;
    }
    /* The caller's independently owning snapshot has stable address and
     * contents through this entire call, including allocation and hashing. */
    bool unicode = true;
    for (Py_ssize_t i = 0; i < count; i++) {
        assert(PyValue_IsLanguageValue(keys[i * key_stride]));
        assert(PyValue_IsLanguageValue(values[i * value_stride]));
        unicode = unicode && value_is_exact_unicode(keys[i * key_stride]);
    }
    PyObject *dict = dict_new_presized(count, unicode);
    if (dict == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < count; i++) {
        PyValue key = PyValue_DUP(keys[i * key_stride]);
        PyValue value = PyValue_DUP(values[i * value_stride]);
        if (_PyDict_SetValue_Take2((PyDictObject *)dict, key, value) < 0) {
            Py_XDECREF(dict);
            return NULL;
        }
    }
    return dict;
}

PyObject *
_PyDict_FromItems(PyObject *const *keys, Py_ssize_t keys_offset,
                  PyObject *const *values, Py_ssize_t values_offset,
                  Py_ssize_t length)
{
    bool unicode = true;
    PyObject *const *ks = keys;

    for (Py_ssize_t i = 0; i < length; i++) {
        if (!PyUnicode_CheckExact(*ks)) {
            unicode = false;
            break;
        }
        ks += keys_offset;
    }

    PyObject *dict = dict_new_presized(length, unicode);
    if (dict == NULL) {
        return NULL;
    }

    ks = keys;
    PyObject *const *vs = values;

    for (Py_ssize_t i = 0; i < length; i++) {
        PyObject *key = *ks;
        PyObject *value = *vs;
        if (setitem_lock_held((PyDictObject *)dict, key, value) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        ks += keys_offset;
        vs += values_offset;
    }

    return dict;
}


static void
dict_unhashable_type(PyObject *op, PyObject *key)
{
    PyObject *exc = PyErr_GetRaisedException();
    assert(exc != NULL);
    if (!Py_IS_TYPE(exc, (PyTypeObject*)PyExc_TypeError)) {
        PyErr_SetRaisedException(exc);
        return;
    }

    const char *errmsg;
    if (PyFrozenDict_Check(op)) {
        errmsg = "cannot use '%T' as a frozendict key (%S)";
    }
    else {
        errmsg = "cannot use '%T' as a dict key (%S)";
    }
    PyErr_Format(PyExc_TypeError, errmsg, key, exc);
    Py_DECREF(exc);
}

/* The cached hash of an exact str key, or -1 if key is not one or has none
   (the dictionary fast path of _PyObject_HashDictKey()). */
static inline Py_hash_t
cached_str_hash(PyValueBorrow key)
{
    if (!PyValue_IsObject(key)) {
        return -1;
    }
    PyObject *object = PyValue_AsKnownObjectBorrow(key);
    if (!PyUnicode_CheckExact(object)) {
        return -1;
    }
    return PyUnstable_Unicode_GET_CACHED_HASH(object);
}

static inline int
can_use_cached_str_lookup(PyDictObject *mp, Py_hash_t cached)
{
#ifndef Py_GIL_DISABLED
    /* A Unicode-only table compares exact strings without callbacks.
     * Mixed tables may call a stored key's Python equality method. */
    return cached != -1 && mp->ma_keys->dk_kind != DICT_KEYS_GENERAL;
#else
    /* Do not classify a mutable keys table without its synchronization. */
    (void)mp;
    (void)cached;
    return 0;
#endif
}

int
(PyDict_GetValue)(PyObject *op, PyValueBorrow key, PyValue *result)
{
    *result = PyValue_NULL;
    if (!PyAnyDict_Check(op) || !PyValue_IsLanguageValue(key)) {
        PyErr_BadInternalCall();
        return -1;
    }
    Py_hash_t cached = cached_str_hash(key);
    if (can_use_cached_str_lookup((PyDictObject *)op, cached)) {
        return _PyDict_GetValue_KnownHash(op, key, cached, result);
    }
    PyObject *dict_owner = Py_NewRef(op);
    PyValue query = PyValue_XDUP(key);
    PyValue value = PyValue_NULL;
    Py_hash_t hash = _PyValue_Hash(query);
    int status = -1;
    if (hash != -1) {
        status = _PyDict_GetValue_KnownHash(op, query, hash, &value);
    }
    else if (PyValue_IsObject(query)) {
        dict_unhashable_type(op, PyValue_AsKnownObjectBorrow(query));
    }
    /* Hashing and cleanup can remove the inputs' last external owners. Keep
     * the selected snapshot independent until every local owner is retired. */
    PyValue_XCLOSE(query);
    Py_XDECREF(dict_owner);
    *result = value;
    return status;
}

int
PyDict_GetValueString(PyObject *op, const char *key, PyValue *result)
{
    *result = PyValue_NULL;
    if (!PyAnyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    PyObject *dict_owner = Py_NewRef(op);
    PyObject *name = PyUnicode_FromString(key);
    PyValue value = PyValue_NULL;
    int status = -1;
    if (name != NULL) {
        status = PyDict_GetValue(op, PyValue_FromObjectBorrow(name), &value);
    }
    Py_XDECREF(name);
    Py_XDECREF(dict_owner);
    *result = value;
    return status;
}

int
(PyDict_ContainsValue)(PyObject *op, PyValueBorrow key)
{
    if (!PyAnyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    Py_hash_t cached = cached_str_hash(key);
    if (can_use_cached_str_lookup((PyDictObject *)op, cached)) {
        /* As dict_contains(): no result copy, no extra owners. */
        return _PyDict_Contains_KnownHash(op, PyValue_AsKnownObjectBorrow(key),
                                          cached);
    }
    PyValue value = PyValue_NULL;
    int status = PyDict_GetValue(op, key, &value);
    PyValue_XCLOSE(value);
    return status;
}

int
PyDict_NextValue(PyObject *op, Py_ssize_t *position,
                 PyValue *key, PyValue *value)
{
    if (key != NULL) {
        *key = PyValue_NULL;
    }
    if (value != NULL) {
        *value = PyValue_NULL;
    }
    if (key != NULL && key == value) {
        PyErr_SetString(PyExc_ValueError, "dictionary outputs must be distinct");
        return -1;
    }
    return _PyDict_NextValue(op, position, key, value, NULL);
}

Py_ssize_t
_PyDict_LookupIndexAndOwnedValue(PyDictObject *mp, PyObject *name, PyValue *out)
{
    *out = PyValue_NULL;
    assert(PyDict_CheckExact(mp));
    assert(PyUnicode_CheckExact(name));
    /* Caller retains the dictionary and exact Unicode metadata name. The
     * returned index remains usable only with the same current keys/layout. */
    PyValue query = PyValue_DUP(PyValue_FromObjectBorrow(name));
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    Py_hash_t hash = _PyValue_Hash(PyValue_FromObjectBorrow(name));
    Py_ssize_t index = hash == -1 ? DKIX_ERROR : _Py_dict_lookup(mp, query, hash, &value);
    if (index >= 0 && !PyValue_IsNull(value)) {
        *out = PyValue_DUP(value);
    }
    PyValue_XCLOSE(query);
    return index;
}


Py_ssize_t
_PyDict_LookupIndex(PyDictObject *mp, PyObject *key)
{
    assert(PyDict_CheckExact(mp));
    assert(PyUnicode_CheckExact(key));
    /* The caller roots the exact metadata name and dictionary. No owned
     * value is released after selecting the returned index. */
    Py_hash_t hash = _PyObject_HashDictKey(key);
    if (hash == -1) {
        return DKIX_ERROR;
    }
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    return _Py_dict_lookup(mp, PyValue_FromObjectBorrow(key), hash, &value);
}


/* The entries below pass the box argument of their caller (THUNK). */
static int
dict_getitemref_owned(PyObject *op, PyObject *key, Py_hash_t hash,
                      PyObject **result _PyBox_AT_PARAM)
{
    *result = NULL;
    Py_INCREF(op);
    Py_INCREF(key);
    PyValue query = PyValue_DUP(PyValue_FromObjectBorrow(key));
    PyValue value = PyValue_NULL;
    PyObject *object = NULL;
    int status = -1;
    if (hash == -1) {
        hash = _PyValue_Hash(query);
        if (hash == -1) {
            dict_unhashable_type(op, key);
            goto done;
        }
    }
    status = _PyDict_GetValue_KnownHash(op, query, hash, &value);
    if (status == 1) {
        object = _PyValue_AsObjectFor(value, at);
        if (object == NULL) {
            status = -1;
        }
    }
done:
    PyValue_XCLOSE(value);
    PyValue_XCLOSE(query);
    Py_XDECREF(key);
    Py_XDECREF(op);
    if (status < 0) {
        Py_XDECREF(object);
        return -1;
    }
    *result = object;
    return status;
}

/* Gets an item and provides a new reference if the value is present.
 * Returns 1 if the key is present, 0 if the key is missing, and -1 if an
 * exception occurred.
*/
int
_PyDict_GetItemRef_KnownHash_LockHeld(PyDictObject *op, PyObject *key,
                                      Py_hash_t hash, PyObject **result)
{
    return dict_getitemref_owned(
        (PyObject *)op, key, hash, result
        _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
}

/* Gets an item and provides a new reference if the value is present.
 * Returns 1 if the key is present, 0 if the key is missing, and -1 if an
 * exception occurred.
*/
int
_PyDict_GetItemRef_KnownHash(PyDictObject *op, PyObject *key,
                            Py_hash_t hash, PyObject **result)
{
    return dict_getitemref_owned(
        (PyObject *)op, key, hash, result
        _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
}

int
PyDict_GetItemRef(PyObject *op, PyObject *key, PyObject **result)
{
    if (!PyAnyDict_Check(op)) {
        *result = NULL;
        PyErr_BadInternalCall();
        return -1;
    }
    return dict_getitemref_owned(
        op, key, -1, result _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
}

int
_PyDict_GetItemRef_Unicode_LockHeld(PyDictObject *op, PyObject *key,
                                    PyObject **result)
{
    ASSERT_DICT_LOCKED(op);
    assert(PyUnicode_CheckExact(key));
    return dict_getitemref_owned(
        (PyObject *)op, key, -1, result
        _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
}


PyObject *
_PyDict_LoadGlobalValue(PyDictObject *globals, PyDictObject *builtins, PyObject *key, PyValue *res)
{
    Py_ssize_t ix;
    Py_hash_t hash;

    hash = _PyObject_HashDictKey(key);
    if (hash == -1) {
        *res = PyValue_NULL;
        return NULL;
    }

    /* namespace 1: globals */
    ix = _Py_dict_lookup_threadsafe_value(globals, key, hash, res);
    if (ix == DKIX_ERROR) {
        return NULL;
    }
    if (ix != DKIX_EMPTY && !PyValue_IsNull(*res)) {
        return (PyObject *)globals;
    }

    /* namespace 2: builtins */
    ix = _Py_dict_lookup_threadsafe_value(builtins, key, hash, res);
    assert(ix >= 0 || PyValue_IsNull(*res));
    return PyValue_IsNull(*res) ? NULL : (PyObject *)builtins;
}

PyObject *
_PyDict_LoadBuiltinsFromGlobals(PyObject *globals)
{
    if (!PyAnyDict_Check(globals)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    PyDictObject *mp = (PyDictObject *)globals;
    PyObject *key = &_Py_ID(__builtins__);
    Py_hash_t hash = unicode_get_hash(key);

    PyValue ref;
    Py_ssize_t ix = _Py_dict_lookup_threadsafe_value(mp, key, hash, &ref);
    if (ix == DKIX_ERROR) {
        return NULL;
    }
    if (PyValue_IsNull(ref)) {
        return Py_NewRef(PyEval_GetBuiltins());
    }
    PyObject *object = _PyValue_AsObjectAt(ref, BOX_SITE_DICT);
    if (object == NULL) {
        PyValue_XCLOSE(ref);
        return NULL;
    }
    PyObject *builtins = object;
    if (PyModule_Check(builtins)) {
        builtins = _PyModule_GetDict(builtins);
        assert(builtins != NULL);
    }
    _Py_INCREF_BUILTINS(builtins);
    Py_XDECREF(object);
    PyValue_XCLOSE(ref);
    return builtins;
}

#define frozendict_does_not_support(WHAT) \
    PyErr_SetString(PyExc_TypeError, "frozendict object does " \
                    "not support item " WHAT)

/* Consumes references to key and value */
static int
setitem_take2_lock_held_known_hash(PyDictObject *mp, PyValue key,
                                   PyValue value, Py_hash_t hash)
{
    assert(PyValue_IsLanguageValue(key) && PyValue_IsLanguageValue(value));
    if (!PyAnyDict_Check(mp)) {
        PyErr_BadInternalCall();
        PyValue_XCLOSE(value);
        PyValue_XCLOSE(key);
        return -1;
    }
    assert(can_modify_dict(mp));
    assert(hash != -1);
    if (mp->ma_keys == Py_EMPTY_KEYS) {
        return insert_to_emptydict(mp, key, hash, value);
    }
    return insertdict(mp, key, hash, value);
}

static int
setitem_take2_lock_held(PyDictObject *mp, PyValue key, PyValue value)
{
    assert(PyValue_IsLanguageValue(key) && PyValue_IsLanguageValue(value));
    if (!PyAnyDict_Check(mp)) {
        PyErr_BadInternalCall();
        PyValue_XCLOSE(value);
        PyValue_XCLOSE(key);
        return -1;
    }
    Py_hash_t hash = _PyValue_Hash(key);
    if (hash == -1) {
        if (PyValue_IsObject(key)) {
            dict_unhashable_type((PyObject *)mp, PyValue_AsKnownObjectBorrow(key));
        }
        PyValue_XCLOSE(value);
        PyValue_XCLOSE(key);
        return -1;
    }
    return setitem_take2_lock_held_known_hash(mp, key, value, hash);
}

int
_PyDict_SetValue_Take2(PyDictObject *mp, PyValue key, PyValue value)
{
    int status;
    Py_BEGIN_CRITICAL_SECTION(mp);
    status = setitem_take2_lock_held(mp, key, value);
    Py_END_CRITICAL_SECTION();
    return status;
}

int
_PyDict_SetValue_Take2_KnownHash(PyDictObject *mp, PyValue key, PyValue value,
                              Py_hash_t hash)
{
    int status;
    Py_BEGIN_CRITICAL_SECTION(mp);
    status = setitem_take2_lock_held_known_hash(mp, key, value, hash);
    Py_END_CRITICAL_SECTION();
    return status;
}

int
(PyDict_SetValue)(PyObject *op, PyValueBorrow key, PyValueBorrow value)
{
    assert(PyValue_IsLanguageValue(key) && PyValue_IsLanguageValue(value));
    if (!PyDict_Check(op)) {
        if (PyFrozenDict_Check(op)) {
            frozendict_does_not_support("assignment");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }
    return _PyDict_SetValue_Take2((PyDictObject *)op,
        PyValue_XDUP(key),
        PyValue_XDUP(value));
}

int
(_PyDict_SetValue_KnownHash)(PyObject *op, PyValueBorrow key, PyValueBorrow value,
                              Py_hash_t hash)
{
    assert(PyValue_IsLanguageValue(key) && PyValue_IsLanguageValue(value));
    if (!PyDict_Check(op)) {
        if (PyFrozenDict_Check(op)) {
            frozendict_does_not_support("assignment");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }
    return _PyDict_SetValue_Take2_KnownHash((PyDictObject *)op,
        PyValue_DUP(key),
        PyValue_DUP(value), hash);
}

int
_PyDict_SetItem_Take2(PyDictObject *mp, PyObject *key, PyObject *value)
{
    PyValue key_value = PyValue_PrepareOwned(key);
    PyValue value_value = PyValue_PrepareOwned(value);
    return _PyDict_SetValue_Take2(mp, key_value, value_value);
}

int
_PyDict_SetItem_Take2_KnownHash(PyDictObject *mp, PyObject *key,
                              PyObject *value, Py_hash_t hash)
{
    PyValue key_value = PyValue_PrepareOwned(key);
    PyValue value_value = PyValue_PrepareOwned(value);
    return _PyDict_SetValue_Take2_KnownHash(mp, key_value, value_value, hash);
}

/* CAUTION: PyDict_SetItem() must guarantee that it won't resize the
 * dictionary if it's merely replacing the value for an existing key.
 * This means that it's safe to loop over a dictionary with PyDict_NextEntryRef()
 * and occasionally replace a value -- but you can't insert new keys or
 * remove them.
 */
int
PyDict_SetItem(PyObject *op, PyObject *key, PyObject *value)
{
    assert(key != NULL && value != NULL);
    return PyDict_SetValue(op, PyValue_FromObjectBorrow(key),
                          PyValue_FromObjectBorrow(value));
}

static int
_PyAnyDict_SetItem(PyObject *op, PyObject *key, PyObject *value)
{
    assert(PyAnyDict_Check(op));
    return _PyDict_SetValue_Take2((PyDictObject *)op,
        PyValue_DUP(PyValue_FromObjectBorrow(key)),
        PyValue_DUP(PyValue_FromObjectBorrow(value)));
}

static int
setitem_lock_held(PyDictObject *mp, PyObject *key, PyObject *value)
{
    return setitem_take2_lock_held(mp,
        PyValue_DUP(PyValue_FromObjectBorrow(key)),
        PyValue_DUP(PyValue_FromObjectBorrow(value)));
}


int
_PyDict_SetItem_KnownHash_LockHeld(PyDictObject *mp, PyObject *key,
                                  PyObject *value, Py_hash_t hash)
{
    return setitem_take2_lock_held_known_hash(mp,
        PyValue_DUP(PyValue_FromObjectBorrow(key)),
        PyValue_DUP(PyValue_FromObjectBorrow(value)), hash);
}

int
_PyDict_SetItem_KnownHash(PyObject *op, PyObject *key, PyObject *value,
                          Py_hash_t hash)
{
    return _PyDict_SetValue_KnownHash(op, PyValue_FromObjectBorrow(key),
                                    PyValue_FromObjectBorrow(value), hash);
}

static void
delete_index_from_values(PyDictValues *values, Py_ssize_t ix)
{
    uint8_t *array = get_insertion_order_array(values);
    int size = values->size;
    assert(size <= values->capacity);
    int i;
    for (i = 0; array[i] != ix; i++) {
        assert(i < size);
    }
    assert(i < size);
    size--;
    for (; i < size; i++) {
        FT_ATOMIC_STORE_UINT8_RELAXED(array[i], array[i+1]);
    }
    FT_ATOMIC_STORE_UINT8_RELEASE(values->size, size);
}

static void
delitem_common(PyDictObject *mp, Py_hash_t hash, Py_ssize_t ix,
               PyValueBorrow selected)
{
    assert(can_modify_dict(mp));
    PyValue old_key = PyValue_NULL;
    /* The entry's value owner moves out of the table and is closed below. It
       is still selected, the value that the caller looked up. */
    PyValue old_value;
    if (_PyDict_HasSplitTable(mp)) {
        old_value = mp->ma_values->values[ix];
        STORE_SPLIT_VALUE(mp, ix, PyValue_NULL);
        delete_index_from_values(mp->ma_values, ix);
    }
    else {
        Py_ssize_t hashpos = lookdict_index(mp->ma_keys, hash, ix);
        assert(hashpos >= 0);
        FT_ATOMIC_STORE_UINT32_RELAXED(mp->ma_keys->dk_version, 0);
        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[ix];
            old_key = dict_take_unicode_key(ep->me_key);
            old_value = ep->me_value;
            STORE_KEY(ep, NULL);
            STORE_VALUE(ep, PyValue_NULL);
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(mp->ma_keys)[ix];
            old_key = ep->me_key;
            old_value = ep->me_value;
            STORE_KEY(ep, PyValue_NULL);
            STORE_VALUE(ep, PyValue_NULL);
            STORE_HASH(ep, 0);
        }
        dictkeys_set_index(mp->ma_keys, hashpos, DKIX_DUMMY);
    }
    assert(_PyValue_IsSameWord(old_value, selected));
    STORE_USED(mp, mp->ma_used - 1);
    /* Publish vacancy, order and count before either original owner dies. */
    PyValue_XCLOSE(old_key);
    PyValue_XCLOSE(old_value);
}

/* Borrow the stored key only until the next callback. */
static PyValueBorrow
dict_entry_key(PyDictObject *mp, Py_ssize_t ix)
{
    if (DK_IS_UNICODE(mp->ma_keys)) {
        return PyValue_FromObjectBorrow(DK_UNICODE_ENTRIES(mp->ma_keys)[ix].me_key);
    }
    return PyValue_Borrow(DK_ENTRIES(mp->ma_keys)[ix].me_key);
}

/* Never dereference a saved table/value-array pointer after a callback. An
 * address may have been reused: read only the current allocation, with its
 * current kind/bounds, and compare independently pinned key/value identities.
 * Identical inline values reappearing at this current slot remain a valid
 * selection; no user equality call is needed just to validate that fact. */
static int
dict_selection_matches(PyDictObject *mp, PyDictKeysObject *saved_keys,
                       Py_ssize_t ix, Py_hash_t hash, PyValue key, PyValue value)
{
    PyDictKeysObject *keys = mp->ma_keys;
    if (keys != saved_keys || ix < 0 || ix >= keys->dk_nentries) {
        return 0;
    }
    PyValue current_value;
    if (mp->ma_values != NULL) {
        if (ix >= mp->ma_values->capacity) {
            return 0;
        }
        current_value = mp->ma_values->values[ix];
    }
    else if (DK_IS_UNICODE(keys)) {
        current_value = DK_UNICODE_ENTRIES(keys)[ix].me_value;
    }
    else {
        current_value = DK_ENTRIES(keys)[ix].me_value;
    }
    if (PyValue_IsNull(current_value) || !_PyValue_IsSameWord(current_value, value)) {
        return 0;
    }
    if (DK_IS_UNICODE(keys)) {
        /* A Unicode table holds its keys as objects, and a word rebuilt from
           one tags it if it is immortal now. The key may have become
           immortal (interned) since it was selected and retained, so compare
           the objects, not the words. */
        if (DK_UNICODE_ENTRIES(keys)[ix].me_key
                != PyValue_AsKnownObjectBorrow(key)) {
            return 0;
        }
    }
    else if (!_PyValue_IsSameWord(DK_ENTRIES(keys)[ix].me_key, key)) {
        return 0;
    }
    Py_hash_t current_hash = DK_IS_UNICODE(keys) ?
        unicode_get_hash(DK_UNICODE_ENTRIES(keys)[ix].me_key) :
        DK_ENTRIES(keys)[ix].me_hash;
    return current_hash == hash && lookdict_index(keys, hash, ix) >= 0;
}

static void
dict_value_keyerror(PyValue key)
{
    _PyErr_SetKeyErrorValue(key);
}

#define delvalue_knownhash_lock_held(V0, V1, V2) \
    (delvalue_knownhash_lock_held)(V0, PyValue_Borrow(V1), V2)
static int
(delvalue_knownhash_lock_held)(PyDictObject *mp, PyValueBorrow key, Py_hash_t hash)
{
    PyValue query = PyValue_DUP(key);
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    int status = -1;
    Py_ssize_t ix = _Py_dict_lookup(mp, query, hash, &value);
    if (ix == DKIX_ERROR) {
        goto done;
    }
    if (PyValue_IsNull(value)) {
        dict_value_keyerror(query);
        goto done;
    }
    if (mp->_ma_watcher_tag & DICT_WATCHER_MASK) {
        _PyDict_NotifyEvent(PyDict_EVENT_DELETED, mp, query, PyValue_NULL);
        ix = _Py_dict_lookup(mp, query, hash, &value);
        if (ix == DKIX_ERROR) {
            goto done;
        }
    }
    if (!PyValue_IsNull(value)) {
        delitem_common(mp, hash, ix, value);
    }
    status = 0;  /* A dict watcher may already have removed this key. */
done:
    PyValue_XCLOSE(query);
    return status;
}

int
(PyDict_DelValue)(PyObject *op, PyValueBorrow key)
{
    assert(PyValue_IsLanguageValue(key));
    if (!PyDict_Check(op)) {
        if (PyFrozenDict_Check(op)) {
            frozendict_does_not_support("deletion");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }
    PyValue query = PyValue_XDUP(key);
    Py_hash_t hash = _PyValue_Hash(query);
    int status = -1;
    if (hash != -1) {
        status = delvalue_knownhash_lock_held((PyDictObject *)op, query, hash);
    }
    else if (PyValue_IsObject(query)) {
        dict_unhashable_type(op, PyValue_AsKnownObjectBorrow(query));
    }
    PyValue_XCLOSE(query);
    return status;
}

int
PyDict_DelItem(PyObject *op, PyObject *key)
{
    assert(key != NULL);
    return PyDict_DelValue(op, PyValue_FromObjectBorrow(key));
}

int
_PyDict_DelItem_KnownHash_LockHeld(PyObject *op, PyObject *key, Py_hash_t hash)
{
    if (!PyDict_Check(op)) {
        if (PyFrozenDict_Check(op)) {
            frozendict_does_not_support("deletion");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }
    return delvalue_knownhash_lock_held((PyDictObject *)op,
                                        PyValue_FromObjectBorrow(key), hash);
}

int
_PyDict_DelItem_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(op);
    res = _PyDict_DelItem_KnownHash_LockHeld(op, key, hash);
    Py_END_CRITICAL_SECTION();
    return res;
}

static int
delitemif_lock_held(PyObject *op, PyValueBorrow key,
                    int (*predicate)(PyObject *value, void *arg), void *arg)
{
    PyDictObject *mp = _PyAnyDict_CAST(op);
    PyValue query = PyValue_DUP(key);
    PyValue selected = PyValue_NULL, selected_key = PyValue_NULL;
    PyObject *object = NULL;
    int status = -1;
    Py_hash_t hash = _PyValue_Hash(query);
    if (hash == -1) {
        goto done;
    }
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t ix = _Py_dict_lookup(mp, query, hash, &value);
    if (ix == DKIX_ERROR) {
        goto done;
    }
    if (PyValue_IsNull(value)) {
        status = 0;
        goto done;
    }
    selected = PyValue_DUP(value);
    selected_key = PyValue_DUP(dict_entry_key(mp, ix));
    PyDictKeysObject *keys = mp->ma_keys;
    object = _PyValue_AsObjectAt(selected, BOX_SITE_DICT);
    if (object == NULL) {
        goto done;
    }
    /* As upstream, the predicate must not reenter. */
    status = predicate(object, arg);
    Py_XDECREF(object);
    object = NULL;
    if (status < 0) {
        status = -1;
        goto done;
    }
    if (status == 0) {
        goto done;
    }
    status = 0;
    if (mp->_ma_watcher_tag & DICT_WATCHER_MASK) {
        _PyDict_NotifyEvent(PyDict_EVENT_DELETED, mp, query, PyValue_NULL);
        /* A dict watcher can run Python that mutates mp: if the selected
         * entry moved, look it up again. */
        if (!dict_selection_matches(mp, keys, ix, hash, selected_key, selected)) {
            ix = _Py_dict_lookup(mp, query, hash, &value);
            if (ix == DKIX_ERROR) {
                status = -1;
                goto done;
            }
            keys = mp->ma_keys;
        }
    }
    if (dict_selection_matches(mp, keys, ix, hash, selected_key, selected)) {
        PyValue_XCLOSE(selected_key);
        selected_key = PyValue_NULL;  /* Current table still owns the key. */
        delitem_common(mp, hash, ix, PyValue_Borrow(selected));
        status = 1;
    }
done:
    Py_XDECREF(object);
    PyValue_XCLOSE(selected);
    PyValue_XCLOSE(selected_key);
    PyValue_XCLOSE(query);
    return status;
}
/* This function promises that the predicate -> deletion sequence is atomic
 * (i.e. protected by the GIL or the per-dict mutex in free threaded builds),
 * assuming the predicate itself doesn't release the GIL (or cause re-entrancy
 * which would release the per-dict mutex)
 */
int
_PyDict_DelItemIfValue(PyObject *op, PyValueBorrow key,
                       int (*predicate)(PyObject *value, void *arg),
                       void *arg)
{
    assert(PyDict_Check(op));
    int res;
    Py_BEGIN_CRITICAL_SECTION(op);
    res = delitemif_lock_held(op, key, predicate, arg);
    Py_END_CRITICAL_SECTION();
    return res;
}

int
_PyDict_DelItemIf(PyObject *op, PyObject *key,
                  int (*predicate)(PyObject *value, void *arg),
                  void *arg)
{
    return _PyDict_DelItemIfValue(op, PyValue_FromObjectBorrow(key),
                                  predicate, arg);
}

int
_PyDict_ReplaceItemIf(PyObject *op, PyObject *key,
                      PyObject *expected, PyObject *replacement)
{
    assert(PyDict_Check(op));
    PyDictObject *mp = (PyDictObject *)op;
    PyValue query = PyValue_DUP(PyValue_FromObjectBorrow(key));
    PyValue expected_value = PyValue_DUP(PyValue_FromObjectBorrow(expected));
    PyValue new_value = PyValue_DUP(PyValue_FromObjectBorrow(replacement));
    int status = -1;
    Py_hash_t hash = _PyValue_Hash(query);
    if (hash == -1) {
        goto done;
    }
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    Py_ssize_t ix = _Py_dict_lookup(mp, query, hash, &value);
    if (ix == DKIX_ERROR) {
        goto done;
    }
    status = 0;
    if (!PyValue_IsNull(value) && PyValue_Is(value, expected_value)) {
        if (!PyValue_Is(value, new_value) &&
            (mp->_ma_watcher_tag & DICT_WATCHER_MASK)) {
            _PyDict_NotifyEvent(PyDict_EVENT_MODIFIED, mp, query, new_value);
            ix = _Py_dict_lookup(mp, query, hash, &value);
            if (ix == DKIX_ERROR) {
                status = -1;
                goto done;
            }
        }
        if (!PyValue_IsNull(value) && PyValue_Is(value, expected_value)) {
            replace_value(mp, query, ix, value, new_value);
            new_value = PyValue_NULL;
            status = 1;
        }
    }
done:
    PyValue_XCLOSE(new_value);
    PyValue_XCLOSE(expected_value);
    PyValue_XCLOSE(query);
    return status;
}

static void
clear_embedded_values(PyDictValues *values, Py_ssize_t nentries)
{
    PyValue refs[SHARED_KEYS_MAX_SIZE];
    assert(nentries <= SHARED_KEYS_MAX_SIZE);
    for (Py_ssize_t i = 0; i < nentries; i++) {
        refs[i] = values->values[i];
        FT_ATOMIC_STORE_PTR_RELEASE(values->values[i], PyValue_NULL);
    }
    FT_ATOMIC_STORE_UINT8_RELEASE(values->size, 0);
    for (Py_ssize_t i = 0; i < nentries; i++) {
        PyValue_XCLOSE(refs[i]);
    }
}

static void
clear_lock_held(PyObject *op)
{
    if (!PyDict_Check(op)) {
        return;
    }
    PyDictObject *mp = (PyDictObject *)op;
    assert(can_modify_dict(mp));

    PyDictKeysObject *oldkeys;
    PyDictValues *oldvalues;
    Py_ssize_t i, n;

    if (mp->ma_keys == Py_EMPTY_KEYS) {
        return;
    }
    /* Empty the dict... */
    _PyDict_NotifyEvent(PyDict_EVENT_CLEARED, mp, PyValue_NULL, PyValue_NULL);
    /* A dict watcher, and the unraisable hook that reports its error, can
     * run Python. Reacquire both storage pointers before inspecting or
     * detaching the current table. */
    oldkeys = mp->ma_keys;
    oldvalues = mp->ma_values;
    if (oldkeys == Py_EMPTY_KEYS) {
        return;
    }
    // We don't inc ref empty keys because they're immortal
    ensure_shared_on_resize(mp);
    STORE_USED(mp, 0);
    if (oldvalues == NULL) {
        set_keys(mp, Py_EMPTY_KEYS);
        assert(oldkeys->dk_refcnt == 1);
        dictkeys_decref(oldkeys, IS_DICT_SHARED(mp));
    }
    else if (oldvalues->embedded) {
        clear_embedded_values(oldvalues, oldkeys->dk_nentries);
    }
    else {
        set_values(mp, NULL);
        set_keys(mp, Py_EMPTY_KEYS);
        n = oldkeys->dk_nentries;
        for (i = 0; i < n; i++) {
            // a move: cleared below
            PyValue tmp = oldvalues->values[i];
            FT_ATOMIC_STORE_PTR_RELEASE(oldvalues->values[i], PyValue_NULL);
            PyValue_XCLOSE(tmp);
        }
        free_values(oldvalues, IS_DICT_SHARED(mp));
        dictkeys_decref(oldkeys, IS_DICT_SHARED(mp));
    }
    ASSERT_CONSISTENT(mp);
}

void
_PyDict_Clear_LockHeld(PyObject *op) {
    clear_lock_held(op);
}

void
PyDict_Clear(PyObject *op)
{
    Py_BEGIN_CRITICAL_SECTION(op);
    clear_lock_held(op);
    Py_END_CRITICAL_SECTION();
}

/* Find the entry at or after *ppos without changing ownership: on success
 * store the dict's own key, value and hash, advance *ppos past the entry and
 * return 1; return 0 at the end. */
static int
dict_next_stored(PyDictObject *mp, Py_ssize_t *ppos, PyValueBorrow *pkey,
                 PyValueBorrow *pvalue, Py_hash_t *phash)
{
    Py_ssize_t i = *ppos;
    PyValueBorrow key, value;
    Py_hash_t hash;
    if (_PyDict_HasSplitTable(mp)) {
        assert(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
        if (i < 0 || i >= mp->ma_used) {
            return 0;
        }
        int index = get_index_from_order(mp, i);
        value = PyValue_Borrow(mp->ma_values->values[index]);
        PyObject *key_object = LOAD_SHARED_KEY(
            DK_UNICODE_ENTRIES(mp->ma_keys)[index].me_key);
        key = PyValue_FromObjectBorrow(key_object);
        hash = unicode_get_hash(key_object);
    }
    else {
        Py_ssize_t n = mp->ma_keys->dk_nentries;
        if (i < 0 || i >= n) {
            return 0;
        }
        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(mp->ma_keys);
            while (i < n && PyValue_IsNull(entries[i].me_value)) {
                i++;
            }
            if (i >= n) {
                return 0;
            }
            key = PyValue_FromObjectBorrow(entries[i].me_key);
            value = PyValue_Borrow(entries[i].me_value);
            hash = unicode_get_hash(entries[i].me_key);
        }
        else {
            PyDictKeyEntry *entries = DK_ENTRIES(mp->ma_keys);
            while (i < n && PyValue_IsNull(entries[i].me_value)) {
                i++;
            }
            if (i >= n) {
                return 0;
            }
            key = PyValue_Borrow(entries[i].me_key);
            value = PyValue_Borrow(entries[i].me_value);
            hash = entries[i].me_hash;
        }
    }
    assert(PyValue_IsLanguageValue(key));
    assert(PyValue_IsLanguageValue(value));
    _PyValue_RecheckHome(key);
    _PyValue_RecheckHome(value);
    *pkey = key;
    *pvalue = value;
    *phash = hash;
    *ppos = i + 1;
    return 1;
}

int
_PyDict_NextValue(PyObject *op, Py_ssize_t *ppos, PyValue *pkey,
                  PyValue *pvalue, Py_hash_t *phash)
{
    assert(pkey == NULL || pvalue == NULL || pkey != pvalue);
    if (pkey != NULL) {
        *pkey = PyValue_NULL;
    }
    if (pvalue != NULL) {
        *pvalue = PyValue_NULL;
    }
    if (!PyAnyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }

    PyValueBorrow key, value;
    Py_hash_t hash;
    if (!dict_next_stored((PyDictObject *)op, ppos, &key, &value, &hash)) {
        return 0;
    }
    /* Capturing owners is pure: no table pointer crosses a callback. */
    if (pkey != NULL) {
        *pkey = PyValue_DUP(key);
    }
    if (pvalue != NULL) {
        *pvalue = PyValue_DUP(value);
    }
    if (phash != NULL) {
        *phash = hash;
    }
    return 1;
}

int
_PyDict_NextBorrowed(PyObject *op, Py_ssize_t *ppos, PyValueBorrow *pkey,
                     PyValueBorrow *pvalue)
{
    assert(PyAnyDict_Check(op));
    PyValueBorrow key, value;
    Py_hash_t hash;
    if (!dict_next_stored((PyDictObject *)op, ppos, &key, &value, &hash)) {
        return 0;
    }
    if (pkey != NULL) {
        *pkey = key;
    }
    if (pvalue != NULL) {
        *pvalue = value;
    }
    return 1;
}

/* A snapshot's value as a new reference to an object: the box argument is
   that of the entry that calls (THUNK, owned by its caller). */
static inline PyObject *
dict_exit_owned(PyValue value _PyBox_AT_PARAM)
{
    if (PyValue_IsNull(value)) {
        return NULL;
    }
    if (PyValue_IsInline(value)) {
        return _PyValue_AsObjectFor(value, at);
    }
    return PyValue_AsKnownObjectSteal(value);
}

/* The object form of _PyDict_NextValue(), the body of the two thunks
   below: owned object snapshots, boxed through the thunk exit. The box
   argument is that of the entry that calls (THUNK, owned by its caller). */
static int
dict_next_entry_ref(PyObject *op, Py_ssize_t *position,
                    PyObject **out_key, PyObject **out_value,
                    Py_hash_t *out_hash _PyBox_AT_PARAM)
{
    if (out_key != NULL) {
        *out_key = NULL;
    }
    if (out_value != NULL) {
        *out_value = NULL;
    }
    if (out_hash != NULL) {
        *out_hash = -1;
    }
    if (out_key != NULL && out_key == out_value) {
        PyErr_SetString(PyExc_ValueError, "owned dictionary outputs must be distinct");
        return -1;
    }
    if (!PyAnyDict_Check(op) || position == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    PyValue key = PyValue_NULL, value = PyValue_NULL;
    Py_hash_t hash = -1;
    int status = _PyDict_NextValue(op, position, out_key != NULL ? &key : NULL,
                                   out_value != NULL ? &value : NULL, &hash);
    PyObject *key_object = NULL, *value_object = NULL;
    if (status > 0 && out_key != NULL) {
        key_object = dict_exit_owned(key _PyBox_AT_ARG(at));
        key = PyValue_NULL;
        if (key_object == NULL) {
            status = -1;
        }
    }
    if (status > 0 && out_value != NULL) {
        value_object = dict_exit_owned(value _PyBox_AT_ARG(at));
        value = PyValue_NULL;
        if (value_object == NULL) {
            status = -1;
        }
    }
    PyValue_XCLOSE(key);
    PyValue_XCLOSE(value);
    if (status < 0) {
        Py_XDECREF(key_object);
        Py_XDECREF(value_object);
        return -1;
    }
    if (out_key != NULL) {
        *out_key = key_object;
    }
    if (out_value != NULL) {
        *out_value = value_object;
    }
    if (status > 0 && out_hash != NULL) {
        *out_hash = hash;
    }
    return status;
}

int
_PyDict_NextEntryRefKnownHash(PyObject *op, Py_ssize_t *position,
                              PyObject **key, PyObject **value,
                              Py_hash_t *hash)
{
    return dict_next_entry_ref(
        op, position, key, value, hash
        _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
}

int
PyDict_NextEntryRef(PyObject *op, Py_ssize_t *position,
                    PyObject **key, PyObject **value)
{
    return dict_next_entry_ref(
        op, position, key, value, NULL
        _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
}


/* Remove key from mp and give the removed value: as an owner in *value_out,
   or boxed in *object_out, boxed before the removal so that an allocation
   failure consumes no item. At most one of the outputs is non-NULL; each is
   set to NULL on a miss or an error. Return 1, 0 if absent, -1 on error. */
static int
dict_pop_knownhash(PyDictObject *mp, PyValueBorrow key, Py_hash_t hash,
                   PyValue *value_out, PyObject **object_out)
{
    assert(value_out == NULL || object_out == NULL);
    if (value_out != NULL) {
        *value_out = PyValue_NULL;
    }
    if (object_out != NULL) {
        *object_out = NULL;
    }
    assert(can_modify_dict(mp));
    PyValue query = PyValue_DUP(key);
    PyValue selected = PyValue_NULL, selected_key = PyValue_NULL;
    PyObject *object = NULL;
    int notified = 0, status = -1;
    for (;;) {
        PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
        Py_ssize_t ix = _Py_dict_lookup(mp, query, hash, &value);
        if (ix == DKIX_ERROR) {
            break;
        }
        if (PyValue_IsNull(value)) {
            status = 0;
            break;
        }
        selected = PyValue_DUP(value);
        selected_key = PyValue_DUP(dict_entry_key(mp, ix));
        PyDictKeysObject *keys = mp->ma_keys;
        if (object_out != NULL) {
            /* Box before removal: allocation failure consumes no item. */
            object = _PyValue_AsObjectAt(selected, BOX_SITE_DICT);
            if (object == NULL) {
                break;
            }
        }
        int matches = 1;
        if (!notified && (mp->_ma_watcher_tag & DICT_WATCHER_MASK)) {
            notified = 1;
            _PyDict_NotifyEvent(PyDict_EVENT_DELETED, mp, query, PyValue_NULL);
            /* A dict watcher can run Python that mutates mp: delete the
             * selected entry only if it is still in place, else look again. */
            matches = dict_selection_matches(mp, keys, ix, hash,
                                              selected_key, selected);
        }
        if (matches) {
            /* Drop only the extra pin while the current table still owns the
             * key, retaining original key-before-value finalizer ordering. */
            PyValue_XCLOSE(selected_key);
            selected_key = PyValue_NULL;
            delitem_common(mp, hash, ix, PyValue_Borrow(selected));
            status = 1;
            break;
        }
        Py_XDECREF(object);
        object = NULL;
        PyValue_XCLOSE(selected);
        selected = PyValue_NULL;
        PyValue_XCLOSE(selected_key);
        selected_key = PyValue_NULL;
    }
    if (status == 1 && value_out != NULL) {
        *value_out = selected;
        selected = PyValue_NULL;
    }
    PyValue_XCLOSE(selected);
    PyValue_XCLOSE(selected_key);
    PyValue_XCLOSE(query);
    if (status == 1 && object_out != NULL) {
        *object_out = object;
        object = NULL;
    }
    Py_XDECREF(object);
    return status;
}

/* Internal version of dict.pop(). */
int
_PyDict_Pop_KnownHash(PyDictObject *mp, PyObject *key, Py_hash_t hash,
                      PyObject **result)
{
    return dict_pop_knownhash(mp, PyValue_FromObjectBorrow(key), hash,
                              NULL, result);
}

static int
pop_lock_held(PyObject *op, PyValueBorrow key, PyValue *value_out,
              PyObject **object_out)
{
    if (value_out != NULL) {
        *value_out = PyValue_NULL;
    }
    if (object_out != NULL) {
        *object_out = NULL;
    }
    if (!PyDict_Check(op)) {
        if (PyFrozenDict_Check(op)) {
            frozendict_does_not_support("deletion");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }
    PyDictObject *dict = (PyDictObject *)op;
    assert(can_modify_dict(dict));

    if (dict->ma_used == 0) {
        return 0;
    }

    Py_hash_t hash = _PyValue_Hash(key);
    if (hash == -1) {
        if (PyValue_IsObject(key)) {
            dict_unhashable_type(op, PyValue_AsKnownObjectBorrow(key));
        }
        return -1;
    }
    return dict_pop_knownhash(dict, key, hash, value_out, object_out);
}

int
(PyDict_PopValue)(PyObject *op, PyValueBorrow key, PyValue *result)
{
    assert(PyValue_IsLanguageValue(key));
    int err;
    Py_BEGIN_CRITICAL_SECTION(op);
    err = pop_lock_held(op, key, result, NULL);
    Py_END_CRITICAL_SECTION();
    return err;
}

int
PyDict_Pop(PyObject *op, PyObject *key, PyObject **result)
{
    int err;
    Py_BEGIN_CRITICAL_SECTION(op);
    err = pop_lock_held(op, PyValue_FromObjectBorrow(key), NULL, result);
    Py_END_CRITICAL_SECTION();

    return err;
}


int
PyDict_PopValueString(PyObject *op, const char *key, PyValue *result)
{
    PyObject *key_obj = PyUnicode_FromString(key);
    if (key_obj == NULL) {
        if (result != NULL) {
            *result = PyValue_NULL;
        }
        return -1;
    }

    int res = PyDict_PopValue(op, _PyValue_FromObjectKnown(key_obj), result);
    Py_DECREF(key_obj);
    return res;
}

int
PyDict_PopString(PyObject *op, const char *key, PyObject **result)
{
    PyObject *key_obj = PyUnicode_FromString(key);
    if (key_obj == NULL) {
        if (result != NULL) {
            *result = NULL;
        }
        return -1;
    }

    int res;
    Py_BEGIN_CRITICAL_SECTION(op);
    res = pop_lock_held(op, _PyValue_FromObjectKnown(key_obj), NULL, result);
    Py_END_CRITICAL_SECTION();
    Py_DECREF(key_obj);
    return res;
}


static PyObject *
dict_pop_default(PyObject *dict, PyObject *key, PyObject *default_value)
{
    PyObject *result;
    int status;
    Py_BEGIN_CRITICAL_SECTION(dict);
    status = pop_lock_held(dict, PyValue_FromObjectBorrow(key), NULL, &result);
    Py_END_CRITICAL_SECTION();
    if (status == 0) {
        if (default_value != NULL) {
            return Py_NewRef(default_value);
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    return result;
}

PyObject *
_PyDict_Pop(PyObject *dict, PyObject *key, PyObject *default_value)
{
    return dict_pop_default(dict, key, default_value);
}

static PyDictObject *
dict_dict_fromkeys(PyDictObject *mp, PyObject *iterable, PyObject *value)
{
    PyValue default_value = PyValue_DUP(PyValue_FromObjectBorrow(value));
    uint8_t size = Py_MAX(estimate_log2_keysize(PyDict_GET_SIZE(iterable)),
                         DK_LOG_SIZE(mp->ma_keys));
    int unicode = DK_IS_UNICODE(((PyDictObject *)iterable)->ma_keys);
    if (dictresize(mp, size, unicode) < 0) {
        goto error;
    }
    Py_ssize_t pos = 0;
    PyValue key;
    Py_hash_t hash;
    int found;
    while ((found = _PyDict_NextValue(iterable, &pos, &key, NULL, &hash)) > 0) {
        if (insertdict(mp, key, hash, PyValue_DUP(default_value)) < 0) {
            goto error;
        }
    }
    if (found < 0) {
        goto error;
    }
    PyValue_XCLOSE(default_value);
    return mp;
error:
    PyValue_XCLOSE(default_value);
    Py_XDECREF((PyObject *)mp);
    return NULL;
}

static PyDictObject *
dict_set_fromkeys(PyDictObject *mp, PyObject *iterable, PyObject *value)
{
    PyValue default_value = PyValue_DUP(PyValue_FromObjectBorrow(value));
    uint8_t size = Py_MAX(estimate_log2_keysize(PySet_GET_SIZE(iterable)),
                         DK_LOG_SIZE(mp->ma_keys));
    if (dictresize(mp, size, 0) < 0) {
        goto error;
    }
    Py_ssize_t pos = 0;
    PyValue key;
    Py_hash_t hash;
    int found;
    while ((found = _PySet_NextValue(iterable, &pos, &key, &hash)) > 0) {
        if (insertdict(mp, key, hash, PyValue_DUP(default_value)) < 0) {
            goto error;
        }
    }
    if (found < 0) {
        goto error;
    }
    PyValue_XCLOSE(default_value);
    return mp;
error:
    PyValue_XCLOSE(default_value);
    Py_XDECREF((PyObject *)mp);
    return NULL;
}

/* Internal version of dict.from_keys().  It is subclass-friendly. */
PyObject *
_PyDict_FromKeys(PyObject *cls, PyObject *iterable, PyObject *value)
{
    PyObject *it = NULL;       /* iter(iterable) */
    PyObject *d;
    int need_copy = 0;

    if (cls == (PyObject*)&PyFrozenDict_Type) {
        // gh-151722: Create a frozendict which is not tracked by the GC.
        d = frozendict_new_untracked(&PyFrozenDict_Type);
    }
    else {
        // Dict subclass, or frozendict subclass which overrides
        // the constructor.
        d = _PyObject_CallNoArgs(cls);
    }
    if (d == NULL) {
        return NULL;
    }

    // gh-151722: If cls constructor returns a frozendict which is tracked by
    // the GC, create a frozendict copy which is not tracked by the GC.
    //
    // At the function exit, return cls(fd) where fd is a frozendict.
    //
    // Untracking the frozendict requires tracking again the frozendict on
    // error which is more complicated. It's easier to work on a copy.
    if (PyFrozenDict_Check(d) && _PyObject_GC_IS_TRACKED(d)) {
        need_copy = 1;

        PyObject *copy = frozendict_new_untracked(&PyFrozenDict_Type);
        if (copy == NULL) {
            goto Fail;
        }
        if (dict_merge(copy, d, 1, NULL) < 0) {
            Py_DECREF(copy);
            goto Fail;
        }
        Py_SETREF(d, copy);
    }
    if (PyFrozenDict_Check(d)) {
        assert(can_modify_dict((PyDictObject*)d));
    }

    if (PyDict_CheckExact(d)) {
        if (PyDict_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;

            Py_BEGIN_CRITICAL_SECTION2(d, iterable);
            d = (PyObject *)dict_dict_fromkeys(mp, iterable, value);
            Py_END_CRITICAL_SECTION2();
            goto Done;
        }
        else if (PyFrozenDict_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;

            Py_BEGIN_CRITICAL_SECTION(d);
            d = (PyObject *)dict_dict_fromkeys(mp, iterable, value);
            Py_END_CRITICAL_SECTION();
            goto Done;
        }
        else if (PyAnySet_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;

            Py_BEGIN_CRITICAL_SECTION2(d, iterable);
            d = (PyObject *)dict_set_fromkeys(mp, iterable, value);
            Py_END_CRITICAL_SECTION2();
            goto Done;
        }
    }
    else if (PyFrozenDict_CheckExact(d)) {
        if (PyDict_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;

            Py_BEGIN_CRITICAL_SECTION(iterable);
            d = (PyObject *)dict_dict_fromkeys(mp, iterable, value);
            Py_END_CRITICAL_SECTION();
            goto Done;
        }
        else if (PyFrozenDict_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;
            d = (PyObject *)dict_dict_fromkeys(mp, iterable, value);
            goto Done;
        }
        else if (PyAnySet_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;

            Py_BEGIN_CRITICAL_SECTION(iterable);
            d = (PyObject *)dict_set_fromkeys(mp, iterable, value);
            Py_END_CRITICAL_SECTION();
            goto Done;
        }
    }

    it = _PyValue_GetIter(PyValue_FromObjectBorrow(iterable));
    if (it == NULL){
        goto Fail;
    }

    if (PyDict_CheckExact(d)) {
        int status = 0;

        Py_BEGIN_CRITICAL_SECTION(d);
        while (1) {
            _PyStatusValue step = _PyValue_IterNext(it);
            if (step.status != PYGEN_NEXT) {
                status = step.status == PYGEN_ERROR ? -1 : 0;
                break;
            }
            status = setitem_take2_lock_held((PyDictObject *)d, step.value,
                                            PyValue_DUP(PyValue_FromObjectBorrow(value)));
            if (status < 0) {
                break;
            }
        }
        Py_END_CRITICAL_SECTION();

        if (status < 0) {
            goto Fail;
        }
    }
    else if (PyFrozenDict_Check(d)) {
        while (1) {
            _PyStatusValue step = _PyValue_IterNext(it);
            if (step.status == PYGEN_ERROR) {
                goto Fail;
            }
            if (step.status == PYGEN_RETURN) {
                break;
            }
            int status = _PyDict_SetValue_Take2((PyDictObject *)d, step.value,
                                               PyValue_DUP(PyValue_FromObjectBorrow(value)));
            if (status < 0) {
                goto Fail;
            }
        }
    }
    else {
        while (1) {
            _PyStatusValue step = _PyValue_IterNext(it);
            if (step.status == PYGEN_ERROR) {
                goto Fail;
            }
            if (step.status == PYGEN_RETURN) {
                break;
            }
            int status = _PyValue_SetItem(PyValue_FromObjectBorrow(d), step.value,
                                          PyValue_FromObjectBorrow(value));
            PyValue_CLOSE(step.value);
            if (status < 0) {
                goto Fail;
            }
        }
    }

    assert(!PyErr_Occurred());
    Py_DECREF(it);
    goto Done;

Fail:
    assert(PyErr_Occurred());
    Py_XDECREF(it);
    Py_DECREF(d);
    return NULL;

Done:
    if (d == NULL) {
        return NULL;
    }

    if (need_copy) {
        PyObject *copy = _PyObject_CallOneArg(cls, d);
        Py_SETREF(d, copy);
    }
    else if (!_PyObject_GC_IS_TRACKED(d)) {
        _PyObject_GC_TRACK(d);
    }
    return d;
}

/* Methods */

static void
dict_dealloc(PyObject *self)
{
    PyDictObject *mp = (PyDictObject *)self;
    _PyObject_ResurrectStart(self);
    _PyDict_NotifyEvent(PyDict_EVENT_DEALLOCATED, mp, PyValue_NULL, PyValue_NULL);
    if (_PyObject_ResurrectEnd(self)) {
        return;
    }
    PyDictValues *values = mp->ma_values;
    PyDictKeysObject *keys = mp->ma_keys;
    Py_ssize_t i, n;

    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(mp);
    mp->ma_values = NULL;
    mp->ma_keys = Py_EMPTY_KEYS;
    mp->ma_used = 0;
    if (values != NULL) {
        if (values->embedded == 0) {
            for (i = 0, n = values->capacity; i < n; i++) {
                // a move: cleared below
                PyValue value = values->values[i];
                values->values[i] = PyValue_NULL;
                PyValue_XCLOSE(value);
            }
            free_values(values, false);
        }
        dictkeys_decref(keys, false);
    }
    else if (keys != NULL) {
        assert(keys->dk_refcnt == 1 || keys == Py_EMPTY_KEYS);
        dictkeys_decref(keys, false);
    }
    if (Py_IS_TYPE(mp, &PyDict_Type)) {
        _Py_FREELIST_FREE(dicts, mp, Py_TYPE(mp)->tp_free);
    }
    else {
        Py_TYPE(mp)->tp_free((PyObject *)mp);
    }
}


static PyObject *
anydict_repr_impl(PyObject *self)
{
    int entered = Py_ReprEnter(self);
    if (entered != 0) {
        return entered > 0 ? PyUnicode_FromString("{...}") : NULL;
    }
    Py_INCREF(self);
    PyDictObject *mp = (PyDictObject *)self;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(
        1 + 4 + 6 * (mp->ma_used > 0 ? mp->ma_used - 1 : 0) + 1);
    PyValue key = PyValue_NULL, value = PyValue_NULL;
    PyObject *key_object = NULL, *value_object = NULL;
    if (writer == NULL || PyUnicodeWriter_WriteChar(writer, '{') < 0) {
        goto error;
    }
    Py_ssize_t pos = 0;
    int first = 1, found;
    while ((found = _PyDict_NextValue(self, &pos, &key, &value, NULL)) > 0) {
        key_object = _PyValue_AsObjectAt(key, BOX_SITE_REPR);
        if (key_object == NULL) {
            goto error;
        }
        value_object = _PyValue_AsObjectAt(value, BOX_SITE_REPR);
        if (value_object == NULL) {
            goto error;
        }
        if ((!first && PyUnicodeWriter_WriteASCII(writer, ", ", 2) < 0) ||
            PyUnicodeWriter_WriteRepr(writer, key_object) < 0 ||
            PyUnicodeWriter_WriteASCII(writer, ": ", 2) < 0 ||
            PyUnicodeWriter_WriteRepr(writer, value_object) < 0)
        {
            goto error;
        }
        first = 0;
        Py_XDECREF(key_object);
        key_object = NULL;
        Py_XDECREF(value_object);
        value_object = NULL;
        PyValue_XCLOSE(key);
        key = PyValue_NULL;
        PyValue_XCLOSE(value);
        value = PyValue_NULL;
    }
    if (found < 0 || PyUnicodeWriter_WriteChar(writer, '}') < 0) {
        goto error;
    }
    Py_ReprLeave(self);
    PyObject *result = PyUnicodeWriter_Finish(writer);
    Py_XDECREF(self);
    return result;
error:
    Py_ReprLeave(self);
    PyUnicodeWriter_Discard(writer);
    Py_XDECREF(key_object);
    Py_XDECREF(value_object);
    PyValue_XCLOSE(key);
    PyValue_XCLOSE(value);
    Py_XDECREF(self);
    return NULL;
}

/*[clinic input]
@critical_section
dict.__repr__

    self: self(type="PyObject *")
[clinic start generated code]*/

static PyObject *
dict_repr_impl(PyObject *self)
/*[clinic end generated code: output=059263af818a321d input=f9c423cbdf534674]*/
{
    ASSERT_DICT_LOCKED((PyDictObject *)self);
    return anydict_repr_impl(self);
}

static Py_ssize_t
dict_length(PyObject *self)
{
    return GET_USED(_PyAnyDict_CAST(self));
}

static Py_ssize_t
frozendict_length(PyObject *self)
{
    return _PyAnyDict_CAST(self)->ma_used;
}

static PyValue
dict_subscript_value(PyObject *self, PyValueBorrow key)
{
    PyValue value;
    int status = PyDict_GetValue(self, key, &value);
    if (status != 0) {
        return status > 0 ? value : PyValue_NULL;
    }
    if (!PyAnyDict_CheckExact(self)) {
        PyObject *missing = _PyObject_LookupSpecial(self, &_Py_ID(__missing__));
        if (missing != NULL) {
            PyValue result = _PyValue_CallOneArg(
                _PyThreadState_GET(), PyValue_FromObjectBorrow(missing), key);
            Py_DECREF(missing);
            return result;
        }
        if (PyErr_Occurred()) {
            return PyValue_NULL;
        }
    }
    _PyErr_SetKeyErrorValue(key);
    return PyValue_NULL;
}

static PyValue
dict_getitem_value(PyValueBorrow self, const PyValue *args,
                   Py_ssize_t Py_UNUSED(nargs), PyObject *Py_UNUSED(kwnames))
{
    return dict_subscript_value(PyValue_AsKnownObjectBorrow(self),
                                PyValue_Borrow(args[0]));
}

/* Identify the value method that corresponds to the two builtin dict slots. */
int
_PyDict_IsBuiltinGetItemDescriptor(PyObject *descr)
{
    if (!Py_IS_TYPE(descr, &PyMethodDescr_Type)) {
        return 0;
    }
    PyTypeObject *owner = PyDescr_TYPE(descr);
    return (owner == &PyDict_Type || owner == &PyFrozenDict_Type) &&
        ((PyMethodDescrObject *)descr)->d_method->ml_meth ==
            _PyCFunction_CAST(dict_getitem_value);
}

Py_VALUE_ENTRY PyObject *
_PyDict_Subscript(PyObject *self, PyObject *key)
{
    return _PyValue_THUNK_EXIT(dict_subscript_value(self, _Py_VALUE_ARG(key)),
                               BOX_SITE_DICT);
}

static int
dict_ass_subscript_value(PyObject *mp, PyValueBorrow key, PyValueBorrow value)
{
    if (PyValue_IsNull(value)) {
        return PyDict_DelValue(mp, key);
    }
    return PyDict_SetValue(mp, key, value);
}

Py_VALUE_ENTRY int
_PyDict_StoreSubscript(PyObject *mp, PyObject *v, PyObject *w)
{
    return dict_ass_subscript_value(mp, _Py_VALUE_ARG(v), _Py_VALUE_ARG(w));
}

static PyMappingMethods dict_as_mapping = {
    dict_length, /*mp_length*/
    _PyDict_Subscript, /*mp_subscript*/
    _PyDict_StoreSubscript, /*mp_ass_subscript*/
};

static PyObject *
dict_snapshot_values(PyObject *dict, int kind)
{
    if (dict == NULL || !PyAnyDict_Check(dict)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyDictObject *mp = (PyDictObject *)dict;
    PyObject *result;
    for (;;) {
        Py_ssize_t count = mp->ma_used;
        result = PyList_New(count);
        if (result == NULL) {
            return NULL;
        }
        if (kind == 2) {
            for (Py_ssize_t i = 0; i < count; i++) {
                PyObject *pair = PyTuple_New(2);
                if (pair == NULL) {
                    goto error;
                }
                PyList_SET_VALUE(result, i, PyValue_FromObjectSteal(pair));
            }
        }
        if (mp->ma_used != count) {
            /* As upstream: if the allocations changed the dict's size,
             * start over. */
            Py_DECREF(result);
            continue;
        }
        Py_ssize_t pos = 0, index = 0;
        PyValue key, value;
        int found;
        while ((found = _PyDict_NextValue(dict, &pos,
                            kind != 1 ? &key : NULL,
                            kind != 0 ? &value : NULL, NULL)) > 0)
        {
            assert(index < count);
            if (kind == 2) {
                PyValueBorrow pair_value = PyList_GET_VALUE(result, index);
                PyObject *pair = PyValue_AsKnownObjectBorrow(pair_value);
                PyTuple_SET_VALUE(pair, 0, key);
                PyTuple_SET_VALUE(pair, 1, value);
            }
            else {
                PyList_SET_VALUE(result, index, kind == 0 ? key : value);
            }
            index++;
        }
        if (found < 0) {
            goto error;
        }
        assert(index == count);
        return result;
    }
error:
    Py_DECREF(result);
    return NULL;
}

static PyObject *
keys_lock_held(PyObject *dict)
{
    ASSERT_DICT_LOCKED(dict);
    return dict_snapshot_values(dict, 0);
}

PyObject *
PyDict_Keys(PyObject *dict)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    res = keys_lock_held(dict);
    Py_END_CRITICAL_SECTION();

    return res;
}

static PyObject *
values_lock_held(PyObject *dict)
{
    ASSERT_DICT_LOCKED(dict);
    return dict_snapshot_values(dict, 1);
}

PyObject *
PyDict_Values(PyObject *dict)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    res = values_lock_held(dict);
    Py_END_CRITICAL_SECTION();
    return res;
}

static PyObject *
items_lock_held(PyObject *dict)
{
    ASSERT_DICT_LOCKED(dict);
    return dict_snapshot_values(dict, 2);
}

PyObject *
PyDict_Items(PyObject *dict)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    res = items_lock_held(dict);
    Py_END_CRITICAL_SECTION();

    return res;
}

/*[clinic input]
@permit_long_summary
@classmethod
dict.fromkeys
    iterable: object
    value: object=None
    /

Create a new dictionary with keys from iterable and values set to value.
[clinic start generated code]*/

static PyValue
dict_fromkeys_impl(PyTypeObject *type, PyValueBorrow iterable,
                   PyValueBorrow value)
/*[clinic end generated code: output=943e0896c77c1f7d input=3903715eb48b287e]*/
{
    PyObject *iterable_object = _PyValue_AsObjectAt(iterable, BOX_SITE_PARSER);
    if (iterable_object == NULL) {
        return PyValue_NULL;
    }
    PyObject *value_object = _PyValue_AsObjectAt(value, BOX_SITE_PARSER);
    if (value_object == NULL) {
        Py_DECREF(iterable_object);
        return PyValue_NULL;
    }
    PyObject *res = _PyDict_FromKeys((PyObject *)type, iterable_object,
                                     value_object);
    Py_DECREF(iterable_object);
    Py_DECREF(value_object);
    return _PyValue_FromObjectResult(res);
}

/* Single-arg dict update; used by dict_update_common and operators. */
static int
dict_update_arg(PyObject *self, PyObject *arg)
{
    if (PyAnyDict_CheckExact(arg)) {
        return dict_merge(self, arg, 1, NULL);
    }
    int has_keys = PyObject_HasAttrWithError(arg, &_Py_ID(keys));
    if (has_keys < 0) {
        return -1;
    }
    if (has_keys) {
        return dict_merge(self, arg, 1, NULL);
    }
    return dict_merge_from_seq2(self, arg, 1);
}

static int
dict_update_arg_value(PyObject *self, PyValueBorrow arg)
{
    if (PyValue_IsObject(arg)) {
        return dict_update_arg(self, PyValue_AsKnownObjectBorrow(arg));
    }
    /* Inline numbers have no keys method and are not iterable. */
    PyObject *iter = _PyValue_GetIter(arg);
    assert(iter == NULL);
    Py_XDECREF(iter);
    return -1;
}

static int
dict_update_common(PyObject *self, PyObject *args, PyObject *kwds,
                   const char *methname)
{
    PyObject *arg = NULL;
    int result = 0;

    if (!PyArg_UnpackTuple(args, methname, 0, 1, &arg)) {
        result = -1;
    }
    else if (arg != NULL) {
        result = dict_update_arg(self, arg);
    }

    if (result == 0 && kwds != NULL) {
        if (PyArg_ValidateKeywordArguments(kwds))
            result = dict_merge(self, kwds, 1, NULL);
        else
            result = -1;
    }
    return result;
}

/* Note: dict.update() uses the METH_VARARGS|METH_KEYWORDS calling convention.
   Using METH_FASTCALL|METH_KEYWORDS would make dict.update(**dict2) calls
   slower, see the issue #29312. */
static PyObject *
dict_update(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (dict_update_common(self, args, kwds, "update") != -1)
        Py_RETURN_NONE;
    return NULL;
}

/* Update unconditionally replaces existing items.
   Merge has a 3rd argument 'override'; if set, it acts like Update,
   otherwise it leaves existing items unchanged.

   PyDict_{Update,Merge} update/merge from a mapping object.

   PyDict_MergeFromSeq2 updates/merges from any iterable object
   producing iterable objects of length 2.
*/

static int
merge_from_seq2_lock_held(PyObject *d, PyObject *seq2, int override)
{
    PyObject *iterator = _PyValue_GetIter(PyValue_FromObjectBorrow(seq2));
    if (iterator == NULL) {
        return -1;
    }
    Py_ssize_t index = 0;
    int status = 0;
    for (;;) {
        _PyStatusValue step = _PyValue_IterNext(iterator);
        if (step.status != PYGEN_NEXT) {
            status = step.status == PYGEN_ERROR ? -1 : 0;
            break;
        }
        PyObject *pair = _PyValue_SequenceFast(step.value, "object is not iterable");
        if (pair == NULL) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                _PyErr_FormatNote("Cannot convert dictionary update sequence element #%zd to a sequence", index);
            }
            PyValue_CLOSE(step.value);
            status = -1;
            break;
        }
        Py_ssize_t size = PySequence_Fast_GET_SIZE(pair);
        if (size != 2) {
            PyErr_Format(PyExc_ValueError,
                         "dictionary update sequence element #%zd has length %zd; 2 is required",
                         index, size);
            Py_XDECREF(pair);
            PyValue_CLOSE(step.value);
            status = -1;
            break;
        }
        PyValue key = PyList_Check(pair) ? PyList_GetValue(pair, 0) : PyTuple_GetValue(pair, 0);
        PyValue value = PyList_Check(pair) ? PyList_GetValue(pair, 1) : PyTuple_GetValue(pair, 1);
        if (PyValue_IsNull(key) || PyValue_IsNull(value)) {
            status = -1;
        }
        else if (override) {
            status = _PyDict_SetValue_Take2((PyDictObject *)d, key, value);
            key = value = PyValue_NULL;
        }
        else {
            status = _PyDict_SetDefaultValue(d, key, value, NULL);
            if (status >= 0) {
                status = 0;
            }
        }
        PyValue_XCLOSE(value);
        PyValue_XCLOSE(key);
        Py_XDECREF(pair);
        PyValue_CLOSE(step.value);
        if (status < 0) {
            break;
        }
        index++;
    }
    Py_XDECREF(iterator);
    return status;
}

static int
dict_merge_from_seq2(PyObject *d, PyObject *seq2, int override)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(d);
    res = merge_from_seq2_lock_held(d, seq2, override);
    Py_END_CRITICAL_SECTION();

    return res;
}

int
PyDict_MergeFromSeq2(PyObject *d, PyObject *seq2, int override)
{
    assert(d != NULL);
    assert(seq2 != NULL);
    if (!PyDict_Check(d)) {
        if (PyFrozenDict_Check(d)) {
            frozendict_does_not_support("assignment");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }

    return dict_merge_from_seq2(d, seq2, override);
}

static int
dict_dict_merge(PyDictObject *mp, PyDictObject *other, int override, PyObject **dupkey)
{
    assert(can_modify_dict(mp));
    if (other == mp || other->ma_used == 0) {
        return 0;
    }
    if (mp->ma_used == 0) {
        PyDictKeysObject *keys = other->ma_keys;
        if (mp->ma_values == NULL && other->ma_values == NULL &&
            other->ma_used == keys->dk_nentries &&
            (DK_LOG_SIZE(keys) == PyDict_LOG_MINSIZE ||
             USABLE_FRACTION(DK_SIZE(keys) / 2) < other->ma_used)) {
            _PyDict_NotifyEvent(PyDict_EVENT_CLONED, mp,
                                PyValue_FromObjectBorrow((PyObject *)other), PyValue_NULL);
            /* A dict watcher can run Python that mutates either dict or
             * changes other's class: clone only if that did not happen. */
            if (mp->ma_used == 0 && mp->ma_values == NULL &&
                other->ma_values == NULL && other->ma_used != 0 &&
                Py_TYPE(other)->tp_iter == dict_iter) {
                PyDictKeysObject *clone = clone_combined_dict_keys(other);
                if (clone == NULL) {
                    return -1;
                }
                ensure_shared_on_resize(mp);
                dictkeys_decref(mp->ma_keys, IS_DICT_SHARED(mp));
                set_keys(mp, clone);
                STORE_USED(mp, other->ma_used);
                ASSERT_CONSISTENT(mp);
                return 0;
            }
        }
    }
    /* See above: a dict watcher can change other's class. */
    if (Py_TYPE(other)->tp_iter != dict_iter) {
        return dict_merge((PyObject *)mp, (PyObject *)other, override, dupkey);
    }
    if (USABLE_FRACTION(DK_SIZE(mp->ma_keys)) < other->ma_used) {
        if (dictresize(mp, estimate_log2_keysize(mp->ma_used + other->ma_used),
                       DK_IS_UNICODE(other->ma_keys)) < 0) {
            return -1;
        }
    }
    Py_ssize_t original_size = other->ma_used, position = 0;
    for (;;) {
        PyValue key = PyValue_NULL, value = PyValue_NULL;
        Py_hash_t hash;
        int next = _PyDict_NextValue((PyObject *)other, &position, &key, &value, &hash);
        if (next <= 0) {
            return next < 0 ? -1 : 0;
        }
        int status = 0;
        if (override == 1) {
            status = _PyDict_SetValue_Take2_KnownHash(mp, key, value, hash);
            key = value = PyValue_NULL;
        }
        else if (override == 0) {
            status = dict_setdefault_value_lock_held(mp, key, value, hash, NULL);
            if (status >= 0) {
                status = 0;
            }
        }
        else {
            PyValue found = PyValue_NULL;
            int present = _PyDict_GetValue_KnownHash((PyObject *)mp, key, hash, &found);
            PyValue_XCLOSE(found);
            if (present < 0) {
                status = -1;
            }
            else if (present) {
                if (dupkey != NULL) {
                    *dupkey = _PyValue_AsObjectAt(key, BOX_SITE_DICT);
                    status = *dupkey == NULL ? -1 : -2;
                }
            }
            else {
                status = _PyDict_SetValue_Take2_KnownHash(mp, key, value, hash);
                key = value = PyValue_NULL;
            }
        }
        PyValue_XCLOSE(value);
        PyValue_XCLOSE(key);
        if (status != 0) {
            return status;
        }
        if (original_size != other->ma_used) {
            PyErr_SetString(PyExc_RuntimeError, "dict mutated during update");
            return -1;
        }
    }
}

static int
dict_merge(PyObject *a, PyObject *b, int override, PyObject **dupkey)
{
    assert(a != NULL && b != NULL);
    assert(0 <= override && override <= 2);
    PyDictObject *mp = _PyAnyDict_CAST(a);
    if (PyAnyDict_Check(b) && Py_TYPE(b)->tp_iter == dict_iter) {
        int status;
        Py_BEGIN_CRITICAL_SECTION2(a, b);
        status = dict_dict_merge(mp, (PyDictObject *)b, override, dupkey);
        Py_END_CRITICAL_SECTION2();
        return status;
    }
    PyObject *keys = _PyValue_MappingKeys(PyValue_FromObjectBorrow(b));
    if (keys == NULL) {
        return -1;
    }
    PyObject *iterator = _PyValue_GetIter(PyValue_FromObjectBorrow(keys));
    Py_XDECREF(keys);
    if (iterator == NULL) {
        return -1;
    }
    int status = 0;
    for (;;) {
        PyObject *key = PyIter_Next(iterator);
        if (key == NULL) {
            status = PyErr_Occurred() ? -1 : 0;
            break;
        }
        PyValue query = PyValue_DUP(PyValue_FromObjectBorrow(key));
        PyValue found = PyValue_NULL;
        PyValue value = PyValue_NULL;
        Py_hash_t hash = -1;
        int skip = 0;
        if (override != 1) {
            hash = _PyValue_Hash(query);
            if (hash == -1 && PyValue_IsObject(query)) {
                dict_unhashable_type(a, key);
            }
            int present = hash == -1 ? -1 :
                _PyDict_GetValue_KnownHash(a, query, hash, &found);
            PyValue_XCLOSE(found);
            found = PyValue_NULL;
            if (present < 0) {
                status = -1;
            }
            else if (present) {
                skip = 1;
                if (override == 2 && dupkey != NULL) {
                    *dupkey = Py_NewRef(key);
                    status = -2;
                }
            }
        }
        if (status == 0 && !skip) {
            value = _PyValue_GetItem(PyValue_FromObjectBorrow(b),
                                    PyValue_FromObjectBorrow(key));
            if (PyValue_IsNull(value)) {
                status = -1;
            }
            else {
                /* Unconditional mapping update historically hashes after
                 * __getitem__; conditional modes retain their earlier hash. */
                if (override == 1) {
                    hash = _PyValue_Hash(query);
                }
                if (hash == -1) {
                    if (override == 1 && PyValue_IsObject(query)) {
                        dict_unhashable_type(a, key);
                    }
                    status = -1;
                }
                else if (override == 1) {
                    status = _PyDict_SetValue_Take2_KnownHash(mp,
                        PyValue_DUP(query), PyValue_DUP(value), hash);
                }
                else {
                    /* __getitem__ can populate the target. Re-probe using
                     * conditional insertion, never an unconditional Set. */
                    status = dict_setdefault_value_lock_held(mp, query,
                        value, hash, NULL);
                    if (status == 1 && override == 2 && dupkey != NULL) {
                        *dupkey = Py_NewRef(key);
                        status = -2;
                    }
                    else if (status >= 0) {
                        status = 0;
                    }
                }
            }
        }
        PyValue_XCLOSE(value);
        PyValue_XCLOSE(query);
        Py_XDECREF(key);
        if (status != 0) {
            break;
        }
    }
    Py_XDECREF(iterator);
    return status;
}

static int
dict_merge_api(PyObject *a, PyObject *b, int override, PyObject **dupkey)
{
    /* We accept for the argument either a concrete dictionary object,
     * or an abstract "mapping" object.  For the former, we can do
     * things quite efficiently.  For the latter, we only require that
     * PyMapping_Keys() and PyObject_GetItem() be supported.
     */
    if (a == NULL || !PyDict_Check(a) || b == NULL) {
        if (a != NULL && PyFrozenDict_Check(a)) {
            frozendict_does_not_support("assignment");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }

    int res = dict_merge(a, b, override, dupkey);
    assert(_PyObject_GC_IS_TRACKED(a));
    return res;
}

int
PyDict_Update(PyObject *a, PyObject *b)
{
    return dict_merge_api(a, b, 1, NULL);
}

int
PyDict_Merge(PyObject *a, PyObject *b, int override)
{
    /* XXX Deprecate override not in (0, 1). */
    return dict_merge_api(a, b, override != 0, NULL);
}

int
_PyDict_MergeUniq(PyObject *a, PyObject *b, PyObject **dupkey)
{
    return dict_merge_api(a, b, 2, dupkey);
}

/*[clinic input]
dict.copy

Return a shallow copy of the dict.
[clinic start generated code]*/

static PyValue
dict_copy_impl(PyDictObject *self)
/*[clinic end generated code: output=631fd9d7bdc37042 input=73935f042b639de4]*/
{
    return _PyValue_FromObjectResult(PyDict_Copy((PyObject *)self));
}

/* Duplicate a stable source into already allocated detached storage. */
static void
copy_values(PyDictValues *destination, const PyDictValues *source)
{
    /* Both arrays are stable here: this pure copy allocates and closes nothing.
     * Every destination value acquires its own canonical strong owner. */
    assert(destination->capacity >= source->capacity);
    destination->size = source->size;
    _PyDictValues_SetShape(destination, _Py_SHAPE_NONE);
    memcpy(get_insertion_order_array(destination),
           get_insertion_order_array((PyDictValues *)source), source->size);
    for (size_t i = 0; i < source->capacity; i++) {
        PyValueBorrow value = PyValue_Borrow(source->values[i]);
        destination->values[i] = PyValue_IsNull(value) ? PyValue_NULL :
            PyValue_DUP(value);
        _PyValue_CheckHome(destination->values[i]);
    }
}

static PyObject *
copy_lock_held_untracked(PyObject *o, int as_frozendict)
{
    PyDictObject *mp = (PyDictObject *)o;
    PyObject *copy = as_frozendict ?
        frozendict_new_untracked(&PyFrozenDict_Type) :
        dict_new_untracked(&PyDict_Type);
    if (copy == NULL) {
        return NULL;
    }
    if (mp->ma_used == 0) {
        return copy;
    }
    PyDictObject *result = (PyDictObject *)copy;
    if (_PyDict_HasSplitTable(mp)) {
        PyDictValues *values = new_values(mp->ma_values->capacity);
        if (values == NULL) {
            PyErr_NoMemory();
            Py_DECREF(copy);
            return NULL;
        }
        dictkeys_incref(mp->ma_keys);
        copy_values(values, mp->ma_values);
        result->ma_keys = mp->ma_keys;
        result->ma_values = values;
        result->ma_used = mp->ma_used;
        ASSERT_CONSISTENT(result);
        return copy;
    }
    if (Py_TYPE(mp)->tp_iter == dict_iter &&
        mp->ma_used >= (mp->ma_keys->dk_nentries * 2) / 3) {
        PyDictKeysObject *keys = clone_combined_dict_keys(mp);
        if (keys == NULL) {
            Py_DECREF(copy);
            return NULL;
        }
        result->ma_keys = keys;
        result->ma_used = mp->ma_used;
        ASSERT_CONSISTENT(result);
        return copy;
    }
    if (dict_merge(copy, o, 1, NULL) < 0) {
        Py_DECREF(copy);
        return NULL;
    }
    return copy;
}

PyObject *
PyDict_Copy(PyObject *o)
{
    if (o == NULL || !PyDict_Check(o)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(o);
    res = copy_lock_held_untracked(o, 0);
    Py_END_CRITICAL_SECTION();
    if (res != NULL) {
        _PyObject_GC_TRACK(res);
    }
    return res;
}

// Similar to PyDict_Copy(), but return a frozendict if the argument
// is a frozendict.
static PyObject *
anydict_copy_untracked(PyObject *o)
{
    assert(PyAnyDict_Check(o));

    PyObject *res;
    if (PyFrozenDict_Check(o)) {
        res = copy_lock_held_untracked(o, 1);
    }
    else {
        Py_BEGIN_CRITICAL_SECTION(o);
        res = copy_lock_held_untracked(o, 0);
        Py_END_CRITICAL_SECTION();
    }
    return res;
}

// Similar to PyDict_Copy(), but accept also frozendict:
// convert frozendict to a new dict.
PyObject*
_PyDict_CopyAsDict(PyObject *o)
{
    assert(PyAnyDict_Check(o));

    PyObject *res;
    if (PyFrozenDict_Check(o)) {
        res = copy_lock_held_untracked(o, 0);
    }
    else {
        Py_BEGIN_CRITICAL_SECTION(o);
        res = copy_lock_held_untracked(o, 0);
        Py_END_CRITICAL_SECTION();
    }
    if (res != NULL) {
        _PyObject_GC_TRACK(res);
    }
    return res;
}

Py_ssize_t
PyDict_Size(PyObject *mp)
{
    if (mp == NULL || !PyAnyDict_Check(mp)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return GET_USED((PyDictObject *)mp);
}

/* Return 1 if dicts equal, 0 if not, -1 if error.
 * Gets out as soon as any difference is detected.
 * Uses only Py_EQ comparison.
 */
static int
dict_equal_lock_held(PyDictObject *a, PyDictObject *b)
{
    if (a->ma_used != b->ma_used) {
        return 0;
    }
    Py_ssize_t pos = 0;
    PyValue key, left;
    Py_hash_t hash;
    int found;
    while ((found = _PyDict_NextValue((PyObject *)a, &pos, &key, &left, &hash)) > 0) {
        PyValue right = PyValue_NULL;
        int present = _PyDict_GetValue_KnownHash((PyObject *)b, key, hash, &right);
        int equal = present > 0 ? _PyValue_RichCompareBool(left, right, Py_EQ) : present;
        PyValue_XCLOSE(key);
        PyValue_XCLOSE(left);
        PyValue_XCLOSE(right);
        if (equal <= 0) {
            return equal;
        }
    }
    return found < 0 ? -1 : 1;
}

static int
dict_equal(PyDictObject *a, PyDictObject *b)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION2(a, b);
    res = dict_equal_lock_held(a, b);
    Py_END_CRITICAL_SECTION2();

    return res;
}

static PyObject *
dict_richcompare(PyObject *v, PyObject *w, int op)
{
    int cmp;
    PyObject *res;

    if (!PyAnyDict_Check(v) || !PyAnyDict_Check(w)) {
        res = Py_NotImplemented;
    }
    else if (op == Py_EQ || op == Py_NE) {
        cmp = dict_equal((PyDictObject *)v, (PyDictObject *)w);
        if (cmp < 0)
            return NULL;
        res = (cmp == (op == Py_EQ)) ? Py_True : Py_False;
    }
    else
        res = Py_NotImplemented;
    return Py_NewRef(res);
}

/*[clinic input]

@coexist
dict.__contains__

  key: object
  /

True if the dictionary has the specified key, else False.
[clinic start generated code]*/

static PyValue
dict___contains___impl(PyDictObject *self, PyValueBorrow key)
/*[clinic end generated code: output=bd89a53d942697b8 input=fe1cb42ad831e820]*/
{
    int contains = PyDict_ContainsValue((PyObject *)self, key);
    if (contains < 0) {
        return PyValue_NULL;
    }
    if (contains) {
        return PyValue_True;
    }
    return PyValue_False;
}

/*[clinic input]
dict.get

    key: object
    default: object = None
    /

Return the value for key if key is in the dictionary, else default.
[clinic start generated code]*/

static PyValue
dict_get_impl(PyDictObject *self, PyValueBorrow key,
              PyValueBorrow default_value)
/*[clinic end generated code: output=9d509cf2f3d049dd input=279ddb5790b6b107]*/
{
    PyValue v;
    int status = PyDict_GetValue((PyObject *)self, key, &v);
    if (status < 0) {
        return PyValue_NULL;
    }
    if (status > 0) {
        return v;
    }
    return PyValue_DUP(default_value);
}

static int
(dict_setdefault_value_lock_held)(PyDictObject *mp, PyValueBorrow key_arg,
                                  PyValueBorrow value_arg, Py_hash_t hash,
                                  PyValue *result)
{
    if (result != NULL) {
        *result = PyValue_NULL;
    }
    PyValue key = PyValue_DUP(key_arg);
    PyValue value = PyValue_DUP(value_arg);
    PyValue output = PyValue_NULL;
    int notified = 0, status = -1;
    for (;;) {
        PyValueBorrow found = PyValue_Borrow(PyValue_NULL);
        Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &found);
        if (ix == DKIX_ERROR) {
            break;
        }
        if (!PyValue_IsNull(found)) {
            output = result == NULL ? PyValue_NULL : PyValue_DUP(found);
            status = 1;
            break;
        }
        if (_PyDict_HasSplitTable(mp) && value_is_exact_unicode(key)) {
            if (ix == DKIX_EMPTY) {
                ix = insert_split_key(mp->ma_keys, PyValue_AsKnownObjectBorrow(key), hash);
                if (ix == DKIX_ERROR) {
                    break;
                }
                if (ix >= 0) {
                    /* insert_split_key() notifies type watchers, which can
                     * run Python: look the key up again. */
                    continue;
                }
                /* No room in the shared keys. The watchers may have run
                 * before insert_split_key() found that: look the key up
                 * again in the combined table. */
                if (insertion_resize(mp, 1) < 0) {
                    break;
                }
                continue;
            }
        }
        else if (_PyDict_HasSplitTable(mp) || mp->ma_keys->dk_usable <= 0 ||
                 (DK_IS_UNICODE(mp->ma_keys) && !value_is_exact_unicode(key))) {
            if (insertion_resize(mp, value_is_exact_unicode(key)) < 0) {
                break;
            }
        }
        if (!notified) {
            notified = 1;
            if (mp->_ma_watcher_tag & DICT_WATCHER_MASK) {
                /* A dict watcher can run Python that mutates mp: look the
                 * key up again. */
                _PyDict_NotifyEvent(PyDict_EVENT_ADDED, mp, key, value);
                continue;
            }
        }
        output = result == NULL ? PyValue_NULL : PyValue_DUP(value);
        if (_PyDict_HasSplitTable(mp)) {
            _PyValue_CheckHome(value);
            STORE_SPLIT_VALUE(mp, ix, value);
            value = PyValue_NULL;
            _PyDictValues_AddToInsertionOrder(mp->ma_values, ix);
            STORE_USED(mp, mp->ma_used + 1);
        }
        else {
            insert_combined_dict(mp, hash, key, value);
            key = value = PyValue_NULL;
        }
        status = 0;
        break;
    }
    PyValue_XCLOSE(value);
    PyValue_XCLOSE(key);
    if (status < 0) {
        PyValue_XCLOSE(output);
        return -1;
    }
    if (result != NULL) {
        *result = output;
    }
    return status;
}

int
(_PyDict_SetDefaultValue)(PyObject *op, PyValueBorrow key_in,
                          PyValueBorrow value_in, PyValue *result)
{
    if (result != NULL) {
        *result = PyValue_NULL;
    }
    assert(PyValue_IsLanguageValue(key_in) && PyValue_IsLanguageValue(value_in));
    if (!PyAnyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(can_modify_dict((PyDictObject *)op));
    PyValue key = PyValue_DUP(key_in);
    PyValue value = PyValue_DUP(value_in);
    Py_hash_t hash = _PyValue_Hash(key);
    int status = -1;
    if (hash != -1) {
        status = dict_setdefault_value_lock_held((PyDictObject *)op, key, value, hash, result);
    }
    else if (PyValue_IsObject(key)) {
        // Only an object can fail to hash.
        dict_unhashable_type(op, PyValue_AsKnownObjectBorrow(key));
    }
    PyValue_XCLOSE(value);
    PyValue_XCLOSE(key);
    return status;
}

static int
dict_setdefault_object_owned(PyObject *op, PyObject *key, PyObject *default_value,
                            PyObject **result _PyBox_AT_PARAM)
{
    if (result != NULL) {
        *result = NULL;
    }
    if (!PyDict_Check(op)) {
        if (PyFrozenDict_Check(op)) {
            frozendict_does_not_support("assignment");
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }
    PyValue query = PyValue_DUP(PyValue_FromObjectBorrow(key));
    PyValue fallback = PyValue_DUP(PyValue_FromObjectBorrow(default_value));
    if (result != NULL) {
        Py_INCREF(default_value);
    }
    PyObject *object = NULL;
    PyValue output = PyValue_NULL;
    int status = -1;
    Py_hash_t hash = _PyValue_Hash(query);
    if (hash == -1) {
        dict_unhashable_type(op, key);
    }
    else {
        status = dict_setdefault_value_lock_held((PyDictObject *)op, query, fallback,
                    hash, result == NULL ? NULL : &output);
    }
    if (status == 0 && result != NULL) {
        /* The inserted value is the supplied default object. */
        object = default_value;
        default_value = NULL;
    }
    else if (status == 1 && result != NULL) {
        object = _PyValue_AsObjectFor(output, at);
        if (object == NULL) {
            status = -1;
        }
    }
    PyValue_XCLOSE(output);
    PyValue_XCLOSE(fallback);
    PyValue_XCLOSE(query);
    if (result != NULL) {
        Py_XDECREF(default_value);
    }
    if (status < 0) {
        Py_XDECREF(object);
        return -1;
    }
    if (result != NULL) {
        *result = object;
    }
    return status;
}


int
PyDict_SetDefaultRef(PyObject *d, PyObject *key, PyObject *default_value,
                     PyObject **result)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(d);
    res = dict_setdefault_object_owned(
        d, key, default_value, result
        _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
    Py_END_CRITICAL_SECTION();
    return res;
}


/*[clinic input]
@critical_section
dict.setdefault

    key: object
    default: object = None
    /

Insert key with a value of default if key is not in the dictionary.

Return the value for key if key is in the dictionary, else default.
[clinic start generated code]*/

static PyValue
dict_setdefault_impl(PyDictObject *self, PyValueBorrow key,
                     PyValueBorrow default_value)
/*[clinic end generated code: output=12ff2eaedbe0f293 input=9237af9a0a224302]*/
{
    Py_hash_t hash = _PyValue_Hash(key);
    if (hash == -1) {
        // Only an object can fail to hash.
        if (PyValue_IsObject(key)) {
            dict_unhashable_type((PyObject *)self,
                                 PyValue_AsKnownObjectBorrow(key));
        }
        return PyValue_NULL;
    }
    PyValue val;
    if (dict_setdefault_value_lock_held(self, key, default_value, hash,
                                        &val) < 0)
    {
        return PyValue_NULL;
    }
    return val;
}


/*[clinic input]
dict.clear

Remove all items from the dict.
[clinic start generated code]*/

static PyValue
dict_clear_impl(PyDictObject *self)
/*[clinic end generated code: output=34e269c180128c75 input=0bf729baba97a4c2]*/
{
    PyDict_Clear((PyObject *)self);
    return PyValue_None;
}

/*[clinic input]
@permit_long_summary
dict.pop

    key: object
    default: object = NULL
    /

D.pop(k[,d]) -> v, remove specified key and return the corresponding value.

If the key is not found, return the default if given; otherwise,
raise a KeyError.
[clinic start generated code]*/

static PyValue
dict_pop_impl(PyDictObject *self, PyValueBorrow key,
              PyValueBorrow default_value)
/*[clinic end generated code: output=d94f479863ae624e input=d409c7eb2de67e38]*/
{
    PyValue result;
    int status = PyDict_PopValue((PyObject *)self, key, &result);
    if (status < 0) {
        return PyValue_NULL;
    }
    if (status == 0) {
        if (!PyValue_IsNull(default_value)) {
            return PyValue_DUP(default_value);
        }
        _PyErr_SetKeyErrorValue(key);
        return PyValue_NULL;
    }
    return result;
}

/*[clinic input]
@critical_section
dict.popitem

Remove and return a (key, value) pair as a 2-tuple.

Pairs are returned in LIFO (last-in, first-out) order.
Raises KeyError if the dict is empty.
[clinic start generated code]*/

static PyValue
dict_popitem_impl(PyDictObject *self)
/*[clinic end generated code: output=8ff069e526686e72 input=ef28b4da5f0f762e]*/
{
    assert(can_modify_dict(self));
    PyObject *result = PyTuple_New(2);
    if (result == NULL) {
        return PyValue_NULL;
    }
    int notified = 0;
    for (;;) {
        if (self->ma_used == 0) {
            PyErr_SetString(PyExc_KeyError, "popitem(): dictionary is empty");
            break;
        }
        if (_PyDict_HasSplitTable(self)) {
            if (dictresize(self, DK_LOG_SIZE(self->ma_keys), 1) < 0) {
                break;
            }
            continue;
        }
        PyDictKeysObject *keys = self->ma_keys;
        Py_ssize_t ix = keys->dk_nentries - 1;
        PyValue value;
        for (;;) {
            assert(ix >= 0);
            value = DK_IS_UNICODE(keys) ? DK_UNICODE_ENTRIES(keys)[ix].me_value :
                DK_ENTRIES(keys)[ix].me_value;
            if (!PyValue_IsNull(value)) {
                break;
            }
            ix--;
        }
        PyValue key = PyValue_DUP(dict_entry_key(self, ix));
        value = PyValue_DUP(value);
        Py_hash_t hash = DK_IS_UNICODE(keys) ?
            unicode_get_hash(DK_UNICODE_ENTRIES(keys)[ix].me_key) :
            DK_ENTRIES(keys)[ix].me_hash;
        if (!notified && (self->_ma_watcher_tag & DICT_WATCHER_MASK)) {
            notified = 1;
            _PyDict_NotifyEvent(PyDict_EVENT_DELETED, self, key, PyValue_NULL);
            /* A dict watcher can run Python that mutates the dict: if the
             * last entry changed, select again. */
            if (!dict_selection_matches(self, keys, ix, hash, key, value)) {
                PyValue_XCLOSE(value);
                PyValue_XCLOSE(key);
                continue;
            }
        }
        /* This block allocates/releases nothing. Tuple and dict become valid
         * together before the original table owners can run finalizers. */
        PyValue old_key, old_value;
        if (DK_IS_UNICODE(keys)) {
            PyDictUnicodeEntry *entry = &DK_UNICODE_ENTRIES(keys)[ix];
            old_key = dict_take_unicode_key(entry->me_key);
            old_value = entry->me_value;
            STORE_KEY(entry, NULL);
            STORE_VALUE(entry, PyValue_NULL);
        }
        else {
            PyDictKeyEntry *entry = &DK_ENTRIES(keys)[ix];
            old_key = entry->me_key;
            old_value = entry->me_value;
            STORE_KEY(entry, PyValue_NULL);
            STORE_VALUE(entry, PyValue_NULL);
            STORE_HASH(entry, -1);
        }
        Py_ssize_t hashpos = lookdict_index(keys, hash, ix);
        assert(hashpos >= 0);
        dictkeys_set_index(keys, hashpos, DKIX_DUMMY);
        keys->dk_version = 0;
        Py_ssize_t end = keys->dk_nentries;
        while (end > 0) {
            PyValue tail = DK_IS_UNICODE(keys) ?
                DK_UNICODE_ENTRIES(keys)[end - 1].me_value :
                DK_ENTRIES(keys)[end - 1].me_value;
            if (!PyValue_IsNull(tail)) {
                break;
            }
            end--;
        }
        STORE_KEYS_NENTRIES(keys, end);
        STORE_USED(self, self->ma_used - 1);
        PyTuple_SET_VALUE(result, 0, key);
        PyTuple_SET_VALUE(result, 1, value);
        PyValue_XCLOSE(old_key);
        PyValue_XCLOSE(old_value);
        return _PyValue_FromObjectResult(result);
    }
    Py_XDECREF(result);
    return PyValue_NULL;
}

static int
dict_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyDictObject *mp = (PyDictObject *)op;
    PyDictKeysObject *keys = mp->ma_keys;
    Py_ssize_t i, n = keys->dk_nentries;

    /* The keys of a unicode-keyed table are strings, which the cycle
       collector ignores, but the tracing collector must see them.  A split
       table's keys are shared and owned by the keys object; visiting them
       from each dict is harmless for both collectors. */
    if (DK_IS_UNICODE(keys)) {
        PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
        if (_PyDict_HasSplitTable(mp)) {
            for (i = 0; i < n; i++) {
                _Py_VISIT_ATOMIC(entries[i].me_key);
                _Py_VISIT_VALUE(mp->ma_values->values[i]);
            }
        }
        else {
            for (i = 0; i < n; i++) {
                _Py_VISIT_ATOMIC(entries[i].me_key);
                _Py_VISIT_VALUE(entries[i].me_value);
            }
        }
    }
    else {
        PyDictKeyEntry *entries = DK_ENTRIES(keys);
        for (i = 0; i < n; i++) {
            if (!PyValue_IsNull(entries[i].me_value)) {
                _Py_VISIT_VALUE(entries[i].me_value);
                _Py_VISIT_VALUE(entries[i].me_key);
            }
        }
    }
    return 0;
}

static int
dict_tp_clear(PyObject *op)
{
    PyDict_Clear(op);
    return 0;
}

static PyObject *dictiter_new(PyDictObject *, PyTypeObject *);

Py_ssize_t
_PyDict_SizeOf_LockHeld(PyDictObject *mp)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(mp);

    size_t res = _PyObject_SIZE(Py_TYPE(mp));
    if (_PyDict_HasSplitTable(mp)) {
        res += shared_keys_usable_size(mp->ma_keys) * sizeof(PyObject*);
    }
    /* If the dictionary is split, the keys portion is accounted-for
       in the type object. */
    if (mp->ma_keys->dk_refcnt == 1) {
        res += _PyDict_KeysSize(mp->ma_keys);
    }
    assert(res <= (size_t)PY_SSIZE_T_MAX);
    return (Py_ssize_t)res;
}

void
_PyDict_ClearKeysVersionLockHeld(PyObject *op)
{
    PyDictObject *mp = _PyAnyDict_CAST(op);
    assert(can_modify_dict(mp));

    FT_ATOMIC_STORE_UINT32_RELAXED(mp->ma_keys->dk_version, 0);
}

Py_ssize_t
_PyDict_SizeOf(PyDictObject *mp)
{
    Py_ssize_t res;
    Py_BEGIN_CRITICAL_SECTION(mp);
    res = _PyDict_SizeOf_LockHeld(mp);
    Py_END_CRITICAL_SECTION();

    return res;
}

size_t
_PyDict_KeysSize(PyDictKeysObject *keys)
{
    size_t es = (keys->dk_kind == DICT_KEYS_GENERAL
                 ? sizeof(PyDictKeyEntry) : sizeof(PyDictUnicodeEntry));
    size_t size = sizeof(PyDictKeysObject);
    size += (size_t)1 << keys->dk_log2_index_bytes;
    size += USABLE_FRACTION((size_t)DK_SIZE(keys)) * es;
    return size;
}

/*[clinic input]
dict.__sizeof__

Return the size of the dict in memory, in bytes.
[clinic start generated code]*/

static PyValue
dict___sizeof___impl(PyDictObject *self)
/*[clinic end generated code: output=221223be7d50b225 input=4fec4ddfc44a4d1a]*/
{
    return PyValue_FromInt64(_PyDict_SizeOf(self));
}

PyObject *
_PyDict_Or(PyObject *self, PyObject *other)
{
    if (!PyAnyDict_Check(self) || !PyAnyDict_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PyObject *new = anydict_copy_untracked(self);
    if (new == NULL) {
        return NULL;
    }
    if (dict_update_arg(new, other)) {
        Py_DECREF(new);
        return NULL;
    }
    _PyObject_GC_TRACK(new);
    return new;
}

static PyObject *
frozendict_or(PyObject *self, PyObject *other)
{
    if (PyFrozenDict_CheckExact(self)) {
        // frozendict() | frozendict(...) => frozendict(...)
        if (GET_USED((PyDictObject *)self) == 0
            && PyFrozenDict_CheckExact(other))
        {
            return Py_NewRef(other);
        }

        // frozendict(...) | frozendict() => frozendict(...)
        if (PyAnyDict_CheckExact(other)
            && GET_USED((PyDictObject *)other) == 0)
        {
            return Py_NewRef(self);
        }
    }

    return _PyDict_Or(self, other);
}


PyObject *
_PyDict_IOr(PyObject *self, PyObject *other)
{
    if (dict_update_arg(self, other)) {
        return NULL;
    }
    return Py_NewRef(self);
}

/* d | other has an object other only: NotImplemented for any other value. */
_Py_VALUE_NOTIMPL_BINARY(dict_or_value, _PyDict_Or)
_Py_VALUE_NOTIMPL_BINARY(frozendict_or_value, frozendict_or)

/* d |= other takes any iterable of pairs or mapping; an inline other is
   boxed to fail as a value that is not one. */
static PyValue
dict_ior_value(PyValueBorrow self, PyValueBorrow other)
{
    PyObject *other_object = _PyValue_AsObjectAt(other, BOX_SITE_DICT);
    if (other_object == NULL) {
        return PyValue_NULL;
    }
    PyObject *result = _PyDict_IOr(PyValue_AsKnownObjectBorrow(self),
                                   other_object);
    Py_DECREF(other_object);
    return _PyValue_FromObjectResult(result);
}

PyDoc_STRVAR(getitem__doc__,
"__getitem__($self, key, /)\n--\n\nReturn self[key].");

PyDoc_STRVAR(update__doc__,
"D.update([E, ]**F) -> None.  Update D from mapping/iterable E and F.\n\
If E is present and has a .keys() method, then does:  for k in E.keys(): D[k] = E[k]\n\
If E is present and lacks a .keys() method, then does:  for k, v in E: D[k] = v\n\
In either case, this is followed by: for k in F:  D[k] = F[k]");

/* Forward */

static PyMethodDef mapp_methods[] = {
    DICT___CONTAINS___METHODDEF
    {"__getitem__",     _PyCFunction_CAST(dict_getitem_value),
                        METH_VALUE | METH_O | METH_COEXIST,
     getitem__doc__},
    DICT___SIZEOF___METHODDEF
    DICT_GET_METHODDEF
    DICT_SETDEFAULT_METHODDEF
    DICT_POP_METHODDEF
    DICT_POPITEM_METHODDEF
    DICT_KEYS_METHODDEF
    DICT_ITEMS_METHODDEF
    DICT_VALUES_METHODDEF
    {"update",          _PyCFunction_CAST(dict_update), METH_VARARGS | METH_KEYWORDS,
     update__doc__},
    DICT_FROMKEYS_METHODDEF
    DICT_CLEAR_METHODDEF
    DICT_COPY_METHODDEF
    DICT___REVERSED___METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("dicts are generic over two types, signifying (respectively) the types of their keys and values")},
    {NULL,              NULL}   /* sentinel */
};

static int
dict_contains_value(PyObject *op, PyValueBorrow key)
{
    return PyDict_ContainsValue(op, key);
}

_Py_VALUE_THUNK_CONTAINS(dict_contains, dict_contains_value)

/* Return 1 if `key` is in dict `op`, 0 if not, and -1 on error. */
int
PyDict_Contains(PyObject *op, PyObject *key)
{
    if (!PyAnyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }

    return dict_contains_value(op, PyValue_FromObjectBorrow(key));
}

int
PyDict_ContainsString(PyObject *op, const char *key)
{
    PyObject *key_obj = PyUnicode_FromString(key);
    if (key_obj == NULL) {
        return -1;
    }
    int res = PyDict_ContainsValue(op, _PyValue_FromObjectKnown(key_obj));
    Py_DECREF(key_obj);
    return res;
}

/* Internal version of PyDict_Contains used when the hash value is already known */
int
_PyDict_Contains_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    PyValue query = PyValue_DUP(PyValue_FromObjectBorrow(key));
    PyValue value = PyValue_NULL;
    int status = _PyDict_GetValue_KnownHash(op, query, hash, &value);
    PyValue_XCLOSE(value);
    PyValue_XCLOSE(query);
    return status;
}

/* Hack to implement "key in dict" */
static PySequenceMethods dict_as_sequence = {
    0,                          /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    0,                          /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    dict_contains,              /* sq_contains */
    0,                          /* sq_inplace_concat */
    0,                          /* sq_inplace_repeat */
};

static PyNumberMethods dict_as_number = {
    .nb_or = _PyDict_Or,
    .nb_inplace_or = _PyDict_IOr,
};

static PyObject*
anydict_new_untracked(PyTypeObject *type)
{
    assert(type != NULL);
    // dict and frozendict subclasses must implement the GC protocol
    assert(_PyType_IS_GC(type));

    PyObject *self = _PyType_AllocNoTrack(type, 0);
    if (self == NULL) {
        return NULL;
    }
    PyDictObject *d = (PyDictObject *)self;

    d->ma_used = 0;
    d->_ma_watcher_tag = 0;
    // We don't inc ref empty keys because they're immortal
    assert((Py_EMPTY_KEYS)->dk_refcnt == _Py_DICT_IMMORTAL_INITIAL_REFCNT);
    d->ma_keys = Py_EMPTY_KEYS;
    d->ma_values = NULL;
    ASSERT_CONSISTENT(d);
    return self;
}

static PyObject*
dict_new_untracked(PyTypeObject *type)
{
    assert(PyObject_IsSubclass((PyObject*)type, (PyObject*)&PyDict_Type));

    return anydict_new_untracked(type);
}

static PyObject *
dict_new(PyTypeObject *type, PyObject *Py_UNUSED(args), PyObject *Py_UNUSED(kwds))
{
    /* tp_new ignores args/kwds; args/kwds are consumed by dict_init (tp_init). */
    PyObject *self = dict_new_untracked(type);
    if (self == NULL) {
        return NULL;
    }
    _PyObject_GC_TRACK(self);
    return self;
}

static int
dict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return dict_update_common(self, args, kwds, "dict");
}

static PyValue
dict_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
                size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_new == dict_new);
    assert(_PyType_CAST(type)->tp_init == dict_init);
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("dict", nargs, 0, 1)) {
        return PyValue_NULL;
    }

    PyObject *self = dict_new(_PyType_CAST(type), NULL, NULL);
    if (self == NULL) {
        return PyValue_NULL;
    }
    if (nargs == 1) {
        if (dict_update_arg_value(self, PyValue_Borrow(args[0])) < 0) {
            Py_DECREF(self);
            return PyValue_NULL;
        }
        args++;
    }
    if (kwnames != NULL) {
        for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(kwnames); i++) {
            PyObject *key = _PyTuple_GET_OBJECT(kwnames, i);  // Unicode metadata
            if (PyDict_SetValue(self, PyValue_FromObjectBorrow(key),
                                PyValue_Borrow(args[i])) < 0) {
                Py_DECREF(self);
                return PyValue_NULL;
            }
        }
    }
    return PyValue_FromObjectSteal(self);
}

static PyValue
frozendict_vectorcall(PyThreadState *tstate, PyObject *type, const PyValue *args,
                      size_t nargsf, PyObject *kwnames)
{
    assert(_PyType_CAST(type)->tp_new == frozendict_new);
    assert(_PyType_CAST(type)->tp_init == PyBaseObject_Type.tp_init);
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("frozendict", nargs, 0, 1)) {
        return PyValue_NULL;
    }

    if (nargs == 1 && kwnames == NULL
        && PyValue_IsObject(PyValue_Borrow(args[0]))
        && PyFrozenDict_CheckExact(PyValue_AsKnownObjectBorrow(PyValue_Borrow(args[0])))
        && Py_Is((PyTypeObject*)type, &PyFrozenDict_Type))
    {
        // frozendict(frozendict) returns the same object unmodified
        return PyValue_DUP(args[0]);
    }

    /* gh-151722: Keep the frozendict untracked until it is fully built,
       so a half-built object is never reachable from another thread (using the gc module). */
    PyObject *self = frozendict_new_untracked(_PyType_CAST(type));
    if (self == NULL) {
        return PyValue_NULL;
    }
    if (nargs == 1) {
        if (dict_update_arg_value(self, PyValue_Borrow(args[0])) < 0) {
            Py_DECREF(self);
            return PyValue_NULL;
        }
        args++;
    }
    if (kwnames != NULL) {
        for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(kwnames); i++) {
            PyObject *key = _PyTuple_GET_OBJECT(kwnames, i);  // Unicode metadata
            if (_PyDict_SetValue_Take2((PyDictObject *)self,
                                       PyValue_DUP(PyValue_FromObjectBorrow(key)),
                                       PyValue_DUP(PyValue_Borrow(args[i]))) < 0) {
                Py_DECREF(self);
                return PyValue_NULL;
            }
        }
    }

    _PyObject_GC_TRACK(self);
    return PyValue_FromObjectSteal(self);
}

static PyObject *
dict_iter(PyObject *self)
{
    PyDictObject *dict = (PyDictObject *)self;
    return dictiter_new(dict, &PyDictIterKey_Type);
}

PyDoc_STRVAR(dictionary_doc,
"dict() -> new empty dictionary\n"
"dict(mapping) -> new dictionary initialized from a mapping object's\n"
"    (key, value) pairs\n"
"dict(iterable) -> new dictionary initialized as if via:\n"
"    d = {}\n"
"    for k, v in iterable:\n"
"        d[k] = v\n"
"dict(**kwargs) -> new dictionary initialized with the name=value pairs\n"
"    in the keyword argument list.  For example:  dict(one=1, two=2)");

_Py_VALUE_NOTIMPL_RICHCMP(dict_richcompare_value, dict_richcompare)

PyTypeObject PyDict_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict",
    sizeof(PyDictObject),
    0,
    dict_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dict_repr,                                  /* tp_repr */
    &dict_as_number,                            /* tp_as_number */
    &dict_as_sequence,                          /* tp_as_sequence */
    &dict_as_mapping,                           /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DICT_SUBCLASS |
        _Py_TPFLAGS_MATCH_SELF | Py_TPFLAGS_MAPPING,  /* tp_flags */
    dictionary_doc,                             /* tp_doc */
    dict_traverse,                              /* tp_traverse */
    dict_tp_clear,                              /* tp_clear */
    dict_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    dict_iter,                                  /* tp_iter */
    0,                                          /* tp_iternext */
    mapp_methods,                               /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    dict_init,                                  /* tp_init */
    _PyType_AllocNoTrack,                       /* tp_alloc */
    dict_new,                                   /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    .tp_vectorcall = _PyValue_VectorcallThunk,
    .tp_value_vectorcall = (_Py_valuefunc)dict_vectorcall,
    .tp_version_tag = _Py_TYPE_VERSION_DICT,
    .tp_value = {
        .nb_or = _Py_VALUE_SLOT_INIT(nb_or, dict_or_value),
        .nb_inplace_or = _Py_VALUE_SLOT_INIT(nb_inplace_or, dict_ior_value),
        .subscript = _Py_VALUE_SLOT_INIT(subscript, dict_subscript_value),
        .ass_subscript = _Py_VALUE_SLOT_INIT(ass_subscript,
                                             dict_ass_subscript_value),
        .contains = _Py_VALUE_SLOT_INIT(contains, dict_contains_value),
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, dict_richcompare_value),
    },
};


int
PyDict_GetItemStringRef(PyObject *v, const char *key, PyObject **result)
{
    PyObject *key_obj = PyUnicode_FromString(key);
    if (key_obj == NULL) {
        *result = NULL;
        return -1;
    }
    int res;
    if (!PyAnyDict_Check(v)) {
        *result = NULL;
        PyErr_BadInternalCall();
        res = -1;
    }
    else {
        res = dict_getitemref_owned(
            v, key_obj, -1, result
            _PyBox_AT_ARG(_PyValue_THUNK_AT(BOX_SITE_DICT)));
    }
    Py_DECREF(key_obj);
    return res;
}

int
(PyDict_SetValueString)(PyObject *v, const char *key, PyValueBorrow item)
{
    PyObject *kv;
    int err;
    kv = PyUnicode_FromString(key);
    if (kv == NULL)
        return -1;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_InternImmortal(interp, &kv); /* XXX Should we really? */
    err = PyDict_SetValue(v, _PyValue_FromObjectKnown(kv), item);
    Py_DECREF(kv);
    return err;
}

int
PyDict_SetItemString(PyObject *v, const char *key, PyObject *item)
{
    assert(item != NULL);
    return PyDict_SetValueString(v, key, PyValue_FromObjectBorrow(item));
}

int
PyDict_DelItemString(PyObject *v, const char *key)
{
    PyObject *kv;
    int err;
    kv = PyUnicode_FromString(key);
    if (kv == NULL)
        return -1;
    err = PyDict_DelValue(v, _PyValue_FromObjectKnown(kv));
    Py_DECREF(kv);
    return err;
}

/* Dictionary iterator types */

typedef struct {
    PyObject_HEAD
    PyDictObject *di_dict; /* Set to NULL when iterator is exhausted */
    Py_ssize_t di_used;
    Py_ssize_t di_pos;
    PyObject* di_result; /* reusable result tuple for iteritems */
    Py_ssize_t len;
} dictiterobject;

static PyObject *
dictiter_new(PyDictObject *dict, PyTypeObject *itertype)
{
    Py_ssize_t used;
    dictiterobject *di;
    di = PyObject_GC_New(dictiterobject, itertype);
    if (di == NULL) {
        return NULL;
    }
    di->di_dict = (PyDictObject*)Py_NewRef(dict);
    used = GET_USED(dict);
    di->di_used = used;
    di->len = used;
    di->di_result = NULL;
    if (itertype == &PyDictRevIterKey_Type ||
         itertype == &PyDictRevIterItem_Type ||
         itertype == &PyDictRevIterValue_Type) {
        if (_PyDict_HasSplitTable(dict)) {
            di->di_pos = used - 1;
        }
        else {
            di->di_pos = load_keys_nentries(dict) - 1;
        }
    }
    else {
        di->di_pos = 0;
    }
    /* gh-152107: track before allocating di_result. A dictiter with a NULL
       di_result is a valid state for dictiter_traverse()/dictiter_dealloc(),
       so a failure of the allocation below can safely DECREF a tracked di. */
    _PyObject_GC_TRACK(di);
    if (itertype == &PyDictIterItem_Type ||
        itertype == &PyDictRevIterItem_Type) {
        di->di_result = _PyTuple_FromPairTakeValue(PyValue_PrepareOwned(Py_None), PyValue_PrepareOwned(Py_None));
        if (di->di_result == NULL) {
            Py_DECREF(di);
            return NULL;
        }
    }
    return (PyObject *)di;
}

static void
dictiter_dealloc(PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    _PyObject_GC_UNTRACK(di);
    Py_XDECREF(di->di_dict);
    Py_XDECREF(di->di_result);
    PyObject_GC_Del(di);
}

static int
dictiter_traverse(PyObject *self, visitproc visit, void *arg)
{
    dictiterobject *di = (dictiterobject *)self;
    Py_VISIT(di->di_dict);
    Py_VISIT(di->di_result);
    return 0;
}

static PyObject *
dictiter_len(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    dictiterobject *di = (dictiterobject *)self;
    Py_ssize_t len = 0;
    if (di->di_dict != NULL && di->di_used == GET_USED(di->di_dict))
        len = FT_ATOMIC_LOAD_SSIZE_RELAXED(di->len);
    return PyLong_FromSize_t(len);
}

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(list(it)).");

static PyObject *
dictiter_reduce(PyObject *di, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef dictiter_methods[] = {
    {"__length_hint__", dictiter_len,                   METH_NOARGS,
     length_hint_doc},
     {"__reduce__",     dictiter_reduce,                METH_NOARGS,
     reduce_doc},
    {NULL,              NULL}           /* sentinel */
};



static bool acquire_iter_result(PyObject *result);
static int dictiter_iternextitem_lock_held(PyDictObject *, PyObject *,
                                          PyValue *, PyValue *);

/* Advance di over d: return 1 with the next entry, 0 at the end (where di
 * releases d) or -1 with an exception set. */
static int
dictiter_capture(PyDictObject *d, dictiterobject *di, int reverse,
                  PyValue *out_key, PyValue *out_value)
{
    int status = 0;
    if (out_key != NULL) {
        *out_key = PyValue_NULL;
    }
    if (out_value != NULL) {
        *out_value = PyValue_NULL;
    }
    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1;
        return -1;
    }
    Py_ssize_t i = di->di_pos;
    PyValueBorrow key = PyValue_Borrow(PyValue_NULL);
    PyValueBorrow value = PyValue_Borrow(PyValue_NULL);
    if (_PyDict_HasSplitTable(d)) {
        if (i < 0 || i >= d->ma_used) {
            goto exhausted;
        }
        int index = get_index_from_order(d, i);
        key = PyValue_FromObjectBorrow(DK_UNICODE_ENTRIES(d->ma_keys)[index].me_key);
        value = PyValue_Borrow(d->ma_values->values[index]);
        assert(!PyValue_IsNull(value));
    }
    else {
        Py_ssize_t n = d->ma_keys->dk_nentries;
        while (i >= 0 && i < n) {
            if (DK_IS_UNICODE(d->ma_keys)) {
                PyDictUnicodeEntry *entry = &DK_UNICODE_ENTRIES(d->ma_keys)[i];
                value = PyValue_Borrow(entry->me_value);
                if (!PyValue_IsNull(value)) {
                    key = PyValue_FromObjectBorrow(entry->me_key);
                    break;
                }
            }
            else {
                PyDictKeyEntry *entry = &DK_ENTRIES(d->ma_keys)[i];
                value = PyValue_Borrow(entry->me_value);
                if (!PyValue_IsNull(value)) {
                    key = PyValue_Borrow(entry->me_key);
                    break;
                }
            }
            i += reverse ? -1 : 1;
        }
        if (i < 0 || i >= n) {
            goto exhausted;
        }
    }
    if (di->len == 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary keys changed during iteration");
        status = -1;
        goto exhausted;
    }
    if (out_key != NULL) {
        *out_key = PyValue_DUP(key);
    }
    if (out_value != NULL) {
        *out_value = PyValue_DUP(value);
    }
    di->di_pos = i + (reverse ? -1 : 1);
    di->len--;
    return 1;
exhausted:
    di->di_dict = NULL;
    Py_XDECREF((PyObject *)d);
    return status;
}

static PyValue
dictiter_pair(dictiterobject *di, PyValue key, PyValue value)
{
    PyObject *result = di->di_result;
    if (acquire_iter_result(result)) {
        PyTupleObject *pair = (PyTupleObject *)result;
        // a move: replaced below
        PyValue old_key = pair->ob_item[0];
        // a move: replaced below
        PyValue old_value = pair->ob_item[1];
        pair->ob_item[0] = key;
        pair->ob_item[1] = value;
        /* Hash and GC state are part of publication, before either release. */
        _PyTuple_Recycle(result);
        PyValue_XCLOSE(old_key);
        PyValue_XCLOSE(old_value);
    }
    else {
        result = PyTuple_New(2);
        if (result != NULL) {
            PyTuple_SET_VALUE(result, 0, key);
            PyTuple_SET_VALUE(result, 1, value);
        }
        else {
            PyValue_XCLOSE(key);
            PyValue_XCLOSE(value);
        }
    }
    if (result == NULL) {
        return PyValue_NULL;
    }
    return PyValue_FromObjectSteal(result);
}

/* The value slot of the six dict iterator types. The reversed iterators
 * run under the dict's critical section, as their object slot did. */
static _PyStatusValue
dictiter_next_value_impl(PyObject *self)
{
    int keys = Py_IS_TYPE(self, &PyDictIterKey_Type) ||
               Py_IS_TYPE(self, &PyDictRevIterKey_Type);
    int values = Py_IS_TYPE(self, &PyDictIterValue_Type) ||
                 Py_IS_TYPE(self, &PyDictRevIterValue_Type);
    int items = Py_IS_TYPE(self, &PyDictIterItem_Type) ||
                Py_IS_TYPE(self, &PyDictRevIterItem_Type);
    int reverse = Py_IS_TYPE(self, &PyDictRevIterKey_Type) ||
                  Py_IS_TYPE(self, &PyDictRevIterValue_Type) ||
                  Py_IS_TYPE(self, &PyDictRevIterItem_Type);
    if (!keys && !values && !items) {
        PyErr_BadInternalCall();
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    dictiterobject *di = (dictiterobject *)self;
    PyDictObject *d = di->di_dict;
    if (d == NULL) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    PyValue key = PyValue_NULL, value = PyValue_NULL;
    int found;
    if (items && !reverse) {
        found = dictiter_iternextitem_lock_held(d, self, &key, &value);
    }
    else {
        found = dictiter_capture(d, di, reverse,
                                keys || items ? &key : NULL,
                                values || items ? &value : NULL);
    }
    if (found < 0) {
        return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
    }
    if (found == 0) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    if (items) {
        /* dictiter_pair() returns NULL only when the pair allocation fails,
           with MemoryError set. */
        PyValue pair = dictiter_pair(di, key, value);
        if (PyValue_IsNull(pair)) {
            return (_PyStatusValue){PYGEN_ERROR, PyValue_NULL};
        }
        return (_PyStatusValue){PYGEN_NEXT, pair};
    }
    return (_PyStatusValue){PYGEN_NEXT, keys ? key : value};
}

static _PyStatusValue
dictiter_next_value(PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    PyDictObject *d = di->di_dict;
    if (d == NULL) {
        return (_PyStatusValue){PYGEN_RETURN, PyValue_NULL};
    }
    if (!(Py_IS_TYPE(self, &PyDictRevIterKey_Type) ||
          Py_IS_TYPE(self, &PyDictRevIterValue_Type) ||
          Py_IS_TYPE(self, &PyDictRevIterItem_Type))) {
        return dictiter_next_value_impl(self);
    }
    _PyStatusValue result;
    Py_BEGIN_CRITICAL_SECTION(d);
    result = dictiter_next_value_impl(self);
    Py_END_CRITICAL_SECTION();
    return result;
}

/* A failed box or pair allocation consumes the entry (see listiter_next()). */
_Py_VALUE_THUNK_ITERNEXT(dictiter_object_next, dictiter_next_value)

PyTypeObject PyDictIterKey_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_keyiterator",                         /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictiter_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    dictiter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    dictiter_object_next,                       /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
    .tp_value = {
        .iternext = _Py_VALUE_SLOT_INIT(iternext, dictiter_next_value),
    },
};


PyTypeObject PyDictIterValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_valueiterator",                       /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictiter_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    0,                                          /* tp_doc */
    dictiter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    dictiter_object_next,                       /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
    .tp_value = {
        .iternext = _Py_VALUE_SLOT_INIT(iternext, dictiter_next_value),
    },
};

static int
dictiter_iternextitem_lock_held(PyDictObject *d, PyObject *self,
                                PyValue *out_key, PyValue *out_value)
{
    return dictiter_capture(d, (dictiterobject *)self, 0, out_key, out_value);
}


static bool
acquire_iter_result(PyObject *result)
{
    if (_PyObject_IsUniquelyReferenced(result)) {
        Py_INCREF(result);
        return true;
    }
    return false;
}

PyTypeObject PyDictIterItem_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_itemiterator",                        /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictiter_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    dictiter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    dictiter_object_next,                       /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
    .tp_value = {
        .iternext = _Py_VALUE_SLOT_INIT(iternext, dictiter_next_value),
    },
};


/* dictreviter */

PyTypeObject PyDictRevIterKey_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reversekeyiterator",
    sizeof(dictiterobject),
    .tp_dealloc = dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = dictiter_object_next,
    .tp_value = {
        .iternext = _Py_VALUE_SLOT_INIT(iternext, dictiter_next_value),
    },
    .tp_methods = dictiter_methods
};


/*[clinic input]
dict.__reversed__

Return a reverse iterator over the dict keys.
[clinic start generated code]*/

static PyValue
dict___reversed___impl(PyDictObject *self)
/*[clinic end generated code: output=d26df897111a5a28 input=23210ef3477d8c4d]*/
{
    assert (PyAnyDict_Check(self));
    return _PyValue_FromObjectResult(dictiter_new(self, &PyDictRevIterKey_Type));
}

static PyObject *
dictiter_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    dictiterobject *di = (dictiterobject *)self;
    /* copy the iterator state */
    dictiterobject tmp = *di;
    Py_XINCREF(tmp.di_dict);
    PyObject *list = _PyValue_SequenceList(PyValue_FromObjectBorrow((PyObject*)&tmp));
    Py_XDECREF(tmp.di_dict);
    if (list == NULL) {
        return NULL;
    }
    return Py_BuildValue("N(N)", _PyEval_GetBuiltin(&_Py_ID(iter)), list);
}

PyTypeObject PyDictRevIterItem_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reverseitemiterator",
    sizeof(dictiterobject),
    .tp_dealloc = dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = dictiter_object_next,
    .tp_value = {
        .iternext = _Py_VALUE_SLOT_INIT(iternext, dictiter_next_value),
    },
    .tp_methods = dictiter_methods
};

PyTypeObject PyDictRevIterValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reversevalueiterator",
    sizeof(dictiterobject),
    .tp_dealloc = dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = dictiter_object_next,
    .tp_value = {
        .iternext = _Py_VALUE_SLOT_INIT(iternext, dictiter_next_value),
    },
    .tp_methods = dictiter_methods
};

/***********************************************/
/* View objects for keys(), items(), values(). */
/***********************************************/

/* The instance lay-out is the same for all three; but the type differs. */

static void
dictview_dealloc(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    _PyObject_GC_UNTRACK(dv);
    Py_XDECREF(dv->dv_dict);
    PyObject_GC_Del(dv);
}

static int
dictview_traverse(PyObject *self, visitproc visit, void *arg)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    Py_VISIT(dv->dv_dict);
    return 0;
}

static Py_ssize_t
dictview_len(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    Py_ssize_t len = 0;
    if (dv->dv_dict != NULL)
        len = GET_USED(dv->dv_dict);
    return len;
}

PyObject *
_PyDictView_New(PyObject *dict, PyTypeObject *type)
{
    _PyDictViewObject *dv;
    if (dict == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (!PyAnyDict_Check(dict)) {
        /* XXX Get rid of this restriction later */
        PyErr_Format(PyExc_TypeError,
                     "%s() requires a dict argument, not '%s'",
                     type->tp_name, Py_TYPE(dict)->tp_name);
        return NULL;
    }
    dv = PyObject_GC_New(_PyDictViewObject, type);
    if (dv == NULL)
        return NULL;
    dv->dv_dict = (PyDictObject *)Py_NewRef(dict);
    _PyObject_GC_TRACK(dv);
    return (PyObject *)dv;
}

static PyObject *
dictview_mapping(PyObject *view, void *Py_UNUSED(ignored)) {
    assert(view != NULL);
    assert(PyDictKeys_Check(view)
           || PyDictValues_Check(view)
           || PyDictItems_Check(view));
    PyObject *mapping = (PyObject *)((_PyDictViewObject *)view)->dv_dict;
    return PyDictProxy_New(mapping);
}

static PyGetSetDef dictview_getset[] = {
    {"mapping", dictview_mapping, NULL,
     PyDoc_STR("dictionary that this view refers to"), NULL},
    {0}
};

/* TODO(guido): The views objects are not complete:

 * support more set operations
 * support arbitrary mappings?
   - either these should be static or exported in dictobject.h
   - if public then they should probably be in builtins
*/

/* Return 1 if self is a subset of other, iterating over self;
   0 if not; -1 if an error occurred. */
static int
all_contained_in(PyObject *self, PyObject *other)
{
    PyObject *iter = _PyValue_GetIter(PyValue_FromObjectBorrow(self));
    int ok = 1;

    if (iter == NULL)
        return -1;
    for (;;) {
        PyObject *next = PyIter_Next(iter);
        if (next == NULL) {
            if (PyErr_Occurred())
                ok = -1;
            break;
        }
        ok = _PyValue_Contains(PyValue_FromObjectBorrow(other), PyValue_FromObjectBorrow(next));
        Py_DECREF(next);
        if (ok <= 0)
            break;
    }
    Py_DECREF(iter);
    return ok;
}

static PyObject *
dictview_richcompare(PyObject *self, PyObject *other, int op)
{
    Py_ssize_t len_self, len_other;
    int ok;
    PyObject *result;

    assert(self != NULL);
    assert(PyDictViewSet_Check(self));
    assert(other != NULL);

    if (!PyAnySet_Check(other) && !PyDictViewSet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;

    len_self = _PyValue_Size(PyValue_FromObjectBorrow(self));
    if (len_self < 0)
        return NULL;
    len_other = _PyValue_Size(PyValue_FromObjectBorrow(other));
    if (len_other < 0)
        return NULL;

    ok = 0;
    switch(op) {

    case Py_NE:
    case Py_EQ:
        if (len_self == len_other)
            ok = all_contained_in(self, other);
        if (op == Py_NE && ok >= 0)
            ok = !ok;
        break;

    case Py_LT:
        if (len_self < len_other)
            ok = all_contained_in(self, other);
        break;

      case Py_LE:
          if (len_self <= len_other)
              ok = all_contained_in(self, other);
          break;

    case Py_GT:
        if (len_self > len_other)
            ok = all_contained_in(other, self);
        break;

    case Py_GE:
        if (len_self >= len_other)
            ok = all_contained_in(other, self);
        break;

    }
    if (ok < 0)
        return NULL;
    result = ok ? Py_True : Py_False;
    return Py_NewRef(result);
}

static PyObject *
dictview_repr(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    PyObject *seq;
    PyObject *result = NULL;
    Py_ssize_t rc;

    rc = Py_ReprEnter((PyObject *)dv);
    if (rc != 0) {
        return rc > 0 ? PyUnicode_FromString("...") : NULL;
    }
    seq = _PyValue_SequenceList(_PyValue_FromObjectKnown((PyObject *)dv));
    if (seq == NULL) {
        goto Done;
    }
    result = PyUnicode_FromFormat("%s(%R)", Py_TYPE(dv)->tp_name, seq);
    Py_DECREF(seq);

Done:
    Py_ReprLeave((PyObject *)dv);
    return result;
}

/*** dict_keys ***/

static PyObject *
dictkeys_iter(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterKey_Type);
}

static int
dictkeys_contains_value(PyObject *self, PyValueBorrow obj)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL)
        return 0;
    return dict_contains_value((PyObject *)dv->dv_dict, obj);
}
_Py_VALUE_THUNK_CONTAINS(dictkeys_contains, dictkeys_contains_value)

static PySequenceMethods dictkeys_as_sequence = {
    dictview_len,                       /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    dictkeys_contains,                  /* sq_contains */
};

// Create a set object from dictviews object.
// Returns a new reference.
// This utility function is used by set operations.
static PyObject*
dictviews_to_set(PyObject *self)
{
    PyObject *left = self;
    if (PyDictKeys_Check(self)) {
        // PySet_New() has fast path for the dict object.
        PyObject *dict = (PyObject *)((_PyDictViewObject *)self)->dv_dict;
        if (PyAnyDict_CheckExact(dict)) {
            left = dict;
        }
    }
    return PySet_New(left);
}

static PyObject*
dictviews_sub(PyObject *self, PyObject *other)
{
    PyObject *result = dictviews_to_set(self);
    if (result == NULL) {
        return NULL;
    }

    PyObject *tmp = PyObject_CallMethodOneArg(
            result, &_Py_ID(difference_update), other);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

static int
dictitems_contains(PyObject *dv, PyObject *obj);

PyObject *
_PyDictView_Intersect(PyObject* self, PyObject *other)
{
    PyObject *result;
    PyObject *it;
    PyObject *key;
    Py_ssize_t len_self;
    int rv;
    objobjproc dict_contains;

    /* Python interpreter swaps parameters when dict view
       is on right side of & */
    if (!PyDictViewSet_Check(self)) {
        PyObject *tmp = other;
        other = self;
        self = tmp;
    }

    len_self = dictview_len(self);

    /* if other is a set and self is smaller than other,
       reuse set intersection logic */
    if (PySet_CheckExact(other) && len_self <= _PyValue_Size(PyValue_FromObjectBorrow(other))) {
        return PyObject_CallMethodObjArgs(
                other, &_Py_ID(intersection), self, NULL);
    }

    /* if other is another dict view, and it is bigger than self,
       swap them */
    if (PyDictViewSet_Check(other)) {
        Py_ssize_t len_other = dictview_len(other);
        if (len_other > len_self) {
            PyObject *tmp = other;
            other = self;
            self = tmp;
        }
    }

    /* at this point, two things should be true
       1. self is a dictview
       2. if other is a dictview then it is smaller than self */
    result = PySet_New(NULL);
    if (result == NULL)
        return NULL;

    it = _PyValue_GetIter(PyValue_FromObjectBorrow(other));
    if (it == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    if (PyDictKeys_Check(self)) {
        dict_contains = dictkeys_contains;
    }
    /* else PyDictItems_Check(self) */
    else {
        dict_contains = dictitems_contains;
    }

    while ((key = PyIter_Next(it)) != NULL) {
        rv = dict_contains(self, key);
        if (rv < 0) {
            goto error;
        }
        if (rv) {
            if (PySet_AddValue(result, PyValue_FromObjectBorrow(key))) {
                goto error;
            }
        }
        Py_DECREF(key);
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }
    return result;

error:
    Py_DECREF(it);
    Py_DECREF(result);
    Py_DECREF(key);
    return NULL;
}

static PyObject*
dictviews_or(PyObject* self, PyObject *other)
{
    PyObject *result = dictviews_to_set(self);
    if (result == NULL) {
        return NULL;
    }

    if (_PySet_Update(result, other) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

static PyObject *
dictitems_xor_lock_held(PyObject *d1, PyObject *d2)
{
    ASSERT_DICT_LOCKED(d1);
    ASSERT_DICT_LOCKED(d2);
    PyObject *temp_dict = copy_lock_held_untracked(d1, 0);
    if (temp_dict == NULL) {
        return NULL;
    }
    _PyObject_GC_TRACK(temp_dict);
    PyObject *result_set = PySet_New(NULL);
    if (result_set == NULL) {
        Py_XDECREF(temp_dict);
        return NULL;
    }
    Py_INCREF(d2);
    PyValue key = PyValue_NULL, val1 = PyValue_NULL, val2 = PyValue_NULL;
    Py_ssize_t pos = 0;
    Py_hash_t hash;
    int found;
    while ((found = _PyDict_NextValue(d2, &pos, &key, &val2, &hash)) > 0) {
        int present = _PyDict_GetValue_KnownHash(temp_dict, key, hash, &val1);
        if (present < 0) {
            goto error;
        }
        int to_delete = present ? _PyValue_RichCompareBool(val1, val2, Py_EQ) : 0;
        PyValue_XCLOSE(val1);
        val1 = PyValue_NULL;
        if (to_delete < 0) {
            goto error;
        }
        if (to_delete) {
            PyValue_XCLOSE(val2);
            val2 = PyValue_NULL;
            if (delvalue_knownhash_lock_held((PyDictObject *)temp_dict, key, hash) < 0) {
                goto error;
            }
            PyValue_XCLOSE(key);
            key = PyValue_NULL;
        }
        else {
            PyObject *pair = PyTuple_New(2);
            if (pair == NULL) {
                goto error;
            }
            PyTuple_SET_VALUE(pair, 0, key);
            PyTuple_SET_VALUE(pair, 1, val2);
            key = val2 = PyValue_NULL;
            if (_PySet_AddValueTakeRef((PySetObject *)result_set,
                                     PyValue_FromObjectSteal(pair)) < 0)
            {
                goto error;
            }
        }
    }
    if (found < 0) {
        goto error;
    }
    PyObject *remaining_pairs = PyObject_CallMethodNoArgs(temp_dict, &_Py_ID(items));
    if (remaining_pairs == NULL) {
        goto error;
    }
    int status = _PySet_Update(result_set, remaining_pairs);
    Py_XDECREF(remaining_pairs);
    if (status < 0) {
        goto error;
    }
    Py_XDECREF(temp_dict);
    Py_XDECREF(d2);
    return result_set;
error:
    PyValue_XCLOSE(key);
    PyValue_XCLOSE(val1);
    PyValue_XCLOSE(val2);
    Py_XDECREF(temp_dict);
    Py_XDECREF(result_set);
    Py_XDECREF(d2);
    return NULL;
}

static PyObject *
dictitems_xor(PyObject *self, PyObject *other)
{
    assert(PyDictItems_Check(self));
    assert(PyDictItems_Check(other));
    PyObject *d1 = (PyObject *)((_PyDictViewObject *)self)->dv_dict;
    PyObject *d2 = (PyObject *)((_PyDictViewObject *)other)->dv_dict;

    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION2(d1, d2);
    res = dictitems_xor_lock_held(d1, d2);
    Py_END_CRITICAL_SECTION2();

    return res;
}

static PyObject*
dictviews_xor(PyObject* self, PyObject *other)
{
    if (PyDictItems_Check(self) && PyDictItems_Check(other)) {
        return dictitems_xor(self, other);
    }
    PyObject *result = dictviews_to_set(self);
    if (result == NULL) {
        return NULL;
    }

    PyObject *tmp = PyObject_CallMethodOneArg(
            result, &_Py_ID(symmetric_difference_update), other);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

static PyNumberMethods dictviews_as_number = {
    0,                                  /*nb_add*/
    dictviews_sub,                      /*nb_subtract*/
    0,                                  /*nb_multiply*/
    0,                                  /*nb_remainder*/
    0,                                  /*nb_divmod*/
    0,                                  /*nb_power*/
    0,                                  /*nb_negative*/
    0,                                  /*nb_positive*/
    0,                                  /*nb_absolute*/
    0,                                  /*nb_bool*/
    0,                                  /*nb_invert*/
    0,                                  /*nb_lshift*/
    0,                                  /*nb_rshift*/
    _PyDictView_Intersect,              /*nb_and*/
    dictviews_xor,                      /*nb_xor*/
    dictviews_or,                       /*nb_or*/
};

static PyObject*
dictviews_isdisjoint(PyObject *self, PyObject *other)
{
    PyObject *it;
    PyObject *item = NULL;

    if (self == other) {
        if (dictview_len(self) == 0)
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    /* Iterate over the shorter object (only if other is a set,
     * because PySequence_Contains may be expensive otherwise): */
    if (PyAnySet_Check(other) || PyDictViewSet_Check(other)) {
        Py_ssize_t len_self = dictview_len(self);
        Py_ssize_t len_other = _PyValue_Size(PyValue_FromObjectBorrow(other));
        if (len_other == -1)
            return NULL;

        if ((len_other > len_self)) {
            PyObject *tmp = other;
            other = self;
            self = tmp;
        }
    }

    it = _PyValue_GetIter(PyValue_FromObjectBorrow(other));
    if (it == NULL)
        return NULL;

    while ((item = PyIter_Next(it)) != NULL) {
        int contains = _PyValue_Contains(PyValue_FromObjectBorrow(self), PyValue_FromObjectBorrow(item));
        Py_DECREF(item);
        if (contains == -1) {
            Py_DECREF(it);
            return NULL;
        }

        if (contains) {
            Py_DECREF(it);
            Py_RETURN_FALSE;
        }
    }
    Py_DECREF(it);
    if (PyErr_Occurred())
        return NULL; /* PyIter_Next raised an exception. */
    Py_RETURN_TRUE;
}

PyDoc_STRVAR(isdisjoint_doc,
"Return True if the view and the given iterable have a null intersection.");

static PyObject* dictkeys_reversed(PyObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_keys_doc,
"Return a reverse iterator over the dict keys.");

static PyMethodDef dictkeys_methods[] = {
    {"isdisjoint",      dictviews_isdisjoint,           METH_O,
     isdisjoint_doc},
    {"__reversed__",    dictkeys_reversed,              METH_NOARGS,
     reversed_keys_doc},
    {NULL,              NULL}           /* sentinel */
};

_Py_VALUE_NOTIMPL_RICHCMP(dictview_richcompare_value, dictview_richcompare)

PyTypeObject PyDictKeys_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_keys",                                /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictview_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dictview_repr,                              /* tp_repr */
    &dictviews_as_number,                       /* tp_as_number */
    &dictkeys_as_sequence,                      /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    dictview_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    dictview_richcompare,                       /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    dictkeys_iter,                              /* tp_iter */
    0,                                          /* tp_iternext */
    dictkeys_methods,                           /* tp_methods */
    .tp_getset = dictview_getset,
    .tp_value = {
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, dictview_richcompare_value),
        .contains = _Py_VALUE_SLOT_INIT(contains, dictkeys_contains_value),
    },
};

/*[clinic input]
dict.keys

Return a set-like object providing a view on the dict's keys.
[clinic start generated code]*/

static PyValue
dict_keys_impl(PyDictObject *self)
/*[clinic end generated code: output=9d9e6484a19538fd input=42f48a7a771212a7]*/
{
    return _PyValue_FromObjectResult(_PyDictView_New((PyObject *)self, &PyDictKeys_Type));
}

static PyObject *
dictkeys_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterKey_Type);
}

/*** dict_items ***/

static PyObject *
dictitems_iter(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterItem_Type);
}

static int
dictitems_contains_value(PyObject *self, PyValueBorrow obj)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL || !PyValue_IsObject(obj)) {
        return 0;
    }
    PyObject *item = PyValue_AsKnownObjectBorrow(obj);
    if (!PyTuple_Check(item) || PyTuple_GET_SIZE(item) != 2) {
        return 0;
    }
    PyObject *dict = Py_NewRef((PyObject *)dv->dv_dict);
    PyValue key = PyValue_DUP(PyTuple_GET_VALUE(item, 0));
    PyValue value = PyValue_DUP(PyTuple_GET_VALUE(item, 1));
    PyValue found = PyValue_NULL;
    Py_hash_t hash = _PyValue_Hash(key);
    int result = -1;
    if (hash != -1) {
        result = _PyDict_GetValue_KnownHash(dict, key, hash, &found);
        if (result == 1) {
            result = _PyValue_RichCompareBool(found, value, Py_EQ);
        }
    }
    else if (PyValue_IsObject(key)) {
        dict_unhashable_type(dict, PyValue_AsKnownObjectBorrow(key));
    }
    PyValue_XCLOSE(found);
    PyValue_XCLOSE(key);
    PyValue_XCLOSE(value);
    Py_XDECREF(dict);
    return result;
}
_Py_VALUE_THUNK_CONTAINS(dictitems_contains, dictitems_contains_value)

static PySequenceMethods dictitems_as_sequence = {
    dictview_len,                       /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    dictitems_contains,                 /* sq_contains */
};

static PyObject* dictitems_reversed(PyObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_items_doc,
"Return a reverse iterator over the dict items.");

static PyMethodDef dictitems_methods[] = {
    {"isdisjoint",      dictviews_isdisjoint,           METH_O,
     isdisjoint_doc},
    {"__reversed__",    dictitems_reversed,             METH_NOARGS,
     reversed_items_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictItems_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_items",                               /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictview_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dictview_repr,                              /* tp_repr */
    &dictviews_as_number,                       /* tp_as_number */
    &dictitems_as_sequence,                     /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    dictview_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    dictview_richcompare,                       /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    dictitems_iter,                             /* tp_iter */
    0,                                          /* tp_iternext */
    dictitems_methods,                          /* tp_methods */
    .tp_getset = dictview_getset,
    .tp_value = {
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, dictview_richcompare_value),
        .contains = _Py_VALUE_SLOT_INIT(contains, dictitems_contains_value),
    },
};

/*[clinic input]
dict.items

Return a set-like object providing a view on the dict's items.
[clinic start generated code]*/

static PyValue
dict_items_impl(PyDictObject *self)
/*[clinic end generated code: output=aa7dd632a434e260 input=87c822872eb71f5a]*/
{
    return _PyValue_FromObjectResult(_PyDictView_New((PyObject *)self, &PyDictItems_Type));
}

static PyObject *
dictitems_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterItem_Type);
}

/*** dict_values ***/

static PyObject *
dictvalues_iter(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterValue_Type);
}

static PySequenceMethods dictvalues_as_sequence = {
    dictview_len,                       /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    0,                                  /* sq_contains */
};

static PyObject* dictvalues_reversed(PyObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_values_doc,
"Return a reverse iterator over the dict values.");

static PyMethodDef dictvalues_methods[] = {
    {"__reversed__",    dictvalues_reversed,            METH_NOARGS,
     reversed_values_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictValues_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_values",                              /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictview_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dictview_repr,                              /* tp_repr */
    0,                                          /* tp_as_number */
    &dictvalues_as_sequence,                    /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    dictview_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    dictvalues_iter,                            /* tp_iter */
    0,                                          /* tp_iternext */
    dictvalues_methods,                         /* tp_methods */
    .tp_getset = dictview_getset,
};

/*[clinic input]
dict.values

Return an object providing a view on the dict's values.
[clinic start generated code]*/

static PyValue
dict_values_impl(PyDictObject *self)
/*[clinic end generated code: output=deac5c3698afa626 input=b46944f85493b230]*/
{
    return _PyValue_FromObjectResult(_PyDictView_New((PyObject *)self, &PyDictValues_Type));
}

static PyObject *
dictvalues_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterValue_Type);
}


#ifdef Py_SHAPES

#define SHAPES_INITIAL_CAPACITY 4

static _PyTypeShapes *
new_type_shapes(void)
{
    _PyTypeShapes *shapes = PyMem_Malloc(
        sizeof(_PyTypeShapes) + SHAPES_INITIAL_CAPACITY * sizeof(_PyShape));
    if (shapes == NULL) {
        return NULL;
    }
    shapes->nshapes = 1;
    shapes->capacity = SHAPES_INITIAL_CAPACITY;
    shapes->shapes[_Py_SHAPE_ROOT] = (_PyShape){
        .present = 0, .parent = _Py_SHAPE_NONE, .slot = 0,
    };
    return shapes;
}

uint8_t
_PyShape_Add(PyTypeObject *tp, uint8_t from, Py_ssize_t slot)
{
    struct _instancekeysobject *shared_keys = _PyDictKeys_AsSharedKeys(CACHED_KEYS(tp));
    _PyTypeShapes *shapes = shared_keys->dsk_shapes;
    assert(from < shapes->nshapes);
    assert(0 <= slot && slot < SHARED_KEYS_MAX_SIZE);
    assert(!_PyShape_HasSlot(tp, from, slot));
    // A shape is always added after its parent.
    for (int i = from + 1; i < shapes->nshapes; i++) {
        _PyShape *s = &shapes->shapes[i];
        if (s->parent == from && s->slot == slot) {
            return (uint8_t)i;
        }
    }
    if (shapes->nshapes == _Py_SHAPE_MAX_PER_TYPE) {
        return _Py_SHAPE_NONE;
    }
    if (shapes->nshapes == shapes->capacity) {
        int capacity = Py_MIN(2 * shapes->capacity, _Py_SHAPE_MAX_PER_TYPE);
        shapes = PyMem_Realloc(
            shapes, sizeof(_PyTypeShapes) + capacity * sizeof(_PyShape));
        if (shapes == NULL) {
            return _Py_SHAPE_NONE;
        }
        shapes->capacity = (uint8_t)capacity;
        shared_keys->dsk_shapes = shapes;
    }
    _PyShape *parent = &shapes->shapes[from];
    uint8_t index = shapes->nshapes++;
    shapes->shapes[index] = (_PyShape){
        .present = parent->present | (1u << slot), .parent = from,
        .slot = (uint8_t)slot,
    };
    return index;
}

#ifndef NDEBUG
static int
shape_matches_order(PyTypeObject *tp, PyDictValues *values)
{
    if (values->shape == _Py_SHAPE_NONE) {
        return 1;
    }
    _PyTypeShapes *shapes = _PyType_Shapes(tp);
    uint8_t *order = get_insertion_order_array(values);
    uint8_t shape = values->shape;
    for (int i = values->size - 1; i >= 0; i--) {
        if (shape == _Py_SHAPE_NONE || shapes->shapes[shape].slot != order[i]) {
            return 0;
        }
        shape = shapes->shapes[shape].parent;
    }
    return shape == _Py_SHAPE_ROOT;
}
#endif

static void
shape_after_delete(PyTypeObject *tp, PyDictValues *values)
{
    if (values->shape == _Py_SHAPE_NONE) {
        return;
    }
    // The attributes left, in their insertion order, are the path of the
    // new shape from the root.
    uint8_t shape = _Py_SHAPE_ROOT;
    uint8_t *order = get_insertion_order_array(values);
    for (int i = 0; i < values->size && shape != _Py_SHAPE_NONE; i++) {
        shape = _PyShape_Add(tp, shape, order[i]);
    }
    values->shape = shape;
    assert(shape_matches_order(tp, values));
}

#else

static inline void
shape_after_delete(PyTypeObject *tp, PyDictValues *values)
{
}

#endif  /* Py_SHAPES */

/* Returns NULL if cannot allocate a new PyDictKeysObject,
   but does not set an error */
PyDictKeysObject *
_PyDict_NewKeysForClass(PyHeapTypeObject *cls)
{
    Py_INCREF(cls);
    int log2_bytes = get_log2_bytes(NEXT_LOG2_SHARED_KEYS_MAX_SIZE);
    Py_ssize_t usable = USABLE_FRACTION((size_t)1 << NEXT_LOG2_SHARED_KEYS_MAX_SIZE);
    struct _instancekeysobject *shared = PyMem_Malloc(sizeof(*shared) +
        ((size_t)1 << log2_bytes) + sizeof(PyDictUnicodeEntry) * usable);
    PyDictKeysObject *keys = NULL;
    PyValue attrs = PyValue_NULL;
    if (shared == NULL) {
        goto done;
    }
    shared->dsk_owning_type = (PyTypeObject *)cls;
#ifdef Py_SHAPES
    shared->dsk_shapes = NULL;
    if (cls->ht_type.tp_itemsize == 0) {
        shared->dsk_shapes = new_type_shapes();
        if (shared->dsk_shapes == NULL) {
            PyMem_Free(shared);
            goto done;
        }
    }
#endif
    keys = &shared->dsk_keys;
    init_keys_object(keys, NEXT_LOG2_SHARED_KEYS_MAX_SIZE, log2_bytes,
                     DICT_KEYS_SPLIT, SHARED_KEYS_MAX_SIZE, sizeof(PyDictUnicodeEntry));
    if (cls->ht_type.tp_dict != NULL) {
        Py_hash_t hash = _PyValue_Hash(_PyValue_FromObjectKnown(&_Py_ID(__static_attributes__)));
        int found = hash == -1 ? -1 : _PyDict_GetValue_KnownHash(
            cls->ht_type.tp_dict, PyValue_FromObjectBorrow(&_Py_ID(__static_attributes__)),
            hash, &attrs);
        if (found < 0) {
            goto failed;
        }
        if (found && PyValue_IsObject(attrs) &&
            PyTuple_Check(PyValue_AsKnownObjectBorrow(attrs))) {
            PyObject *tuple = PyValue_AsKnownObjectBorrow(attrs);
            for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(tuple); i++) {
                PyValue key = PyTuple_GetValue(tuple, i);
                if (!PyValue_IsNull(key) && value_is_exact_unicode(key)) {
                    PyObject *name = PyValue_AsKnownObjectBorrow(key);
                    Py_hash_t key_hash = _PyValue_Hash(PyValue_FromObjectBorrow(name));
                    Py_ssize_t ix = key_hash == -1 ? DKIX_ERROR :
                        insert_split_key(keys, name, key_hash);
                    PyValue_XCLOSE(key);
                    if (ix == DKIX_ERROR) {
                        goto failed;
                    }
                    if (ix == DKIX_EMPTY) {
                        break;
                    }
                }
                else {
                    PyValue_XCLOSE(key);
                }
            }
        }
    }
    goto done;
failed:
#ifdef Py_SHAPES
    PyMem_Free(shared->dsk_shapes);
    shared->dsk_shapes = NULL;
#endif
    shared->dsk_owning_type = NULL;
    dictkeys_decref(keys, false);
    keys = NULL;
done:
    PyValue_XCLOSE(attrs);
    Py_XDECREF((PyObject *)cls);
    if (keys == NULL) {
        /* This optional metadata constructor reports failure by NULL
         * without an exception, as upstream's does. */
        PyErr_Clear();
    }
    return keys;
}

/* Visit the keys of a class's shared keys object, which instances with
   inline values use without a dict; for the tracing collector. */
int
_PyDict_TraverseKeysForClass(PyHeapTypeObject *cls, visitproc visit, void *arg)
{
    PyDictKeysObject *keys = cls->ht_cached_keys;
    if (keys == NULL) {
        return 0;
    }
    assert(DK_IS_UNICODE(keys));
    PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
    Py_ssize_t n = keys->dk_nentries;
    for (Py_ssize_t i = 0; i < n; i++) {
        _Py_VISIT_ATOMIC(entries[i].me_key);
    }
    return 0;
}

void
_PyDict_RemoveKeysForClass(PyHeapTypeObject *cls)
{
    struct _instancekeysobject *shared_keys = _PyDictKeys_AsSharedKeys(cls->ht_cached_keys);
    FT_ATOMIC_STORE_PTR_RELEASE(shared_keys->dsk_owning_type, NULL);
#ifdef Py_SHAPES
    // No instance of the type is left to use its shapes.
    PyMem_Free(shared_keys->dsk_shapes);
    shared_keys->dsk_shapes = NULL;
#endif

    _PyDictKeys_DecRef(cls->ht_cached_keys);
}

void
_PyObject_InitInlineValues(PyObject *obj, PyTypeObject *tp)
{
    assert(tp->tp_flags & Py_TPFLAGS_HEAPTYPE);
    assert(tp->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictKeysObject *keys = CACHED_KEYS(tp);
    assert(keys != NULL);
    OBJECT_STAT_INC(inline_values);
    if (keys->dk_usable > 1) {
        keys->dk_usable--;
    }
    size_t size = shared_keys_usable_size(keys);
    PyDictValues *values = _PyObject_InlineValues(obj);
    assert(size < 256);
    values->capacity = (uint8_t)size;
    values->size = 0;
    values->embedded = 1;
    values->valid = 1;
    _PyDictValues_SetShape(values, _Py_SHAPE_ROOT);
    for (size_t i = 0; i < size; i++) {
        values->values[i] = PyValue_NULL;
    }
    _PyObject_ManagedDictPointer(obj)->dict = NULL;
}

static PyDictObject *
make_dict_from_instance_attributes(PyObject *obj)
{
    /* Return obj's borrowed dictionary, creating one that aliases its valid
     * inline values if it has none. */
    PyDictObject *current = _PyObject_GetManagedDict(obj);
    if (current != NULL) {
        return current;
    }
    PyDictObject *dict = (PyDictObject *)PyDict_New();
    if (dict == NULL) {
        return NULL;
    }
    PyDictValues *values = (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) ?
        _PyObject_InlineValues(obj) : NULL;
    if (values != NULL && values->valid) {
        PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
        dictkeys_incref(keys);
        dict->ma_keys = keys;
        dict->ma_values = values;
        dict->ma_used = values->size;
        _PyDictValues_SetShape(values, _Py_SHAPE_NONE);
    }
    _PyObject_ManagedDictPointer(obj)->dict = dict;
    ASSERT_CONSISTENT(dict);
    return dict;
}

PyDictObject *
_PyObject_MaterializeManagedDict_LockHeld(PyObject *obj)
{
    ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(obj);
    OBJECT_STAT_INC(dict_materialized_on_request);
    return make_dict_from_instance_attributes(obj);
}

PyDictObject *
_PyObject_MaterializeManagedDict(PyObject *obj)
{
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict != NULL) {
        return dict;
    }

    Py_BEGIN_CRITICAL_SECTION(obj);

    dict = _PyObject_MaterializeManagedDict_LockHeld(obj);

    Py_END_CRITICAL_SECTION();
    return dict;
}

int
_PyDict_SetItem_LockHeld(PyDictObject *dict, PyObject *name, PyObject *value)
{
    if (!PyDict_Check(dict)) {
        if (PyFrozenDict_Check(dict)) {
            if (value == NULL) {
                frozendict_does_not_support("deletion");
            }
            else {
                frozendict_does_not_support("assignment");
            }
        }
        else {
            PyErr_BadInternalCall();
        }
        return -1;
    }

    if (value == NULL) {
        Py_hash_t hash = _PyObject_HashDictKey(name);
        if (hash == -1) {
            dict_unhashable_type((PyObject*)dict, name);
            return -1;
        }
        return _PyDict_DelItem_KnownHash_LockHeld((PyObject *)dict, name, hash);
    } else {
        return setitem_lock_held(dict, name, value);
    }
}

/* The caller holds the type dictionary lock and supplies an exact name. */
int
_PyDict_SetValue_LockHeld(PyDictObject *dict, PyObject *name,
                          PyValueBorrow value)
{
    ASSERT_DICT_LOCKED(dict);
    assert(PyDict_Check(dict));
    assert(PyUnicode_CheckExact(name));
    if (PyValue_IsNull(value)) {
        Py_hash_t hash = _PyObject_HashDictKey(name);
        if (hash == -1) {
            dict_unhashable_type((PyObject *)dict, name);
            return -1;
        }
        return delvalue_knownhash_lock_held(dict, PyValue_FromObjectBorrow(name), hash);
    }
    return setitem_take2_lock_held(dict, PyValue_DUP(PyValue_FromObjectBorrow(name)),
                                    PyValue_DUP(value));
}

// Called with either the object's lock or the dict's lock held
// depending on whether or not a dict has been materialized for
// the object.
static int
store_instance_attr_lock_held(PyObject *obj, PyObject *name, PyValue value)
{
    for (;;) {
        PyDictObject *dict = _PyObject_GetManagedDict(obj);
        PyDictValues *values = (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) ?
            _PyObject_InlineValues(obj) : NULL;
        if (dict != NULL || values == NULL || !values->valid || !PyUnicode_CheckExact(name)) {
            if (dict == NULL) {
                dict = (PyDictObject *)PyObject_GenericGetDict(obj, NULL);
                if (dict == NULL) {
                    return -1;
                }
            }
            else {
                Py_INCREF(dict);
            }
            int status = PyValue_IsNull(value) ?
                PyDict_DelValue((PyObject *)dict, PyValue_FromObjectBorrow(name)) :
                PyDict_SetValue((PyObject *)dict, PyValue_FromObjectBorrow(name), value);
            Py_XDECREF((PyObject *)dict);
            return status;
        }
        PyTypeObject *type = Py_TYPE(obj);
        Py_INCREF(type);
        PyDictKeysObject *keys = CACHED_KEYS(type);
        dictkeys_incref(keys);
        Py_hash_t hash = _PyValue_Hash(PyValue_FromObjectBorrow(name));
        Py_ssize_t ix = hash == -1 ? DKIX_ERROR : insert_split_key(keys, name, hash);
        /* insert_split_key() notifies type watchers, which can run Python that
         * changes obj's class, the class's keys or obj's dictionary; start
         * again if one did. While nothing changed, the releases below are not
         * final (obj owns type, and type owns keys). */
        int unchanged = Py_TYPE(obj) == type && CACHED_KEYS(type) == keys &&
            _PyObject_GetManagedDict(obj) == NULL &&
            _PyObject_InlineValues(obj)->valid;
        dictkeys_decref(keys, false);
        Py_XDECREF((PyObject *)type);
        if (ix == DKIX_ERROR) {
            return -1;
        }
        if (!unchanged) {
            continue;
        }
        values = _PyObject_InlineValues(obj);
        if (ix == DKIX_EMPTY || ix >= values->capacity) {
            if (_PyObject_MaterializeManagedDict_LockHeld(obj) == NULL) {
                return -1;
            }
            continue;
        }
        // a move: replaced below
        PyValue old_value = values->values[ix];
        if (PyValue_IsNull(old_value) && PyValue_IsNull(value)) {
            PyErr_Format(PyExc_AttributeError, "'%.100s' object has no attribute '%U'",
                         Py_TYPE(obj)->tp_name, name);
            (void)_PyObject_SetAttributeErrorContext(obj, name);
            return -1;
        }
        values->values[ix] = PyValue_XDUP(value);
        _PyValue_CheckHome(values->values[ix]);
        if (PyValue_IsNull(old_value)) {
            _PyDictValues_AddToInsertionOrder(values, ix);
            _PyDictValues_ShapeAfterAdd(Py_TYPE(obj), values, ix);
        }
        else if (PyValue_IsNull(value)) {
            delete_index_from_values(values, ix);
            shape_after_delete(Py_TYPE(obj), values);
        }
        PyValue_XCLOSE(old_value);
        return 0;
    }
}

static inline int
store_instance_attr_dict(PyObject *obj, PyDictObject *dict,
                         PyObject *name, PyValue value)
{
    /* Once a dictionary is visible, use its watched/reentrant transaction. */
    return PyValue_IsNull(value) ?
        PyDict_DelValue((PyObject *)dict, PyValue_FromObjectBorrow(name)) :
        PyDict_SetValue((PyObject *)dict, PyValue_FromObjectBorrow(name), value);
}

int
(_PyObject_StoreInstanceValue)(PyObject *obj, PyObject *name, PyValueBorrow value)
{
    assert(_PyValue_IsHome(value));
    assert(PyValue_IsLanguageValue(value) || PyValue_IsNull(value));
    Py_INCREF(obj);
    Py_INCREF(name);
    PyValue owner = PyValue_XDUP(value);
    int status = store_instance_attr_lock_held(obj, name, owner);
    PyValue_XCLOSE(owner);
    Py_XDECREF(name);
    Py_XDECREF(obj);
    return status;
}

int
_PyObject_TryGetInstanceValue(PyObject *obj, PyObject *name, PyValue *out)
{
    *out = PyValue_NULL;
    assert(PyUnicode_CheckExact(name));
    PyDictValues *values = _PyObject_InlineValues(obj);
    if (!values->valid) {
        return 0;
    }
    PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
    Py_ssize_t ix = _PyDictKeys_StringLookupSplit(keys, name);
    if (ix >= 0 && ix < values->capacity) {
        PyValueBorrow value = PyValue_Borrow(values->values[ix]);
        if (!PyValue_IsNull(value)) {
            *out = PyValue_DUP(value);
        }
    }
    return 1;
}

int
_PyObject_StoreInstanceAttribute(PyObject *obj, PyObject *name, PyObject *value)
{
    return _PyObject_StoreInstanceValue(obj, name, value == NULL ? PyValue_Borrow(PyValue_NULL) :
                                      PyValue_FromObjectBorrow(value));
}

/* Sanity check for managed dicts */
#if 0
#define CHECK(val) assert(val); if (!(val)) { return 0; }

int
_PyObject_ManagedDictValidityCheck(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    CHECK(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyManagedDictPointer *managed_dict = _PyObject_ManagedDictPointer(obj);
    if (_PyManagedDictPointer_IsValues(*managed_dict)) {
        PyDictValues *values = _PyManagedDictPointer_GetValues(*managed_dict);
        int size = ((uint8_t *)values)[-2];
        int count = 0;
        PyDictKeysObject *keys = CACHED_KEYS(tp);
        for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
            if (values->values[i] != NULL) {
                count++;
            }
        }
        CHECK(size == count);
    }
    else {
        if (managed_dict->dict != NULL) {
            CHECK(PyDict_Check(managed_dict->dict));
        }
    }
    return 1;
}
#endif

// Attempts to get an instance attribute from the inline values. Returns true
// if successful, or false if the caller needs to lookup in the dictionary.
bool
_PyObject_TryGetInstanceAttribute(PyObject *obj, PyObject *name, PyObject **attr)
{
    *attr = NULL;
    PyValue value = PyValue_NULL;
    int handled = _PyObject_TryGetInstanceValue(obj, name, &value);
    if (handled <= 0) {
        return handled < 0;
    }
    if (!PyValue_IsNull(value)) {
        *attr = _PyValue_AsObjectAt(value, BOX_SITE_ATTRIBUTE);
        PyValue_XCLOSE(value);
    }
    return true;
}

int
_PyObject_IsInstanceDictEmpty(PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (type->tp_dictoffset == 0) {
        return 1;
    }
    PyDictObject *dict;
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictValues *values = _PyObject_InlineValues(obj);
        if (values->valid) {
            return values->size == 0;
        }
        dict = _PyObject_GetManagedDict(obj);
    }
    else if (type->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        dict = _PyObject_GetManagedDict(obj);
    }
    else {
        dict = (PyDictObject *)*_PyObject_ComputedDictPointer(obj);
    }
    return dict == NULL || GET_USED(dict) == 0;
}

int
PyObject_VisitManagedDict(PyObject *obj, visitproc visit, void *arg)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return 0;
    }
    PyDictObject *dict = _PyObject_ManagedDictPointer(obj)->dict;
    if (dict != NULL) {
        // GH-130327: If there's a managed dictionary available, we should
        // *always* traverse it. The dict is responsible for traversing the
        // inline values if it points to them.
        Py_VISIT(dict);
    }
    else if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictValues *values = _PyObject_InlineValues(obj);
        if (values->valid) {
            for (Py_ssize_t i = 0; i < values->capacity; i++) {
                _Py_VISIT_VALUE(values->values[i]);
            }
        }
    }
    return 0;
}

static void
clear_inline_values(PyDictValues *values)
{
    if (values->valid) {
        PyValue refs[SHARED_KEYS_MAX_SIZE];
        Py_ssize_t capacity = values->capacity;
        assert(capacity <= SHARED_KEYS_MAX_SIZE);
        FT_ATOMIC_STORE_UINT8(values->valid, 0);
        _PyDictValues_SetShape(values, _Py_SHAPE_NONE);
        for (Py_ssize_t i = 0; i < capacity; i++) {
            refs[i] = values->values[i];
            values->values[i] = PyValue_NULL;
        }
        FT_ATOMIC_STORE_UINT8_RELEASE(values->size, 0);
        for (Py_ssize_t i = 0; i < capacity; i++) {
            PyValue_XCLOSE(refs[i]);
        }
    }
}

static void
set_dict_inline_values(PyObject *obj, PyDictObject *new_dict)
{
    Py_XINCREF(new_dict);
    PyDictObject *old = _PyObject_GetManagedDict(obj);
    _PyObject_ManagedDictPointer(obj)->dict = new_dict;
    clear_inline_values(_PyObject_InlineValues(obj));
    Py_XDECREF((PyObject *)old);
}


static void
decref_maybe_delay(PyObject *obj, bool delay)
{
    if (delay) {
        _PyObject_XDecRefDelayed(obj);
    }
    else {
        Py_XDECREF(obj);
    }
}

static int detach_dict_from_object(PyDictObject *, PyObject *);

int
_PyObject_SetManagedDict(PyObject *obj, PyObject *new_dict)
{
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictObject *old = _PyObject_GetManagedDict(obj);
    if (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        if (old != NULL && detach_dict_from_object(old, obj) < 0) {
            return -1;
        }
        set_dict_inline_values(obj, (PyDictObject *)new_dict);
        return 0;
    }
    FT_ATOMIC_STORE_PTR_RELEASE(_PyObject_ManagedDictPointer(obj)->dict,
        (PyDictObject *)Py_XNewRef(new_dict));
    decref_maybe_delay((PyObject *)old, true);
    return 0;
}

static int
detach_dict_from_object(PyDictObject *mp, PyObject *obj)
{
    assert(_PyObject_GetManagedDict(obj) == mp);
    PyDictValues *inline_values = _PyObject_InlineValues(obj);
    if (mp->ma_values != inline_values) {
        return 0;
    }
    assert(inline_values->valid);
    PyDictValues *values = new_values(inline_values->capacity);
    if (values == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    values->size = inline_values->size;
    memcpy(get_insertion_order_array(values),
           get_insertion_order_array(inline_values), inline_values->size);
    for (size_t i = 0; i < inline_values->capacity; i++) {
        values->values[i] = inline_values->values[i];
        inline_values->values[i] = PyValue_NULL;
    }
    /* MOVE: the detached array takes each owner; the old array owns none. */
    inline_values->valid = 0;
    inline_values->size = 0;
    _PyDictValues_SetShape(inline_values, _Py_SHAPE_NONE);
    mp->ma_values = values;
    ASSERT_CONSISTENT(mp);
    return 0;
}


void
PyObject_ClearManagedDict(PyObject *obj)
{
    /* subtype_dealloc can pass a refcount-zero instance. Never INCREF it.
     * The detach below can fail (out of memory), and that failure is reported
     * as unraisable; a deallocator must leave the current exception as it
     * found it (gh-89373), so set the caller's exception aside first. */
    PyObject *entry_error = PyErr_GetRaisedException();
    PyTypeObject *type = Py_TYPE(obj);
    Py_INCREF(type);
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    Py_XINCREF(dict);
    if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        if (dict == NULL) {
            clear_inline_values(_PyObject_InlineValues(obj));
        }
        else if (dict->ma_values == _PyObject_InlineValues(obj)) {
            if (detach_dict_from_object(dict, obj) < 0) {
                PyErr_FormatUnraisable("Exception ignored while clearing an object managed dict");
                /* The reporting hook may already detach/replace the association. */
                PyDictValues *values = _PyObject_InlineValues(obj);
                if (_PyObject_GetManagedDict(obj) == dict && dict->ma_values == values) {
                    PyValue refs[SHARED_KEYS_MAX_SIZE];
                    size_t capacity = values->capacity;
                    assert(capacity <= SHARED_KEYS_MAX_SIZE);
                    PyDictKeysObject *keys = dict->ma_keys;
                    for (size_t i = 0; i < capacity; i++) {
                        refs[i] = values->values[i];
                        values->values[i] = PyValue_NULL;
                    }
                    values->size = 0;
                    values->valid = 0;
                    _PyDictValues_SetShape(values, _Py_SHAPE_NONE);
                    dict->ma_keys = Py_EMPTY_KEYS;
                    dict->ma_values = NULL;
                    STORE_USED(dict, 0);
                    /* Every visible shape is empty before the first close. */
                    dictkeys_decref(keys, false);
                    for (size_t i = 0; i < capacity; i++) {
                        PyValue_XCLOSE(refs[i]);
                    }
                }
            }
        }
    }
    PyDictObject *current = _PyObject_GetManagedDict(obj);
    _PyObject_ManagedDictPointer(obj)->dict = NULL;
    Py_XDECREF((PyObject *)current);
    Py_XDECREF((PyObject *)dict);
    Py_XDECREF((PyObject *)type);
    PyErr_SetRaisedException(entry_error);
}

int
_PyDict_DetachFromObject(PyDictObject *mp, PyObject *obj)
{
    ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(obj);

    return detach_dict_from_object(mp, obj);
}

static inline PyObject *
ensure_managed_dict(PyObject *obj)
{
    for (;;) {
        PyDictObject *dict = _PyObject_GetManagedDict(obj);
        if (dict != NULL) {
            return (PyObject *)dict;
        }
        PyTypeObject *type = Py_TYPE(obj);
        if (type->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
            return (PyObject *)_PyObject_MaterializeManagedDict(obj);
        }
        Py_INCREF(type);
        PyDictKeysObject *keys = CACHED_KEYS(type);
        dict = (PyDictObject *)new_dict_with_shared_keys(keys);
        Py_XDECREF((PyObject *)type);
        if (dict == NULL) {
            return NULL;
        }
        if (_PyObject_GetManagedDict(obj) != NULL || Py_TYPE(obj) != type) {
            Py_XDECREF((PyObject *)dict);
            continue;
        }
        _PyObject_ManagedDictPointer(obj)->dict = dict;
        return (PyObject *)dict;
    }
}

static inline PyObject *
ensure_nonmanaged_dict(PyObject *obj, PyObject **dictptr)
{
    for (;;) {
        PyObject *dict = *dictptr;
        if (dict != NULL) {
            return dict;
        }
        PyTypeObject *type = Py_TYPE(obj);
        Py_INCREF(type);
        PyDictKeysObject *keys = PyType_HasFeature(type, Py_TPFLAGS_HEAPTYPE) ?
            CACHED_KEYS(type) : NULL;
        dict = keys == NULL ? PyDict_New() : new_dict_with_shared_keys(keys);
        Py_XDECREF((PyObject *)type);
        if (dict == NULL) {
            return NULL;
        }
        /* The generic setattr also stores a managed dict without inline
           values through here. */
        PyObject **current = (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT) ?
            (PyObject **)&_PyObject_ManagedDictPointer(obj)->dict :
            _PyObject_ComputedDictPointer(obj);
        if (current == NULL) {
            Py_XDECREF(dict);
            PyErr_SetString(PyExc_AttributeError, "This object has no __dict__");
            return NULL;
        }
        if (current != dictptr || *current != NULL || Py_TYPE(obj) != type) {
            Py_XDECREF(dict);
            dictptr = current;
            continue;
        }
        *current = dict;
        return dict;
    }
}

PyObject *
PyObject_GenericGetDict(PyObject *obj, void *context)
{
    Py_INCREF(obj);
    PyObject *dict;
    if (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        dict = ensure_managed_dict(obj);
    }
    else {
        PyObject **pointer = _PyObject_ComputedDictPointer(obj);
        if (pointer == NULL) {
            PyErr_SetString(PyExc_AttributeError, "This object has no __dict__");
            dict = NULL;
        }
        else {
            dict = ensure_nonmanaged_dict(obj, pointer);
        }
    }
    /* Acquire the outward owner while obj still roots the borrowed dict. */
    Py_XINCREF(dict);
    Py_XDECREF(obj);
    return dict;
}

int
(_PyObjectDict_SetValue)(PyTypeObject *tp, PyObject *obj, PyObject **dictptr,
                         PyObject *key, PyValueBorrow value)
{
    PyObject *dict;
    int res;

    assert(dictptr != NULL);
    dict = ensure_nonmanaged_dict(obj, dictptr);
    if (dict == NULL) {
        return -1;
    }

    Py_INCREF(dict);  /* The attribute store can replace obj's dictionary. */
    res = PyValue_IsNull(value) ?
        PyDict_DelValue(dict, PyValue_FromObjectBorrow(key)) :
        PyDict_SetValue(dict, PyValue_FromObjectBorrow(key), value);
    ASSERT_CONSISTENT(dict);
    Py_XDECREF(dict);
    return res;
}

void
_PyDictKeys_DecRef(PyDictKeysObject *keys)
{
    dictkeys_decref(keys, false);
}

static inline uint32_t
get_next_dict_keys_version(PyInterpreterState *interp)
{
    if (interp->dict_state.next_keys_version == 0) {
        return 0;
    }
    uint32_t v = interp->dict_state.next_keys_version++;
    return v;
}

// In free-threaded builds the caller must ensure that the keys object is not
// being mutated concurrently by another thread.
uint32_t
_PyDictKeys_GetVersionForCurrentState(PyInterpreterState *interp,
                                      PyDictKeysObject *dictkeys)
{
    uint32_t dk_version = FT_ATOMIC_LOAD_UINT32_RELAXED(dictkeys->dk_version);
    if (dk_version != 0) {
        return dk_version;
    }
    dk_version = get_next_dict_keys_version(interp);
    FT_ATOMIC_STORE_UINT32_RELAXED(dictkeys->dk_version, dk_version);
    return dk_version;
}

uint32_t
_PyDict_GetKeysVersionForCurrentState(PyInterpreterState *interp,
                                      PyDictObject *dict)
{
    ASSERT_DICT_LOCKED((PyObject *) dict);
    uint32_t dk_version =
        _PyDictKeys_GetVersionForCurrentState(interp, dict->ma_keys);
    ensure_shared_on_keys_version_assignment(dict);
    return dk_version;
}

static inline int
validate_watcher_id(PyInterpreterState *interp, int watcher_id)
{
    if (watcher_id < 0 || watcher_id >= DICT_MAX_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "Invalid dict watcher ID %d", watcher_id);
        return -1;
    }
    PyDict_WatchCallback cb = FT_ATOMIC_LOAD_PTR_RELAXED(
        interp->dict_state.watchers[watcher_id]);
    if (cb == NULL) {
        PyErr_Format(PyExc_ValueError, "No dict watcher set for ID %d", watcher_id);
        return -1;
    }
    return 0;
}

// In free-threaded builds, Add/Clear serialize on watcher_mutex and publish
// callbacks with release stores. SendEvent reads them lock-free using
// acquire loads.

int
PyDict_Watch(int watcher_id, PyObject* dict)
{
    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_ValueError, "Cannot watch non-dictionary");
        return -1;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id)) {
        return -1;
    }
    FT_ATOMIC_OR_UINT64(((PyDictObject*)dict)->_ma_watcher_tag,
                        1ULL << watcher_id);
    return 0;
}

int
PyDict_Unwatch(int watcher_id, PyObject* dict)
{
    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_ValueError, "Cannot watch non-dictionary");
        return -1;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id)) {
        return -1;
    }
    FT_ATOMIC_AND_UINT64(((PyDictObject*)dict)->_ma_watcher_tag,
                         ~(1ULL << watcher_id));
    return 0;
}

int
PyDict_AddWatcher(PyDict_WatchCallback callback)
{
    int watcher_id = -1;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    FT_MUTEX_LOCK_FLAGS(&interp->dict_state.watcher_mutex,
                        _Py_LOCK_DONT_DETACH);
    /* Some watchers are reserved for CPython, start at the first available one */
    for (int i = FIRST_AVAILABLE_WATCHER; i < DICT_MAX_WATCHERS; i++) {
        if (!interp->dict_state.watchers[i]) {
            FT_ATOMIC_STORE_PTR_RELEASE(interp->dict_state.watchers[i], callback);
            watcher_id = i;
            goto done;
        }
    }
    PyErr_SetString(PyExc_RuntimeError, "no more dict watcher IDs available");
done:
    FT_MUTEX_UNLOCK(&interp->dict_state.watcher_mutex);
    return watcher_id;
}

int
PyDict_ClearWatcher(int watcher_id)
{
    int res = 0;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    FT_MUTEX_LOCK_FLAGS(&interp->dict_state.watcher_mutex,
                        _Py_LOCK_DONT_DETACH);
    if (validate_watcher_id(interp, watcher_id)) {
        res = -1;
        goto done;
    }
    FT_ATOMIC_STORE_PTR_RELEASE(interp->dict_state.watchers[watcher_id], NULL);
done:
    FT_MUTEX_UNLOCK(&interp->dict_state.watcher_mutex);
    return res;
}

static const char *
dict_event_name(PyDict_WatchEvent event) {
    switch (event) {
        #define CASE(op)                \
        case PyDict_EVENT_##op:         \
            return "PyDict_EVENT_" #op;
        PY_FOREACH_DICT_EVENT(CASE)
        #undef CASE
    }
    Py_UNREACHABLE();
}

void
_PyDict_SendEvent(int watcher_bits, PyDict_WatchEvent event,
                  PyDictObject *mp, PyValueBorrow key, PyValueBorrow value)
{
    /* As upstream, a callback can be called with an exception pending and
     * must leave it pending (PyDict_WatchCallback). */
    PyValue key_owner = PyValue_XDUP(key);
    PyValue value_owner = PyValue_XDUP(value);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    /* mp follows the existing metadata/resurrection protocol. In particular,
     * DEALLOCATED must not acquire an ordinary owner of a refcount-zero dict. */
    for (int i = 0; i < DICT_MAX_WATCHERS; i++) {
        if (watcher_bits & 1) {
            PyDict_WatchCallback cb = interp->dict_state.watchers[i];
            if (cb != NULL) {
                int status = cb(event, (PyObject *)mp,
                                PyValue_Borrow(key_owner),
                                PyValue_Borrow(value_owner));
                if (status < 0) {
                    PyErr_FormatUnraisable(
                        "Exception ignored in %s watcher callback for <dict at %p>",
                        dict_event_name(event), mp);
                }
            }
        }
        watcher_bits >>= 1;
    }
    PyValue_XCLOSE(value_owner);
    PyValue_XCLOSE(key_owner);
}

#ifndef NDEBUG
static int
_PyObject_InlineValuesConsistencyCheck(PyObject *obj)
{
    if ((Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) == 0) {
        return 1;
    }
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL) {
        return 1;
    }
    if (dict->ma_values == _PyObject_InlineValues(obj) ||
        _PyObject_InlineValues(obj)->valid == 0) {
        return 1;
    }
    assert(0);
    return 0;
}
#endif

// --- frozendict implementation ---------------------------------------------

static PyObject *
frozendict_getnewargs(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    // Call dict(op): convert 'op' frozendict to a dict
    PyObject *arg = PyObject_CallOneArg((PyObject*)&PyDict_Type, op);
    if (arg == NULL) {
        return NULL;
    }
    return Py_BuildValue("(N)", arg);
}


static PyNumberMethods frozendict_as_number = {
    .nb_or = frozendict_or,
};

static PyMappingMethods frozendict_as_mapping = {
    .mp_length = frozendict_length,
    .mp_subscript = _PyDict_Subscript,
};

static PyMethodDef frozendict_methods[] = {
    DICT___CONTAINS___METHODDEF
    {"__getitem__", _PyCFunction_CAST(dict_getitem_value),
     METH_VALUE | METH_O | METH_COEXIST, getitem__doc__},
    DICT___SIZEOF___METHODDEF
    DICT_GET_METHODDEF
    DICT_KEYS_METHODDEF
    DICT_ITEMS_METHODDEF
    DICT_VALUES_METHODDEF
    DICT_FROMKEYS_METHODDEF
    FROZENDICT_COPY_METHODDEF
    DICT___REVERSED___METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("frozendicts are generic over two types, signifying (respectively) the types of the frozendict's keys and values")},
    {"__getnewargs__", frozendict_getnewargs, METH_NOARGS},
    {NULL,              NULL}   /* sentinel */
};


static PyObject *
frozendict_repr(PyObject *self)
{
    PyDictObject *mp = _PyAnyDict_CAST(self);
    if (mp->ma_used == 0) {
        return PyUnicode_FromFormat("%s()", Py_TYPE(self)->tp_name);
    }

    PyObject *repr = anydict_repr_impl(self);
    if (repr == NULL) {
        return NULL;
    }
    assert(PyUnicode_Check(repr));

    PyObject *res = PyUnicode_FromFormat("%s(%U)",
                                         Py_TYPE(self)->tp_name,
                                         repr);
    Py_DECREF(repr);
    return res;
}

static Py_uhash_t
_shuffle_bits(Py_uhash_t h)
{
    return ((h ^ 89869747UL) ^ (h << 16)) * 3644798167UL;
}

// Compute hash((key, value)).
// Code copied from tuple_hash().
static Py_hash_t
frozendict_pair_hash(Py_hash_t key_hash, PyValue value)
{
    assert(key_hash != -1);

    const Py_ssize_t len = 2;
    Py_uhash_t acc = _PyTuple_HASH_XXPRIME_5;

    Py_uhash_t lane = key_hash;
    acc += lane * _PyTuple_HASH_XXPRIME_2;
    acc = _PyTuple_HASH_XXROTATE(acc);
    acc *= _PyTuple_HASH_XXPRIME_1;

    lane = _PyValue_Hash(value);
    if (lane == (Py_uhash_t)-1) {
        return -1;
    }
    acc += lane * _PyTuple_HASH_XXPRIME_2;
    acc = _PyTuple_HASH_XXROTATE(acc);
    acc *= _PyTuple_HASH_XXPRIME_1;

    /* Add input length, mangled to keep the historical value of hash(()). */
    acc += len ^ (_PyTuple_HASH_XXPRIME_5 ^ 3527539UL);

    if (acc == (Py_uhash_t)-1) {
        acc = 1546275796;
    }
    return acc;
}


// Code copied from frozenset_hash()
static Py_hash_t
frozendict_hash(PyObject *op)
{
    PyFrozenDictObject *self = _PyFrozenDictObject_CAST(op);
    Py_hash_t shash = FT_ATOMIC_LOAD_SSIZE_RELAXED(self->ma_hash);
    if (shash != -1) {
        return shash;
    }

    PyDictObject *mp = _PyAnyDict_CAST(op);
    Py_uhash_t hash = 0;

    /* Hashing a value recurses back into frozendict_hash() when that value
       is itself a frozendict, so guard the descent to raise RecursionError
       rather than overflow the C stack on a deeply nested frozendict.  Keys
       are safe: their hashes are already computed at construction time. */
    if (_Py_EnterRecursiveCall(" while hashing a frozendict")) {
        return -1;
    }

    PyValue value;
    Py_ssize_t pos = 0;
    Py_hash_t key_hash;
    int found;
    while ((found = _PyDict_NextValue(op, &pos, NULL, &value, &key_hash)) > 0) {
        Py_hash_t pair_hash = frozendict_pair_hash(key_hash, value);
        PyValue_XCLOSE(value);
        if (pair_hash == -1) {
            _Py_LeaveRecursiveCall();
            return -1;
        }
        hash ^= _shuffle_bits(pair_hash);
    }
    _Py_LeaveRecursiveCall();
    if (found < 0) {
        return -1;
    }

    /* Factor in the number of active entries */
    hash ^= ((Py_uhash_t)mp->ma_used + 1) * 1927868237UL;

    /* Disperse patterns arising in nested frozendicts */
    hash ^= (hash >> 11) ^ (hash >> 25);
    hash = hash * 69069U + 907133923UL;

    /* -1 is reserved as an error code */
    if (hash == (Py_uhash_t)-1) {
        hash = 590923713UL;
    }

    FT_ATOMIC_STORE_SSIZE_RELAXED(self->ma_hash, (Py_hash_t)hash);
    return (Py_hash_t)hash;
}


/* Allocate an empty, GC-untracked frozendict; the constructor tracks it once
   fully built. */
static PyObject *
frozendict_new_untracked(PyTypeObject *type)
{
    assert(PyObject_IsSubclass((PyObject*)type, (PyObject*)&PyFrozenDict_Type));

    PyObject *d = anydict_new_untracked(type);
    if (d == NULL) {
        return NULL;
    }
    assert(can_modify_dict(_PyAnyDict_CAST(d)));
    _PyFrozenDictObject_CAST(d)->ma_hash = -1;
    return d;
}

static PyObject *
frozendict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *d = frozendict_new_untracked(type);
    if (d == NULL) {
        return NULL;
    }

    if (args != NULL) {
        if (dict_update_common(d, args, kwds, "frozendict") < 0) {
            Py_DECREF(d);
            return NULL;
        }
    }
    else {
        assert(kwds == NULL);
    }

    _PyObject_GC_TRACK(d);
    return d;
}


PyObject*
PyFrozenDict_New(PyObject *iterable)
{
    if (iterable != NULL) {
        if (PyFrozenDict_CheckExact(iterable)) {
            // PyFrozenDict_New(frozendict) returns the same object unmodified
            return Py_NewRef(iterable);
        }

        PyObject *args = _PyTuple_PackValues(1, (PyValueBorrow[]){PyValue_FromObjectBorrow(iterable)});
        if (args == NULL) {
            return NULL;
        }
        PyObject *frozendict = frozendict_new(&PyFrozenDict_Type, args, NULL);
        Py_DECREF(args);
        return frozendict;
    }
    else {
        PyObject *args = Py_GetConstantBorrowed(Py_CONSTANT_EMPTY_TUPLE);
        return frozendict_new(&PyFrozenDict_Type, args, NULL);
    }
}

/*[clinic input]
frozendict.copy

Return a shallow copy of the frozendict.
[clinic start generated code]*/

static PyValue
frozendict_copy_impl(PyFrozenDictObject *self)
/*[clinic end generated code: output=4deece76550d0b8e input=35f6abeaa08fd4bc]*/
{
    assert(PyFrozenDict_Check(self));

    if (PyFrozenDict_CheckExact(self)) {
        return PyValue_DUP(PyValue_FromObjectBorrow(self));
    }

    PyObject *copy = anydict_copy_untracked((PyObject*)self);
    if (copy != NULL) {
        _PyObject_GC_TRACK(copy);
    }
    return _PyValue_FromObjectResult(copy);
}

PyDoc_STRVAR(frozendict_doc,
"frozendict() -> new empty immutable dictionary\n"
"frozendict(mapping) -> new immutable dictionary initialized from a mapping\n"
"    object's (key, value) pairs\n"
"frozendict(iterable) -> new immutable dictionary initialized as if via:\n"
"    d = {}\n"
"    for k, v in iterable:\n"
"        d[k] = v\n"
"    d = frozendict(d)\n"
"frozendict(**kwargs) -> new immutable dictionary initialized with the name=value\n"
"    pairs in the keyword argument list.  For example:  frozendict(one=1, two=2)");

PyTypeObject PyFrozenDict_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "frozendict",
    .tp_basicsize = sizeof(PyFrozenDictObject),
    .tp_dealloc = dict_dealloc,
    .tp_repr = frozendict_repr,
    .tp_as_number = &frozendict_as_number,
    .tp_as_sequence = &dict_as_sequence,
    .tp_as_mapping = &frozendict_as_mapping,
    .tp_hash = frozendict_hash,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC
                | Py_TPFLAGS_BASETYPE
                | _Py_TPFLAGS_MATCH_SELF | Py_TPFLAGS_MAPPING,
    .tp_doc = frozendict_doc,
    .tp_traverse = dict_traverse,
    .tp_clear = dict_tp_clear,
    .tp_richcompare = dict_richcompare,
    .tp_iter = dict_iter,
    .tp_methods = frozendict_methods,
    .tp_alloc = _PyType_AllocNoTrack,
    .tp_new = frozendict_new,
    .tp_free = PyObject_GC_Del,
    .tp_vectorcall = _PyValue_VectorcallThunk,
    .tp_value_vectorcall = (_Py_valuefunc)frozendict_vectorcall,
    .tp_version_tag = _Py_TYPE_VERSION_FROZENDICT,
    .tp_value = {
        .nb_or = _Py_VALUE_SLOT_INIT(nb_or, frozendict_or_value),
        .subscript = _Py_VALUE_SLOT_INIT(subscript, dict_subscript_value),
        .contains = _Py_VALUE_SLOT_INIT(contains, dict_contains_value),
        .richcompare = _Py_VALUE_SLOT_INIT(richcompare, dict_richcompare_value),
    },
};
