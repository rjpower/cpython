# Value-native id audit arguments

The `7baf96ed017` full-suite census attributes 1,568,829 core parser boxes to `builtin_id_impl` when audit hooks are active. The identity computation is already value-native. The coordinator proposed and Sol peer-reviewed replacing the temporary numeric object with an owned one-element value tuple passed to existing `PySys_AuditTuple`. It retains the no-hook fast path, event name, argument shape and exception propagation. The tuple roots the id across arbitrary hooks; the local id remains owned until return or error cleanup.

The source packet is generated against `7baf96ed017`. A stats regression checks the numeric identity and one-int event argument, hook exception propagation, and zero core/numeric boxes. Merged compilation and test execution remain pending.
