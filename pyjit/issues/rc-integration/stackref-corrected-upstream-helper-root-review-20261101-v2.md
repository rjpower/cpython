# Corrected upstream control helper review

2026-11-01. **ACCEPT exact held v2 helpers**, source d2ca4122333cf89ef97dcb4904cafd61a02308c7,
for one comparison only. Root independently rehashed5,833 input records,
36 capsule artifacts and all tool/path/mode pins;12 actual host controls pass.
Only two host git calls occurred, zero target or compiler queries.

The prior accepted run_row and pidfd sweep bodies are AST-identical. Standard
target, three-method roster, result contract, controller, profile witness and
gate bytes are unchanged. Configuration remains GCC/computed/mimalloc/O1 debug
with Py_STACKREF_DEBUG, without Boil IR/shapes, ASan or CLOSE_DEBUG. New bindings
select the separately reviewed one-assignment source and a fresh build directory.

Canonical serialization is now enforced and held by the runner itself through
cleanup. Launch it directly; do not wrap it in another flock on that same lock.
Both machine slots and the per-build gate lock remain. Cleanup, idle checks
and receipts now cover Python, _testembed, _bootstrap_python and _freeze_module.
Missing prospective files are legal; actual PID/startticks/executable and pidfd
checks remain authoritative.

The same fixed two rows fit one future1800-second inclusive window. There is
no activation yet. All three methods must complete without skip or missing
record; standard failures remain comparison data. Source inspection confirms
one XML record per method even when threading's subtests fail: the stock result
may leave that record unnamed, which the unchanged reader already handles.
No full upstream sweep, further source correction, gate amendment or RC
qualification follows automatically.

Pins: plan ec427a7285ea4fc715736c7bfdadae8ca67d20c07e44a25a798c43047ab1e855;
manifest472763d6a6aa589ec7ba80ba2f8e23ac064539b50905ecc137bc2a2884b2294b;
capsule882a0c01fed79a9371d537f3f36dbdb89976830485c2aa196e73fc794df0525a;
ready555803e0261ffbcc0ec910e95ed0094e669237c40a1c9de82d1f4326b5a37645.
Independent receipt `build/upstream-stackref-helper-root-review-20261101-v2/receipt.json`
SHA c76518e79ee5c986e04bd0735866b2de0e66349b2cbe64944fe6ddedd06ca6ac.
Native activation and actual-outcome adjudication remain separate.
