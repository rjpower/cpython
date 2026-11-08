# `_operator` value-call post-review, round 1

Reviewed the frozen `w5-operator-value-call.patch` against S `e983958bede`. The paired object/value vectorcall layout follows `functools.partial`. Itemgetter's exact-tuple path returns an owned value, attrgetter's dotted path holds and closes each intermediate across callbacks, and methodcaller keeps owned receiver and positional values through dispatch. The keyword window owns its dict values and is freed after the call. Repr/reduce retain their result shapes.

**Must fix:** `methodcaller_repr` obtains an owned `arg_v` with `PyTuple_GetValue` and passes it to `_PyValue_Repr`, but never closes `arg_v`. The leak occurs on both successful and failing repr. Close it immediately after `_PyValue_Repr`, before testing the returned object. Luna is preparing a separate additive fix; this original patch remains unapproved until then.

The `_operator` translation unit compiled in S. A coherent runtime build and focused `test_operator` result were unavailable at this review point because S's full build had unrelated Parser failures. Include a repr-error and keyword mutation probe in the eventual focused check.
