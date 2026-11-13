#ifndef Py_INTERNAL_HOME_BITMAP_H
#define Py_INTERNAL_HOME_BITMAP_H
#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interpframe_structs.h"
#include "pycore_bitutils.h"
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_value.h"

/* Canonical non-NULL frame-owned homes. A positive bit authorizes a value
 * load; zero only authorizes NULL synthesis by an absence-aware consumer.
 * Named-local boundness and execution/protocol reads have separate contracts. */
static_assert(sizeof(PyValue) == sizeof(uint64_t), "bitmap cells are eight bytes");

static inline size_t
_PyHomeBitmap_HeapWords(int nslots)
{
    /* The first home can occupy any bit of the first physical word. */
    return ((size_t)nslots + 126) / 64;
}

static inline size_t
_PyStackChunk_BitmapWords(size_t size, unsigned int first_bit)
{
    size_t cells = (size - offsetof(_PyStackChunk, data)) / sizeof(PyValue);
    return (first_bit + cells + 64) / 65;
}

static inline uint64_t *
_PyHomeBitmap_Word(uint64_t *words, const PyValue *base, const PyValue *home)
{
    return words + (((uintptr_t)home >> 9) - ((uintptr_t)base >> 9));
}

static inline void
_PyFrame_BindChunkBitmap(_PyInterpreterFrame *frame, _PyStackChunk *chunk,
                         PyValue *base)
{
    frame->presence_bits = _PyHomeBitmap_Word(
        chunk->presence_bits, (const PyValue *)chunk->data, base);
}

static inline uint64_t
_PyHomeBitmap_Mask(const PyValue *home)
{
    return UINT64_C(1) << (((uintptr_t)home >> 3) & 63);
}

static inline uint64_t *
_PyHomeBitmap_InitStorage(PyValue *base, int nslots)
{
    uint64_t *words = (uint64_t *)(base + nslots);
    memset(words, 0, _PyHomeBitmap_HeapWords(nslots) * sizeof(uint64_t));
    return words;
}

static inline void
_PyFrame_InitHeapBitmap(_PyInterpreterFrame *frame, int nslots)
{
    frame->presence_bits = _PyHomeBitmap_InitStorage(frame->owned_slots, nslots);
}

static inline uint64_t *
_PyFrame_BitmapWord(_PyInterpreterFrame *frame, const PyValue *home)
{
    assert(frame->presence_bits != NULL);
    assert(home >= frame->base);
    return _PyHomeBitmap_Word(frame->presence_bits, frame->base, home);
}

static inline int
_PyFrame_HomeIsPresent(_PyInterpreterFrame *frame, const PyValue *home)
{
    uint64_t *word = _PyFrame_BitmapWord(frame, home);
    return (FT_ATOMIC_LOAD_UINT64_RELAXED(*word) & _PyHomeBitmap_Mask(home)) != 0;
}

/* Only absence-aware consumers may interpret a vacant home as NULL. The
 * caller separately supplies its actual ownership extent and read role. */
static inline PyValue
_PyFrame_ReadHome(_PyInterpreterFrame *frame, const PyValue *home)
{
    return _PyFrame_HomeIsPresent(frame, home) ? *home : PyValue_NULL;
}

static inline void
_PyFrame_SetHomeBit(_PyInterpreterFrame *frame, const PyValue *home, int present)
{
    uint64_t *word = _PyFrame_BitmapWord(frame, home);
    uint64_t mask = _PyHomeBitmap_Mask(home);
    uint64_t old = FT_ATOMIC_LOAD_UINT64_RELAXED(*word);
    if (((old & mask) != 0) == present) {
        return;
    }
    if (present) {
        FT_ATOMIC_OR_UINT64(*word, mask);
    }
    else {
        FT_ATOMIC_AND_UINT64(*word, ~mask);
    }
}

/* The caller supplies canonical value/initialized-home authority. No poll,
 * close or callback may separate the home write from its bit update. */
static inline void
_PyFrame_WriteHome(_PyInterpreterFrame *frame, PyValue *home, PyValue value)
{
    *home = value;
    _PyFrame_SetHomeBit(frame, home, !PyValue_IsNull(value));
}

/* Storage has established a scratch destination. Absence has no value word;
 * initialized named bindings instead use WriteHome, including for NULL. */
static inline void
_PyFrame_WriteScratchHome(_PyInterpreterFrame *frame, PyValue *home, PyValue value)
{
    if (!PyValue_IsNull(value)) {
        *home = value;
    }
#ifdef Py_GIL_DISABLED
    else {
        *home = PyValue_NULL;
    }
#endif
    _PyFrame_SetHomeBit(frame, home, !PyValue_IsNull(value));
}

static inline PyValue
_PyFrame_TakeHome(_PyInterpreterFrame *frame, PyValue *home)
{
    PyValue value = _PyFrame_ReadHome(frame, home);
    _PyFrame_WriteHome(frame, home, PyValue_NULL);
    return value;
}

static inline void
_PyFrame_MoveHome(_PyInterpreterFrame *src, PyValue *from,
                   _PyInterpreterFrame *dest, PyValue *to)
{
    if (from != to) {
        PyValue value = _PyFrame_TakeHome(src, from);
        _PyFrame_WriteHome(dest, to, value);
    }
}

/* Reload after every callback. Endpoint masks preserve adjacent owners in
 * the same physical word. The caller separately supplies a readable region. */
static inline unsigned int
_PyHomeBitmap_HighBit(uint64_t word)
{
    assert(word != 0);
    uint32_t high = (uint32_t)(word >> 32);
    return high ? 31 + _Py_bit_length(high) : _Py_bit_length((uint32_t)word) - 1;
}

/* The view carries the physical first bit without retaining a home pointer. */
static inline int
_PyHomeBitmap_NextBit(const uint64_t *bits, unsigned int first,
                      int begin, int end, int present)
{
    while (begin < end) {
        unsigned int position = first + begin;
        unsigned int bit = position & 63;
        int width = Py_MIN(64 - bit, (unsigned int)(end - begin));
        uint64_t mask = UINT64_MAX << bit;
        if (width < 64 - bit) {
            mask &= (UINT64_C(1) << (bit + width)) - 1;
        }
        uint64_t word = FT_ATOMIC_LOAD_UINT64_RELAXED(bits[position / 64]);
        if (!present) {
            word = ~word;
        }
        word &= mask;
        if (word) {
            unsigned int low = _PyHomeBitmap_HighBit(word & (UINT64_C(0) - word));
            return begin + low - bit;
        }
        begin += width;
    }
    return end;
}

static inline int
_PyHomeBitmap_PreviousBit(const uint64_t *bits, unsigned int first,
                          int begin, int end, int present)
{
    while (end > begin) {
        unsigned int physical = first + (unsigned int)end - 1;
        unsigned int bit = physical & 63;
        unsigned int width = Py_MIN(bit + 1, (unsigned int)(end - begin));
        uint64_t mask = UINT64_MAX >> (63 - bit);
        if (width < bit + 1) {
            mask &= UINT64_MAX << (bit + 1 - width);
        }
        uint64_t word = FT_ATOMIC_LOAD_UINT64_RELAXED(bits[physical >> 6]);
        word = (present ? word : ~word) & mask;
        if (word) {
            return end - 1 - bit + _PyHomeBitmap_HighBit(word);
        }
        end -= width;
    }
    return begin - 1;
}

static inline int
_PyFrame_RangeIsPresent(_PyInterpreterFrame *frame, const PyValue *homes, int count)
{
    assert(count >= 0);
    if (count == 0) {
        return 1;
    }
    int begin = (int)(homes - frame->base);
    assert(begin >= 0);
    unsigned int first = ((uintptr_t)frame->base >> 3) & 63;
    return _PyHomeBitmap_NextBit(frame->presence_bits, first,
                                begin, begin + count, 0) == begin + count;
}

static inline int
_PyHomeBitmap_NextPresentHome(uint64_t *presence_bits, const PyValue *base,
                             int begin, int end)
{
    while (begin < end) {
        const PyValue *home = base + begin;
        unsigned int bit = ((uintptr_t)home >> 3) & 63;
        int width = Py_MIN(64 - bit, (unsigned int)(end - begin));
        uint64_t mask = UINT64_MAX << bit;
        if (width < 64 - bit) {
            mask &= (UINT64_C(1) << (bit + width)) - 1;
        }
        uint64_t word = FT_ATOMIC_LOAD_UINT64_RELAXED(
            *_PyHomeBitmap_Word(presence_bits, base, home)) & mask;
        if (word) {
            unsigned int low = _PyHomeBitmap_HighBit(word & (UINT64_C(0) - word));
            return begin + low - bit;
        }
        begin += width;
    }
    return end;
}

static inline int
_PyHomeBitmap_PreviousPresentHome(uint64_t *presence_bits, const PyValue *base,
                                 int begin, int end)
{
    while (end > begin) {
        const PyValue *home = base + end - 1;
        unsigned int bit = ((uintptr_t)home >> 3) & 63;
        unsigned int width = Py_MIN(bit + 1, (unsigned int)(end - begin));
        uint64_t mask = UINT64_MAX >> (63 - bit);
        if (width < bit + 1) {
            mask &= UINT64_MAX << (bit + 1 - width);
        }
        uint64_t word = FT_ATOMIC_LOAD_UINT64_RELAXED(
            *_PyHomeBitmap_Word(presence_bits, base, home)) & mask;
        if (word) {
            return end - 1 - bit + _PyHomeBitmap_HighBit(word);
        }
        end -= width;
    }
    return begin - 1;
}

static inline int
_PyFrame_NextPresentHome(_PyInterpreterFrame *frame, int begin, int end)
{
    assert(frame->presence_bits != NULL);
    return _PyHomeBitmap_NextPresentHome(frame->presence_bits, frame->base, begin, end);
}

static inline int
_PyFrame_PreviousPresentHome(_PyInterpreterFrame *frame, int begin, int end)
{
    assert(frame->presence_bits != NULL);
    return _PyHomeBitmap_PreviousPresentHome(frame->presence_bits, frame->base, begin, end);
}

#endif
