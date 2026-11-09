# Canonical-home duplication patch peer

Reviewed the actual canonical-home-duplication-full.patch artifact without
applying or executing it. Revised source is approved for a separately admitted
direct application and host/native gates.

The single result fact and all runtime generator/test callers are renamed
coherently. The new proof requires the exact one-uop pure local-duplicate body,
one logically fresh owned output, matching LOCAL_IN/immediate field, no input
owners, no named cache bindings and no preprocessor ambiguity. Exact tokens
exclude hidden local helper rebinding or callback/suffix work. Physical source
and destination aliasing remains valid because duplication precedes replacement.
Both identical LOAD_FAST and LOAD_FAST_BORROW bodies qualify without a name gate;
the four numeric bodies continue through their existing proof unchanged.

One proposed negative originally nested GETLOCAL inside PyValue_AsObject, which
slot_uses rejects before the canonical proof. The artifact now uses direct raw
GETLOCAL assignment, an analyzable non-duplicated source. This preserves the
negative's purpose instead of expecting a normal false result after an unrelated
format error. The renamed, cache and directive variants and analyzer API calls
were reviewed statically; they still need actual host execution after admission.

Combined generation must account separately for three new instrumented provider
bodies and two local-load bodies, preserve the earlier ten observed and four
numeric bodies, and leave metadata unchanged by the canonical-home proof. No
performance result follows from source approval.
