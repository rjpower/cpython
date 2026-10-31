# Due-major trigger: independent test-correction design review

**ACCEPT the proposed narrow test-only correction for source preparation.**
Replace the due-major test's `types.CellType('trigger')` with `[]`, and its
post-stress trigger-payload assertion with empty-list equality. Keep every
collector-kind, count, old-cycle and rooted-cell assertion. No runtime,
threshold, loop, observer or expected-count relaxation is justified. The
coordinator owns the source grant and separately reviewed remaining execution
under the original 00:14:59.201344 UTC deadline.

Read-only inputs are frozen source `5cc1e5310e2cb5f11feade09195b5a3695188161`
and `build/nursery-cadence-precise-feedback/runs` in the nursery-adoption worktree.
The focus result is SHA256
`22da3da33ad76202b73af5cc447c66ee6e07d0fb4b29b11b220d3e683dd90863`,
with native return 2 and no qualification pass. The child observer file is
`b0454a9b7d3a42bc31ecaa51893aa7daeafe9159ff3780c7f907bc3db1ee9298`.
It binds this method's successful every=0 child body
`122f7f343db5cb7f5f83cf92c72de16031c2a21f410e50e388a2f0e766a31e0e`
and failing every=1 body
`57315fc8464ab9b78871f85c95196331ff2dee5094cdc145978a98da92fd4edd`.
The failure is the unchanged minor-collection-count assertion. This review
does not convert that failure into native success or claim measured allocation
counts from the logs.

## Source proof

`Objects/call.c:_PyObject_MakeTpCall` constructs positional arguments through
`_PyTuple_FromValues`. CellType has the non-vectorcall `cell_new` constructor,
which then calls `PyCell_New`; that function separately calls
`PyObject_GC_New`. Thus the Python call is not a single-allocation trigger.
Under stress1, the due major can complete at its argument tuple and the cell
allocation can legitimately reach a new epoch's minor opportunity before the
Python code disables collection. Inline collection defers Python callbacks to
the eval breaker; the callback's disable is not an inter-allocation guard.

The existing generated BUILD_LIST handler calls `_PyList_FromValues`. For an
empty literal, its n=0 branch at `Objects/listobject.c:3049` directly returns
`PyList_New(0)`. `PyList_New` unconditionally calls `PyObject_GC_New` and sets
`ob_item=NULL` for size0. **There is no list-object freelist branch in this
fork's constructor.** It allocates no item buffer and makes no Python call or
argument tuple. `_PyObject_GC_New` calls `gc_alloc` once; that reaches
`_PyObject_MallocWithType`, then one `PyObject_Malloc`, whose one
`gc_count_alloc(size,1)` performs the stress probe before allocator dispatch.
Underlying allocator reuse therefore cannot bypass this accounting. Type-root
push/pop and list initialization/tracking add no second object allocation on
this normal constructor path.

This is a stronger trigger for the intended oracle: one due object-allocation
opportunity, rather than a compound call that can enter two collection epochs.
The existing due-debt assertion, major-count increase, unchanged minor count,
all-start-events-major requirement, old-cycle reclamation, rooted cell payload
and retained byte block remain necessary. `assert trigger == []` runs after
GC is disabled and stress reset; its comparison literal cannot affect the
measured collection interval. Keep both every=0 and every=1 children.

Exact source review must confirm only this nominated trigger, its explanatory
comment and its corresponding result assertion change. Native behavior remains
unexecuted for the corrected source; preserve the original failed focus and
reuse the compiled image only through an exact test-only bridge.
