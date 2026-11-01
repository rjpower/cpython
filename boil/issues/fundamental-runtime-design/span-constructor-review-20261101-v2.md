# Supported native-call fixture correction review

2026-11-01, coordinator. **ACCEPT source `67190a9d81c683f7e1713300b9a99feb2bc85a93`
for fresh focused feedback.** Exact diff from 7f982b3 changes only the new C
test helper, its selected test and the experiment's scope note. Production
allocator, collector, generator and frame code are unchanged.

The helper now accepts an actual async Python function, roots it explicitly
across `PyObject_CallNoArgs`, pops the root on either outcome and returns the
call's result/error. It preserves function defaults. The old impossible
frame-constructor helper is removed, with all new usages renamed. The test
requires span admission, suspended/finished behavior and identity-preserving
payload survival after dropping the original function/default owner. Required
argument and wrong-function calls must raise TypeError.

Pinned upstream documentation explains the original invalid fixture; the
[first native outcome](span-native-outcome-20261101-v1.md) remains failed and
unchanged. No production assertion is weakened and the constructor case is
retained through a supported interface. The original demand for two usable
construction paths is corrected explicitly: the legacy frame constructor
cannot supply native coverage under the pinned upstream contract.

Root rehashed the source capsule and all its artifacts, including original
failure/cleanup/build bindings, and checked the actual clean three-file diff.
Capsule SHA256 `36a13842d5f64f7868184a46dc6c2c547791f1af94df073699bd0ea114340c86`;
source patch `dcd2aca1834f4158d03c07643f473541feb37a5f2352ccb1b49435e83e702b4b`.
Retained host checks report source-root/diff checks without errors, 21 ordinary
root-checker regression passes and one existing expected failure. The same
twenty selected methods and three unchanged child bodies parse. None ran in
this correction scope.

Fresh debug/release build directories and a new reviewed manifest/activation
are required. Original images and native receipts remain preserved. This
accepts a correction's source, not native semantics, storage performance or
upstream parity. All GC work remains bounded by 15:14:44 UTC.
