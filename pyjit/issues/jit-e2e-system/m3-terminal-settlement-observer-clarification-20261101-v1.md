# Post-cold observer synchronization

Additive clarification to proposal SHA `24730fd2bb2116e8c457b6271e1b68006d731c37959c321430d6150705e60667`; its bytes remain unchanged.

The fresh admission can occur after the original RESUME has already performed its service. Cold construction and temporary-pin release can reenter and change monitoring epoch. After release, refetch and validate the linked current frame, code, exec and word backing. If the code's instrumentation epoch differs from the interpreter's current epoch, call the existing `_Py_Instrument(code, interp)` once, preserve any error, then revalidate those identities. Reconstruct the pending PC from the saved logical word only after these checks. Active observer policy declines to ordinary interpretation using the current instrumented handler at that exact pending word. Do not replay RESUME service, periodic polling, the pending semantic operation or cold construction. An unexpected changed backing is an explicit error; no stale raw PC is used for dispatch or native entry.

This adds no observer policy, source-word change or admission retry. It closes the callback boundary introduced by the proposed cold settlement.
