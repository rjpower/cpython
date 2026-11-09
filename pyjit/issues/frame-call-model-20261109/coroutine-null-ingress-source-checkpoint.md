# Coroutine first-send NULL ingress source checkpoint

The isolated N patch changes only `Objects/frameobject.c` and
`Lib/test/test_code.py`. The existing NULL worklist now takes an entry PC.
Ordinary optimized functions still seed pc0; only an exact optimized coroutine
with `RETURN_GENERATOR` NoDst/B=nlocalsplus at pc0 and `RESUME` at pc1 seeds
pc1. Closure shapes, an ordinary EH covering pc0, a source-error record from
pc0, or another predecessor of pc1 withhold this optional proof. They do not
invalidate code or change normal execution. The graph, error-edge fact loss,
backward requirements, stopped phase, saved-local rules and setter transaction
are unchanged.

The focused test checks the encoded entry shape, cold and warm first sends
through native `send(None)` and VM `await`, one real PRE line redirect, stable
code bytes, and non-None CREATED-send refusal. A constructor-valid EH entry
covering pc0 is a structural negative: its normal send completes, but its
redirect must refuse with unchanged PC. The test deliberately does not throw
into that forged handler. No post-yield or async-with success is claimed.

`git diff --check` passed. The configured shared UV cache was read-only to
this worker, so its AST host check could not acquire a temporary cache file.
Native compilation and test results are pending the coordinator's gate.
