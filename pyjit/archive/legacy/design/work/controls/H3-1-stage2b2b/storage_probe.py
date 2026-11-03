"""Semantic controls for copied real storage helpers and deinstrumentation."""
import dis
import importlib.util
import sys
import _testinternalcapi as capi

assert sys.flags.optimize == 0 and __debug__
assert capi.gc_verify_mode() == (0, 0) and capi.gc_get_stress() == 0
spec = importlib.util.spec_from_file_location("_h3_storage", sys.argv[1])
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
expected = ((255, -1), (511, 0), (65535, 2**31 - 1))
actual = module.roundtrip(1, 511, 0)
assert actual == expected, ("adjacent storage values", actual, expected)
for index in range(3):
    for token in (0, 255, 256, 65535):
        for line in (-1, 0, 2**31 - 1):
            values = list(expected)
            values[index] = (token, line)
            assert module.roundtrip(index, token, line) == tuple(values)
for args in ((-1, 1, 1), (3, 1, 1), (0, 65536, 1),
             (0, 65536 + dis.opmap["NOP"], 1), (0, 1, -2)):
    try:
        module.roundtrip(*args)
    except ValueError:
        pass
    else:
        raise AssertionError(("scalar boundary accepted", args))
alias = dis._all_opmap["INSTRUMENTED_LOAD_SUPER_ATTR_NOFB"]
base = dis.opmap["LOAD_SUPER_ATTR"]
assert module.deinstrument(alias) == base, ("canonical alias", module.deinstrument(alias), base)
for value in (-1, 65536, 65536 + dis.opmap["NOP"]):
    assert module.deinstrument(value) == -1
print("PASS: copied helpers preserve full-width adjacent values and canonical aliases")
