# Dict and frozendict getitem method values

The frozen patch `r27-dict-getitem-values.diff` applies to runtime base `af82c03c4e5`. Both method descriptors use the existing checked fixed-arity value trampoline and the existing value subscript implementation; the object C entry and subscript slots remain unchanged.

A locked W5 release shadow build and the restored build each checked 117 modules with no import failures. The final run of `test_dict` passed 154 tests, and `r27-dict-getitem-probe.py` passed direct dict and frozendict calls, subclass missing-key handling, invalid receiver and arguments, and a raising hash. An earlier command also requested the nonexistent `test_frozendict` module; its 154 dict tests passed but the overall command failed to import that module. The final run requested the correct module only.

The restored W5 baseline already prints `dict.__getitem__` in the bound dict-subclass arity error, where the upstream reference prints the subclass name. This patch preserves the W5 baseline. No whole-suite or performance claim is made for this packet.
