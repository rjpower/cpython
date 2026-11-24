"""The review script that compares declared records with the inferred ones
(Tools/cases_generator/ir_contract_diff.py): it compares every field."""

import copy
import dataclasses
import os
import tempfile
import textwrap
import unittest

from test import test_generated_cases as cases
from test import test_tools

ir_generator = cases.ir_generator
with test_tools.imports_under_tool("cases_generator"):
    import ir_contract_diff
    import ir_error_effects
    import ir_records

FIXTURE = """
    op(_GUARD, (value -- value: inline_int)) effects() exits(next, miss) {
        EXIT_IF(!PyValue_IsInlineInt(value));
    }
    op(_TAKE, (value: consumed inline_int -- res: owned exact(PyLong_Type))) effects(allocates, raises)
        exits(next, raise)
    {
        res = _PyValue_BoxInt64(PyValue_InlineInt64(value));
        DEAD(value);
        ERROR_IF(PyValue_IsNull(res));
    }
    macro(OP) = _GUARD + _TAKE;
"""


def setUpModule():
    global filename, analysis, records, declared
    fd, filename = tempfile.mkstemp(suffix=".c")
    with os.fdopen(fd, "w") as f:
        f.write(cases.parser.BEGIN_MARKER)
        f.write(textwrap.dedent(FIXTURE))
        f.write(cases.parser.END_MARKER)
    with cases.handle_stderr():
        analysis, records = ir_contract_diff.build(filename, declared=True,
                                                       numbers=cases.SequentialNumbers())
    declared = ir_contract_diff.normalized(analysis, records)


def tearDownModule():
    os.remove(filename)


_TREE = []


def tree_routes():
    "a handler of the tree whose error routes ir_error_effects summarizes"
    if not _TREE:
        bytecodes = os.path.join(test_tools.basepath, "Python", "bytecodes.c")
        with cases.handle_stderr():
            tree, tree_records = ir_contract_diff.build(bytecodes, declared=True)
        found = ir_contract_diff.normalized(tree, tree_records)
        _TREE.append(next((name, value["error_routes"]) for name, value in sorted(found.items())
                          if value["error_routes"] is not None))
    return _TREE[0]


def field_names(cls) -> set[str]:
    return {f.name for f in dataclasses.fields(cls)}


class DiffTests(unittest.TestCase):

    def test_equal_builds_have_no_difference(self):
        with cases.handle_stderr():
            lines = ir_contract_diff.report(filename, numbers=cases.SequentialNumbers())
        self.assertEqual(lines, ["1 handlers compared; 2 of 2 uops declared"])

    def test_every_field_is_normalized(self):
        record = declared["OP"]["record"]
        self.assertEqual(set(record) - {"__type"}, field_names(ir_records.OperationRecord))
        uop = record["uops"][0]
        self.assertEqual(set(uop) - {"__type"}, field_names(ir_records.UopRecord))
        self.assertEqual(set(uop["guards"][0]) - {"__type"}, field_names(ir_records.Guard))
        self.assertEqual(set(uop["transfers"][0]) - {"__type"},
                         field_names(ir_records.Transfer))
        routes = tree_routes()[1]
        self.assertEqual(set(routes) - {"__type"},
                         field_names(ir_error_effects.SourceErrorEffects))

    def test_a_difference_planted_in_each_field_is_reported(self):
        # Every field of the operation record, of each uop record, of a
        # guard and a transfer, and of the error routes (raise checkpoints
        # included): a changed value is reported at its path
        base = declared["OP"]
        places = [("record", key) for key in base["record"] if key != "__type"]
        places += [("record", "uops", 0, key) for key in base["record"]["uops"][0]
                   if key != "__type"]
        places += [("record", "uops", 0, "guards", 0, key)
                   for key in base["record"]["uops"][0]["guards"][0] if key != "__type"]
        places += [("record", "uops", 1, "transfers", 0, key)
                   for key in base["record"]["uops"][1]["transfers"][0] if key != "__type"]
        for place in places:
            with self.subTest(place=place):
                planted = copy.deepcopy(declared)
                target = planted["OP"]
                for step in place[:-1]:
                    target = target[step]
                target[place[-1]] = ["planted"]
                lines = ir_contract_diff.differences(declared, planted)
                path = "OP" + "".join(f"[{p}]" if isinstance(p, int) else f".{p}"
                                      for p in place)
                self.assertTrue(lines, place)
                self.assertTrue(all(line.startswith(path) for line in lines), lines)

    def test_error_routes_are_compared(self):
        # Every field of a handler's error routes, raise checkpoints
        # included, from the tree (the fixture's handler has none)
        handler, routes = tree_routes()
        for key in routes:
            if key == "__type":
                continue
            with self.subTest(field=key):
                base = {handler: {"error_routes": routes}}
                planted = copy.deepcopy(base)
                planted[handler]["error_routes"][key] = ["planted"]
                lines = ir_contract_diff.differences(base, planted)
                self.assertTrue(lines)
                self.assertTrue(all(line.startswith(f"{handler}.error_routes.{key}")
                                    for line in lines), lines)

    def test_guard_count_and_reason_are_compared(self):
        planted = copy.deepcopy(declared)
        guards = planted["OP"]["record"]["uops"][0]["guards"]
        guards[0]["reason"] = "a reason"
        self.assertEqual(ir_contract_diff.differences(declared, planted),
                         ['OP.record.uops[0].guards[0].reason: inferred "", '
                          'declared "a reason"'])
        guards.append(copy.deepcopy(guards[0]))
        lines = ir_contract_diff.differences(declared, planted)
        self.assertEqual(len(lines), 1)
        self.assertRegex(lines[0], r"^OP\.record\.uops\[0\]\.guards: inferred 1 entries .*"
                                   r"declared 2 ")

    def test_modes_and_exits_against_the_record(self):
        self.assertEqual(ir_contract_diff.declared_against_record(analysis, records), [])
        record = records["OP"]
        take = record.uops[1]
        changed = dataclasses.replace(
            take, inputs=(dataclasses.replace(take.inputs[0],
                                              ownership=ir_records.BORROWED),),
            transfers=take.transfers + (ir_records.Transfer(ir_records.EXIT),))
        planted = {"OP": dataclasses.replace(record, uops=(record.uops[0], changed))}
        self.assertEqual(ir_contract_diff.declared_against_record(analysis, planted), [
            "OP/_TAKE: input `value` is declared consumed, the record has BORROWED",
            "OP/_TAKE: declares exits(next, raise), the record has exit, next, raise"])


if __name__ == "__main__":
    unittest.main()
