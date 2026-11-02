# Value-native VM iteration

- Stage: next design; no active source owner during the wiki reset
- Owner: coordinator assigns a fresh implementation owner and reviewer
- Hypothesis: generic VM iteration can consume canonical iterator providers
  directly and avoid boxing each scalar result.

List/tuple virtual fast paths and container providers are partly value-native;
generic FOR_ITER still uses object iternext and re-canonicalizes its box. Actual
dictionary forward/reverse VM adoption remains required.

Scope the real producer/helper/handler route with owning inputs, saved PC,
result publication, StopIteration, event polling, instrumentation and refresh.
Include allocating feedback prefixes rather than proving only a body in isolation.
No implementation is assigned by this record. Next: review a bounded design and
actual-route tests, then implement/test/review/commit.
