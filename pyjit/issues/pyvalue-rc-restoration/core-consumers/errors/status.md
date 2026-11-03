# Exception matching and unraisable-hook consumers

- Stage: independent code/result review accepted; committed as `d1c8041c78c1a1811d05978b5649fd59a711b37a`
- Owner: coordinator implemented; wiki_audit reviewed design and code/results
- Scope: two Python/errors.c functions and one focused seven-method test file

Canonical tuple reads avoid boxing or scalar pointer decoding under pending
errors. Five independently owning hook fields survive write/repr/stderr
callbacks, then reverse cleanup preserves the primary/first cleanup error.
All other source bytes are exact. No VM invocation contract changes.

Full new-config TU syntax and actual errors.o compilation pass without warnings;
305 header/source inputs stayed stable. Five complete host hook methods pass.
Host3.14 lacks the newer matching C-test helper: two initial failures remain
preserved, with separate controls calling the real host C API through ctypes.
All seven candidate methods remain unexecuted; pending-error C injection/OOM
have source reasoning only. No linked restored runtime or admission claim.

[Design](../../../../../build/pyvalue-errors-consumers-20261102-v1/design.md),
[results](../../../../../build/pyvalue-errors-consumers-20261102-v1/results.json),
[review](../../../../../build/pyvalue-errors-consumers-20261102-v1/independent-review.md),
[commit](../../../../../build/pyvalue-errors-consumers-20261102-v1/commit.json).
Next: execute focused candidate tests after coherent linking.
