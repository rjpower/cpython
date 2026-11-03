# Core type mutation and retirement

- Stage: independently reviewed dependent component; six files committed as `7b923d2b935`
- Author: wiki_audit; independent reviewer/integrator: coordinator

The positive-owner scope closes cache/version admission through actual invalidation,
writes and displaced-owner retirement. Descendant invalidation restarts owning
selection after callbacks. Version/specification borrows clear and the cache
detaches before watcher notification, with table retirement afterward. Actual
metadata setters reacquire owned namespaces and stop on pending errors. The Dict
shared-key writer keeps its scope through publication and keys/type retirement.

Seven paths were reviewed; the typeobject-core1-only.patch remains held alongside
other typeobject work. Six independent dependencies are committed. Three full
TUs (Dict/cache/fixture) pass; real objects build, retaining three unrelated Dict
unused-function warnings. Full typeobject still failed with 49 errors/39 warnings,
all attributed to unchanged bodies/lines. Nine host controls pass without skips;
22 candidate methods remain unexecuted at this endpoint.

This does not admit the runtime. Static publication, graph/MRO/construction,
guard/payload/terminal and scalar providers remain required. Subsequent graph
repairs have separate frozen evidence; do not silently rebind this receipt.

[Design grant](../../../../../build/pyvalue-typecache-core-scope-20261102-v1/coordinator-review.md),
[code/results review](../../../../../build/pyvalue-typecache-core1-20261102-v1/coordinator-code-review.md),
[native checks](../../../../../build/pyvalue-typecache-core1-20261102-v1/native-v1.json),
[attribution](../../../../../build/pyvalue-typecache-core1-20261102-v1/native-classification.md),
[commit/pending delta](../../../../../build/pyvalue-typecache-core1-20261102-v1/commit.json).
