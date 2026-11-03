# Datetime value readers

Checkpoint52 `67210b9cf67` ports twelve functions and removes all 23 old tuple
getter uses, including three variadic uses that produced no compiler error.
Stored values retain the canonical representation. Fallible owning native views
stay alive through their semantic users and retire with the first error protected.

Sol designed the ownership and branch recipes. Luna applied bounded ISO,
numeric and pickle packets and prepared their exact composition. Root executed
the configured native checks and copied test discovery; independent Sol review
accepted actual code and evidence before root committed the two-path change.

- [Composition and commit](../../../../../build/pyvalue-datetime-readers-composed-20261103-v1/commit.json)
- [Independent final review](../../../../../build/pyvalue-datetime-readers-composed-20261103-v1/independent-solshared-composed-review.md)
- [Coordinator review](../../../../../build/pyvalue-datetime-readers-composed-20261103-v1/coordinator-review.md)
- [Native dependency comparison](../../../../../build/pyvalue-datetime-readers-native-20261103-v1/dependency-comparison.json)

The matched full-file probe improves from nineteen errors/three warnings to
zero diagnostics; the actual object builds. All 197 common dependencies stay
identical, with no additions or removals. Seven discoverable controls pass on
reference Python 3.14.4 without skips. This is reference evidence: the restored
candidate is not linked and has executed none of these tests. Runtime, lifetime
and fault qualification remain required after linking.

The merged v18 build includes this checkpoint and removes exactly nineteen
error observations, leaving 280 errors and no executable. See the
[merged build](../../merged-build/status.md) and [porting queue](../../porting/status.md).
