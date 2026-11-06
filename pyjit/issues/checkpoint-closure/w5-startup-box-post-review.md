# W5 four-box startup patch: round-2 post-review

Reviewed the frozen [`w5-startup-boxes.patch`](w5-startup-boxes.patch), generated
against runtime commit `58db4324fec`, after the successful RawIO branch
was changed to `PyValue_CLEAR(path_or_fd)`. **Approved for the Linux R27
startup gate; no remaining must-fix finding in this patch.**

The first version closed `path_or_fd` without clearing it, so a later
allocation or wrapper failure could jump to `error:` and close the owner a
second time. The corrected code clears it before later work. Numeric inputs
now retain their exact incoming value and object number subclasses keep an
owned reference. Nonnumber inputs still pass through `PyOS_FSPath`; the
result is held through validation and the RawIO call. The four call-window
entries and fixed RawIO class have balanced owners. Both fixed RawIO types
have `Py_TPFLAGS_IMMUTABLETYPE` and return heap instances, which justifies
the successful result's known-object projection.

`dict_value_keyerror` now calls the existing `_PyErr_SetKeyErrorValue`,
which builds a KeyError args tuple holding the inline word. The Windows
console probe directly handles inline integer descriptors, clears the same
conversion errors as the old object probe, and delegates object numbers and
paths to that probe. Its subsequent WindowsConsoleIO constructor is still
object-convention Clinic and may box an inline descriptor inside the type
call adapter; Windows zero-box admission needs a separate check if required.

The author reports release compile, focused dict and IO tests plus fd,
pathlike and opener probes, and a fresh stats `-I -S -c pass` run reducing
the four startup CORE boxes to zero. I did not rerun those W5 builds.
