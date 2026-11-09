# LRU cache value integration

The `_functools._lru_cache_wrapper` caches calls with either no storage, an unbounded dictionary, or a bounded dictionary plus an LRU list. The cache must preserve argument and keyword ordering, the selected policy's hit/miss accounting, and behavior under reentrant hash, equality, function calls, finalization, and cache clearing.

In the current runtime, bounded list links already own `PyValue` key and result fields. Their traversal and destruction visit and close both homes. The wrapper still projects values to objects on its call, key, lookup, result, and bounded eviction paths. The constructor's `OOpO` parser is also an object boundary. Consequently this area is not yet fully value-native.

The reviewed continuation design carries owned values through the three policies and pairs a value call slot with the actual object call thunk. It preserves each computed key hash across lookup, insertion, and eviction. It retains old link owners until dictionary and linked-list updates are consistent, so callbacks cannot observe a partially updated cache. The key builder retains the current exact-integer/unicode shortcut, typed distinctions, and ordered keywords. A narrow known-hash value-pop provider is required for bounded eviction; it must retain dictionary watcher behavior and error ordering.

The wrapper type is currently immutable and does not advertise subclassing. Constructor argument parsing remains separate work: its callable checks, index conversion and overflow, truth callbacks, positional and keyword rules, and exception text must remain compatible. No value-native constructor admission is claimed.

The source-specific contract, validation plan, and review status are in [the LRU proposal](../issues/checkpoint-closure/functools-lru-values-proposal.md). At this writing the design is approved but the implementation has not landed; automatic approval rejected the initial implementation edit and prohibited workarounds. The proposal excludes `partial`, `cmp_to_key`, and cache-info result migration.
