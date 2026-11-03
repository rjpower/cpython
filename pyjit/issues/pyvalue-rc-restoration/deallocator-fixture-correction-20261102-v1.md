# Preserve the deallocator exception contract

At00:02–00:04 UTC the coordinator and independent reviewer checked the actual
debug py_dealloc wrapper in Objects/object.c and Doc/c-api/typeobj.rst698–716,
759–802. tp_dealloc must preserve the current exception, including its identity.
Destructor failures are unraisable. Py_DECREF_MORTAL still reaches the debug
guard. Allocator hooks injecting a new pending error during deallocation violate
that contract and would abort before an outer owner scope could observe them.

D retracts acceptance of the exceptional-cleanup witnesses in the native capsule,
retired-result-box and set-key free-hook fixtures. They were never executed.
Previous source archives, receipts and syntax results are retained; they do not
establish valid runtime failure coverage. Publication/ownership/reentry source
findings stand. Do not relax the debug guard or change production deallocator
semantics to make artificial tests pass.

Replace those injections with legal unraisable reports and exact preservation of
the entry exception. Capture the reports, assert completed operation/body once,
published state and legal callback/reentry ordering. A native body returning NULL
with an exception remains a real checked-error/abort route. Do not invent a
resource-cleanup API solely to force a failing destructor test.

C corrects its native C/Python fixtures under its unchanged00:10:49 ceiling.
Root corrects only Modules/_testinternalcapi/set.c's free hook/result assertions
and the corresponding test_pyvalue_set.py test/imports in a distinct20-minute
test-correction cut: conservative00:04:24 UTC, checkpoint00:14:24, ceiling00:24:24.
Set production storage is frozen. Focused AST and one fixture-TU syntax check,
then independent source review; candidate execution remains pending a coherent
runtime. This correction does not extend the original set source budget.
