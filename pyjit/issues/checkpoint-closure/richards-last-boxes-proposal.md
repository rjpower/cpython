# Richards startup boxes: exact attribution and port recipe

The retained `build/w5-out/oserror-richards.json` was produced by
`build/w5-stats/python` on 2026-11-05. Its low and high process samples each
contain three CORE parser boxes at `0x5d6275` and one CORE dict box at
`0x1d3ee4`. The per-loop difference is zero. A controlled
`w5-stats/python -S -X pystats -c pass` reproduces the same four counts, so
these are process startup/shutdown costs, not Richards loop costs.

`addr2line` resolves `0x5d6275` to `_io_open_impl`,
`Modules/_io/_iomodule.c:233`. The inline descriptor argument for each of
standard input, output and error is boxed into `PyObject *path_or_fd` before
the RawIO constructor call.

`0x1d3ee4` resolves only to the out-of-line `PyValue_AsObject` copy in
`dictobject.c`. A one-shot in-process breakpoint on that exact instruction
captured its call stack in the same stats binary:

```
dict_value_keyerror (Objects/dictobject.c:2573)
delvalue_knownhash_lock_held -> PyDict_DelValue
remove_subclass -> remove_all_subclasses -> type_dealloc
delete_garbage -> gc_collect_main -> finalize_modules
```

The key is an inline int. `remove_subclass` suppresses this missing-key
exception during shutdown, but `dict_value_keyerror` boxes the key just to
construct it. Replace that helper's object projection, `_PyErr_SetKeyError`
call and DECREF with `_PyErr_SetKeyErrorValue(key)`. That function is already
used by two other paths in `dictobject.c`; it stores the key's value word in
the exception's args tuple and retains the existing KeyError behavior.

For `_io_open_impl`, keep `path_or_fd` as an owned `PyValue`: duplicate a
number argument, or wrap the owned `PyOS_FSPath` result for a nonnumber. Keep
the current nonnumber str/bytes check and `%R` error. Build a four-element
value-call window containing the held `path_or_fd`, an owned Unicode
`rawmode`, the bool, and the opener, then call `_PyValue_Vectorcall` on the
fixed `RawIO_class`. Close the temporary window owners after the call, and
close `path_or_fd` where the old code released its object. The fixed FileIO
type's value call passes the tuple's value words to its generated
`__init___value`; `fileio_new` ignores the arguments. The successful raw
result is therefore a heap object and can be taken with
`PyValue_AsKnownObjectSteal` after a null check.

On Windows, `_PyIO_get_console_type` still takes `PyObject *`. Its numeric
descriptor path needs a value-aware branch that extracts an inline int and
calls `_Py_get_osfhandle_noraise` and `_get_console_type` directly. Keep the
existing object path for object numbers and filesystem paths, including its
error-clearing behavior; an inline float cannot designate a console. This
avoids moving the descriptor box into the Windows-only dispatch.

Focused validation: the old and new stats builds should each run a fresh
`-S -c pass` process and compare the four site counts. Also exercise
`open(fd, 'rb', closefd=False)` for 0/1/2 with a safely controlled pipe or
duplicate fd; a numeric subclass; an `__fspath__` provider returning a
string or invalid type; a custom opener; and Windows console dispatch where
available. Run `test_io` once at integration. No benchmark roster is needed
for this four-box source change.
