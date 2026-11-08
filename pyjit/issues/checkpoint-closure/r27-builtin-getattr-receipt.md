# Builtin getattr value continuation

The patch `r27-builtin-getattr-values.diff` applies to runtime `66981c963be`. The method accepts a value window, retains the existing arity and positional-only behavior, returns owned attribute results, and duplicates a default only for an absent attribute.

A locked W5 release shadow build and restoration each imported 117 modules with zero failures. The filtered `test_builtin.test_getattr` passed. The direct probe `r27-builtin-getattr-probe.py` matched upstream output for argument counts and keywords, inline and object invalid names, Unicode subclasses, inline receivers/defaults/results, properties, AttributeError versus other exception handling, and reentrant name lifetime. No full-suite or fresh box-count claim is made.
