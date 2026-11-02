# Object, attribute and repr composition

- Stage: full pending object composition independently reviewed and committed as `e52539e7f68`
- Authors: earlier canonical providers; coordinator bridge/setter; wiki_audit final corrections
- Independent final reviewer: state_audit

The canonical attribute/method providers, generic object getter/setter, startup
borrowed reduction cache and repr guard now compose in one reviewed object.c.
Generic scalar attributes use fallible owning views. Operation errors retain
priority across owner retirement; later cleanup cannot fall through to another
lookup or return nominal success with an error. Dictionary missing-key conversion
keeps descriptor setters’ original errors. Repr entry stops on errors and preserves
the primary across guard/candidate cleanup; best-effort ReprLeave remains intact.
The unused object-only method helper and its private declaration were removed.

Whole-file review covered the earlier held changes and the new corrections;
132 other original function bodies remain exact. This is the first full object.c
checkpoint, not blanket acceptance of the still-held typeobject.c. Four paths
are committed, including fifteen attribute and five repr candidate test methods.

Actual full TU and configured object pass with zero diagnostics on 308 stable
header/config/source/test inputs. All twenty host controls pass without skips.
Twenty candidate methods and allocator/reentry qualification remain unexecuted;
no restored interpreter links. The initial review’s three concrete findings and
initial host discovery failure remain in the evidence.

[Initial design](../../../../../build/pyvalue-object-consumers-20261102-v1/design.md),
[initial composition findings](../../../../../build/pyvalue-object-consumers-20261102-v1/independent-code-review.md),
[final independent acceptance](../../../../../build/pyvalue-object-final-amendment-20261102-v1/independent-code-review.md),
[actual native and host checks](../../../../../build/pyvalue-object-final-amendment-20261102-v1/native-v1.json),
[commit receipt](../../../../../build/pyvalue-object-final-amendment-20261102-v1/commit.json).
