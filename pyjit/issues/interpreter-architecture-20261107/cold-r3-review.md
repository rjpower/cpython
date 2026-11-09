# Cold R3 review: Python call transfer and empty varargs

2026-11-07. **Decision: accept** integrated runtime `225480a4250` on parent
`25ce1654801`. I found no must-fix semantic, ownership, callback-order, or
frame-state defect in the actual two-commit diff (`25ce1654801^..225480a4250`).
This decision does not claim an elapsed-time improvement.

The R2 commit moves the unchanged consuming body into
`Py_NO_INLINE call_transaction_push_python_fallback()`. Its wrapper returns an
overlap failure (`-1`) or published child (`0`) directly, and reaches the
consuming body only on decline (`1`). `call_transaction_invoke()` still performs
method/EX preparation and monitoring before selecting a Python provider; a
declined Python provider can reach native dispatch once. In the overlap path,
fallible keyword-dict allocation and revalidation precede detachment. The
transfer then has no callback or failure edge before making the child current.
The fallback still copies parent inputs before the consuming binder, refreshes
after binding, detaches only after a viable child, publishes the child before
retiring parent owners, and restores the parent before clearing a failed child.
The `Py_GIL_DISABLED` overlap branch still returns `1`, so it uses that fallback.

The R3 runtime edit replaces borrow-and-duplicate of
`&_Py_SINGLETON(tuple_empty)` with `_PyValue_STATIC_IMMORTAL` for the same child
`CO_VARARGS` slot. The singleton is initialized with `PyTuple_Type` and
`_PyVarObject_HEAD_INIT`; the GIL header starts at the immortal refcount and
the free-threaded header at `_Py_IMMORTAL_REFCNT_LOCAL`. Exact tuple cannot
take the number-inline branch in `PyValue_FromObjectBorrow()`, so the old value
was the same pointer with `Py_TAG_REFCNT`. `PyValue_DUP()` took no count for
that tag, and `PyValue_CLOSE()` takes none either. The new value therefore
retains canonical object identity and the same ownership and cleanup behavior.
This proof is specific to the static empty-tuple singleton; the edit adds no
representation or general initializer rule.

The added `test_call` assertions check the same empty `*args` object across
repeated direct calls and a generator-owned caller. The existing call tests
exercise default arguments, distinct empty `**kwargs`, retained and suspended
frames, lifetime and unwinding, tracing, monitoring and callback/error order.
The direct second call can use the warmed overlap path; the generator-owned
caller exercises the fallback branch. These are behavioral checks, not a
runtime counter proving how often either branch ran. The focused release and
debug logs each report 916 tests, one skip, and success across five files;
their builds each checked 117 modules. `r3-identities.json` ties both images
to source `225480a4250` (release build ID `aab32ced7412daa2f21d2640a3f3dda9ddcfeafd`;
debug `b5d0dc4c36ed8e5d01f0a8822231e249f6e30531`). I ran no build, test,
or native workload for this review.

In the retained GCC release ELFs, the overlap symbol shrinks from `0x881`
bytes at R2 to `0x86e` at R3; the fallback stays `0x917` and
`call_transaction_run_vector` stays `0x241`. R2's empty-varargs sequence tests
the tuple type and immortality and conditionally increments a refcount; R3
stores the tagged singleton address directly. GCC also changes register
allocation and grows the overlap stack reservation from `0x58` to `0x68`, so
the byte-size delta is not a direct count of useful work removed. The source
removes that conversion work from the empty-varargs accepted branch; elapsed
impact, including any effect on other call shapes, remains unproven. The basic
matched screen was active at review time, and full merged validation remains
the coordinator's later gate.

**Limited abstraction feedback:** the split puts the consuming array and its
cleanup behind a clear boundary without changing the public call contract.
The static initializer is an appropriate local constant for this singleton.
Keep both as bounded improvements; further call-path simplification needs an
observed hot-path cost and a separate ownership proof.
