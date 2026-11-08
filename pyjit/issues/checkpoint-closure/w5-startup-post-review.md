# Structseq and weakref startup increment

Reviewed the actual five-file W5 diff in `Objects/structseq.c`,
`Objects/dictobject.c`, `Include/internal/pycore_dict.h`, `Modules/_weakref.c`
and `Lib/test/test_weakref.py`. No must-fix finding. The private focused run
reported 28 structseq tests and the new weakref case passing; the coordinator
will validate the integrated build.

`get_type_attr_as_size` now converts its owned dict value with
`_PyValue_IntAsSsize_t` and closes it. That is the extractor used by the former
`PyLong_AsSsize_t` object wrapper, so integer type and overflow behavior stay
the same. The new dict entrypoint retains the borrowed key immediately inside
the existing lock-held helper. It keeps the selected key/value owners,
predicate, watcher notification, relookup and conditional deletion unchanged.
The old object entrypoint delegates through the new lock-taking API, preserving
the single critical section. `_weakref` passes its Clinic borrow directly,
removing the parser-site box while retaining atomic predicate/deletion behavior.
The added test exercises an inline integer key with both live and dead weakref
values.
