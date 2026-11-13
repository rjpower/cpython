# Frame geometry and cost accounting

Source geometry is distinct from native work and elapsed time. Existing release
ELF DWARF was read without starting an inferior. `ptype /o` confirms selected,
F, R and N release geometry, plus F debug geometry. Other sizes remain
predictions until their ELFs are available. All sizes exclude the trailing home;
`sizeof(_PyInterpreterFrame)` includes one additional eight-byte declared home.

| GIL frame | Header / FRAME_SPECIALS_SIZE | Source work removed | Recurring work added | Evidence |
| --- | --- | --- | --- | --- |
| Selected51 |112 bytes /14 words | Baseline | Baseline | Existing selected release ELF |
| F: feedback derivation |104 bytes /13 words | One cached entries load/store at setup; one pointer copy/refresh store | Dependent entries load per feedback access | Option2 release and debug ELFs; sizeof112; return_pc88, recipient96, owner100, homes104 |
| R: recipient proof |112 bytes /14 words | Empty-recipient old-home load/retire classification | Prefix local-boundary/encoding at producer, kind branch and numeric masks | Existing option1 release ELF; sizeof120; geometry unchanged |
| N: namespace derivation |96 bytes /12 words | Two source environment reads/stores and copied cache words | Function decode plus dependent namespace load at each use; cold root/tool checks | Existing option3 release ELF; sizeof104; return_pc80, recipient88, owner92, homes96 |
| L: mapping on frame object |104 bytes /13 words predicted | Ordinary NULL mapping store and header word | Eager variable-tail frame allocation for supplied mappings, mapping indirection, retirement record handling | Source review pending |
| S: numeric same-home overwrite |112 bytes /14 words predicted | One admitted temporary NULL store and redundant path tests | Variant selection/text; entry fallback if needed | Source proposal; no new layout |
| Upstream |80 bytes /10 words | Different stack/value model | Different stack/value model | Existing upstream release ELF; sizeof88 |

Header words occupy the downward side of a shared thread arena; their removal
does not remove a malloc per ordinary call. Heap generator/frame objects have
variable owning-home tails. `co_framesize` and deepfreeze retain all physical
home capacity when only a fixed pointer disappears. Hidden constants, NULL
consumption, overlapping arguments and initialized frontier remain unchanged.

Feedback debug uses its spare five flag bits for LLTrace and has the same
104-byte home offset as release. Free-threaded alignment is not established by
these GIL ELFs. Combined F+N+L predicts80 bytes only after coherent integration;
that would not establish upstream behavior, native instruction equivalence or
speed. No cost row is a performance claim or a measured removable fraction.
