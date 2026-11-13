# Frame geometry and cost accounting

Source geometry is distinct from native work and elapsed time. Existing release
ELF DWARF was read without starting an inferior. The isolated component,
validated S2, R2 and combined GIL layouts are verified. Compact F+N+L's actual
release ELF has homes at 80 and sizeof88; this replaces its earlier prediction.
All header sizes exclude the trailing home; sizeof(_PyInterpreterFrame)
includes one additional eight-byte declared home.

| GIL frame | Header / FRAME_SPECIALS_SIZE | Source work removed | Recurring work added | Evidence |
| --- | --- | --- | --- | --- |
| Selected51 |112 bytes /14 words | Baseline | Baseline | Existing selected release ELF |
| F: feedback derivation |104 bytes /13 words | One cached entries load/store at setup; one pointer copy/refresh store | Dependent entries load per feedback access | Option2 release and debug ELFs; sizeof112; return_pc88, recipient96, owner100, homes104 |
| R: recipient proof |112 bytes /14 words | Empty-recipient old-home load/retire classification | Prefix local-boundary/encoding at producer, kind branch and numeric masks | Existing option1 release ELF; sizeof120; geometry unchanged |
| N: namespace derivation |96 bytes /12 words | Two source environment reads/stores and copied cache words | Function decode plus dependent namespace load at each use; cold root/tool checks | Existing option3 release ELF; sizeof104; return_pc80, recipient88, owner92, homes96 |
| L: mapping on frame object |104 bytes /13 words | Ordinary NULL mapping store and header word | Eager variable-tail frame allocation for supplied mappings, mapping indirection, retirement record handling | Matching release/debug ELFs; sizeof112; see option4 post-review |
| S: numeric same-home overwrite |112 bytes /14 words | One admitted temporary NULL store and redundant path tests | Variant selection/text; entry fallback if needed | Matching release ELF; sizeof120; see option5 post-review |
| S2: validated overwrite |112 bytes /14 words | One certified temporary NULL store; four repeated geometry checks removed | Cold refinement admission and complete selection bounds; extra variant text | e95328ba1d6 matching release/debug; sizeof120; variant296 bytes/rsp32 |
| R2: no fresh recipient |112 bytes /14 words | Original R's invented slot-zero default encoding; retains empty-recipient old-home removal | Real producer classification/masks remain; cold public-constructor correction | fcc82220ddd matching release/debug; sizeof120; exactCALL989 bytes/rsp80 |
| Combined R2+S2 |112 bytes /14 words | Both qualified component mechanisms retained | Same selection and real producer dependencies; no extra release hot predicate | f3c464a54c8 matching release/debug; sizeof120; variant296/exactCALL989; native review confirms sequences |
| Compact F+N+L |80 bytes /10 words | Feedback, two namespace and mapping cache words | Combined derivation dependencies and mapping allocation/lifetime work | 82afe7aef2d actual release ELF; sizeof88; rejected performance candidate |
| Upstream |80 bytes /10 words | Different stack/value model | Different stack/value model | Existing upstream release ELF; sizeof88 |

Header words occupy the downward side of a shared thread arena; their removal
does not remove a malloc per ordinary call. Heap generator/frame objects have
variable owning-home tails. `co_framesize` and deepfreeze retain all physical
home capacity when only a fixed pointer disappears. Hidden constants, NULL
consumption, overlapping arguments and initialized frontier remain unchanged.

Feedback debug uses its spare five flag bits for LLTrace and has the same
104-byte home offset as release. Free-threaded alignment is not established by
these GIL ELFs. Compact's measured 80-byte header does not establish upstream
behavior, native instruction equivalence or speed. The coordinator rejects
the performance candidates; selected51 remains selected while the separate
cold correctness candidate undergoes qualification. No cost row is a performance claim or
a measured removable fraction.
