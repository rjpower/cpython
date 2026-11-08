# W5 startup boxes: round-3 review

**Must fix before integration.** The round-2 owner-clear change prevents the original double-close, but the `_io_open_impl` error label still calls `PyValue_CLOSE(path_or_fd)`. Once RawIO construction succeeds, the code clears that owner and can fail later (for example, text mode with `buffering=0`). `PyValue_CLOSE` asserts that its argument is not `PyValue_NULL`; this error path passes null in a debug build. Use `PyValue_XCLOSE(path_or_fd)` at the common error label. It handles both the pre-RawIO owned value and the post-RawIO null value without changing the value API contract.

The earlier round-2 approval missed this later cleanup path and is superseded for integration by this finding. Keep the earlier file as review history. A focused debug and release probe must exercise a failure after successful RawIO construction; the coordinator will run merged validation. A final source verdict follows the frozen patch correction.
