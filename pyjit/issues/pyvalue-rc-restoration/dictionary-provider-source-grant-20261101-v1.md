# Internal owning dictionary providers

The coordinator independently reviewed C's concrete provider design against the
actual value-native lookup, internal declarations and set consumers, and accepts
the first owning-read cut. Proposal SHA256
`73b81d05f59bd9e0c10fe8f3fa6d0cfb8fe1cfd87ff6d45d4f00e36e88339129`,
design receipt `4d71f46aa726393729757fae77be09757cc57e6889b2cd3a62403bcbafe1ff8f`.
value_api_consumer_map independently reviews the implementation.

The earlier dictionary clear/release increment froze and passed review at
`bfa9814475be8f16b989704f6c91beeeaac1fe1a27e127461ad5847c38df6803`.
Close that original source window early. This is a distinct 30-minute provider
cut; it does not extend the original mutation/storage or native fixture budgets.
Use conservative start 2026-11-01 23:51:48 UTC, checkpoint 2026-11-02 00:06:48,
ceiling 00:21:48. Record the later actual first edit too.

## Exact scope

- Objects/dictobject.c: add only _PyDict_GetValue_KnownHash and
  _PyDict_NextValue bodies over the existing value-native storage/lookup.
- Include/internal/pycore_dict.h: ownership/status comments for those existing
  declarations; no unrelated declaration changes.
- New Lib/test/test_pyvalue_dict.py: focused owning-provider behavior through
  the real set-from-dictionary update/difference consumers. Drafted tests are
  not runtime evidence before mutation/storage closure makes a candidate work.

Known-hash output is independently owned on success and NULL1 on miss/error.
Pin the query before comparisons; take the found owner before closing its pin.
Check both lookup status and pending error. Preserve the primary/first-new
cleanup failure and close any provisional result on failure. Never reuse an
entry/table pointer after callbacks. Inputs are canonical language values.

NextValue captures requested strong key/value outputs and hash without boxing
or callbacks, preserving split insertion order and combined hole skipping.
Only then advance the cursor. Optional outputs acquire only requested owners;
non-success clears requested outputs. Invalid dict returns a real error.

Leave legacy getters, iteration APIs, declarations and implementations untouched.
The rejected port_getters.py script or an equivalent API removal is outside
this grant. No scalar cache or borrowed boxed scalar return is introduced.
This additive internal provider cut does not solve the old bridge contract.

Review exact source fragments, lexer/AST, focused test logic and actual owning
set consumer calls. No compiler, object, link, candidate execution or benchmark
is part of this cut. Whole dictionary closure still needs mutations/watchers,
copy/resize/managed storage, deallocation/views and legacy/native consumers.
