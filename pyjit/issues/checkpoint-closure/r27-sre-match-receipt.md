# SRE match value subscript

The reviewed `r27-sre-match-getitem.diff` applies to runtime `af82c03c4e5`. It pairs the match type's value subscript with its object thunk and removes the key projection in the shared group-index lookup.

A locked W5 release shadow build and restoration each checked 117 modules with no import failures. Four filtered `test_re` group and getitem tests passed. `r27-sre-match-probe.py` exactly matched upstream output for group, start, end, span and subscription with normal and named groups, int and bool keys, int subclasses and custom `__index__`, negative and huge indices, floats and unhashable keys, hash/equality callbacks, raised index exceptions, unmatched groups, and bytes. No full-suite or fresh box-count claim is made.
