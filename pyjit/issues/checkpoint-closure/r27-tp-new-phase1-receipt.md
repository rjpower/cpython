# Direct value constructor phase 1

`r27-tp-new-phase1-combined.diff` is the consolidated, apply-checked patch; the earlier draft and subsequent fix and test layers are retained separately. It adds the cold `new_` pair, generated slot metadata, int and float providers, the Python-defined constructor pair, and guarded direct `__new__` dispatch. The phase-two general type-call and initializer continuation is mandatory before constructor integration is complete.

A locked W5 release shadow build and restoration each imported 117 modules without failures. Sixty-five filtered int, float and descriptor tests passed with one skip; `test_value_slots` passed 242 tests with three skips, including an added constructor pair inheritance and override test. Direct constructor and error probes matched upstream. A C++17 syntax check of `Python.h` passed. Earlier runs found and corrected a stale slot count, the C++ reserved field spelling, a shadow-base dictionary header mismatch, the test kind enum, and three test inventory expectations.

Debug paired-row validation, an independent object-C `tp_new` probe and a fresh private stats measurement remain for merged qualification; no full-suite or performance claim is made here.
