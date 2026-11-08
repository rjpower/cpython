# LRU cache value continuation proposal

Base: runtime `7baf96ed017`. Scope is `_functools._lru_cache_wrapper`; `partial`, `cmp_to_key`, cache-info and constructor parser work are separate tails unless explicitly admitted below. The current link key/result fields already are owned `PyValue` homes and their traverse/dealloc paths already visit/close both.

## Call and key path

Pair `_Py_value_call` with `_PyValue_TpCallThunk` in the type spec. The value call receives the value argument window and keyword names; build the argument tuple and keyword dict using the existing value-window helpers, preserving keyword insertion order and owners, then invoke the selected private wrapper with these containers. The tuple owns its existing values without projecting them. Preserve the tuple-form object boundary and method-descriptor binding semantics. `lru_cache_make_key` returns an owned `PyValue`: the fast single exact integer/unicode key duplicates its value rather than projecting it; other paths return a stolen tuple value and retain the current typed markers and keyword ordering. Hash the borrowed key once and preserve the existing reuse of that exact hash across lookups/insertion/eviction.

## Three cache policies

Private wrapper callbacks return owned `PyValue`/null on error. Uncached calls `_PyValue_Call` with the existing tuple and dict, retaining miss accounting and exception behavior. Infinite-cache lookup uses `_PyDict_GetValue_KnownHash`; a hit transfers the owned result directly, while a miss invokes `_PyValue_Call`, inserts the borrowed result with the owned key, and returns the owner. Preserve references and counters around each potentially reentrant hash, equality and call.

Bounded cache get/update take owned value key/result, duplicating a hit result before releasing the held link. Existing link key/result homes receive duplicated values; on reuse, retain old key/result owners until cache dict and linked-list publication is complete, exactly as current code does. Preserve all orphan-link, deletion, reentrant equality, duplicate insertion, error, hit/miss and ordering paths. Current bounded known-hash lookup/set/pop helpers are object-oriented: a small exported private `_PyDict_PopValue_KnownHash` wrapper over `dict_pop_knownhash` is needed to keep stored key values inline without rehashing or changing watcher/equality order; use the existing known-hash value get/set helpers and preserve existing critical sections. Returned link is concrete heap-only after checking the known cache invariant; user result never requires projection.

## Construction and limits

The type spec currently has `Py_TPFLAGS_IMMUTABLETYPE` and no `Py_TPFLAGS_BASETYPE`; its own type therefore is not subclassable. Do not install an inherited fast path for unrelated mutable types. The current `PyArg_ParseTupleAndKeywords(..., "OOpO")` constructor can box inline maxsize/typed, so this remains a separate constructor-parser gap until a value-aware parser is specified and reviewed. Preserve all callable, None/index/overflow, typed truth-callback, duplicate/keyword and arity errors if that gap is ported. Do not relabel these boxes.

Validate `test_functools` focused LRU tests, hit/miss/eviction, int and float result/keys, typed calls and keyword ordering, mutable hashes/equality failures, reentrant cache_clear/misses and object finalization, GC cycles, wrapped Python and C callables, and object-call boundary behavior. Full integration tests and stats belong to the coordinator.

The value-slot entry can use `_PyTuple_FromValueArray(args, PyVectorcall_NARGS(nargsf))`; for non-null `kwnames`, `_PyValue_StackAsDict(args + nargs, kwnames)` creates the keyword dictionary. These are existing borrowed-window APIs. `_PyDict_GetValue_KnownHash` returns the owned dictionary entry, which must be checked as the private link type before borrowing its object pointer in the bounded path. A new pop provider should return an owned value via `dict_pop_knownhash(..., value_out, NULL)` and preserve the existing watcher and known-hash semantics; its precise exported signature needs review.
