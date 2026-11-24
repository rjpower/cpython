"""Report where the declared records differ from the inferred ones.

The generator builds every handler's record twice: once from Python/
bytecodes.c as it is, where a uop that declares a contract has the record
its declarations give (ir_contract.py), and once with every uop's clauses
removed, where each record is today's inference (ir_records.py).  The two
are compared whole: every field of every operation record, uop record,
guard and transfer, normalized (tokens by their place, analyzer objects by
their names), and the error routes that ir_error_effects exports, raise
checkpoints included.  For each declared uop the declared modes and exits
are also held to the record's ownership and transfers, which the generator
still derives from the stack effect and the body (G0d replaces them).

Review reads the report; the generator never does.  It lives until G0d
deletes the inference, and this script with it.

    python Tools/cases_generator/ir_contract_diff.py [--excepted SEED.json] [Python/bytecodes.c]

SEED.json is the report of ir_contract_seed.py: the uops that the seed left
to the inference, which the report lists as excepted, with their reasons.
"""

import argparse
import dataclasses
import enum
import json

import analyzer
from generators_common import ROOT
from lexer import Token
from parser import Stmt
import ir_contract
import ir_error_effects
import ir_generator
import ir_records

# The record's ownership of a declared mode (ir_records.walk_values)
INPUT_OWNERSHIP = {"": ir_records.BORROWED, "borrowed": ir_records.CONSUMED,
                   "consumed": ir_records.CONSUMED, "taken": ir_records.CONSUMED,
                   "closed": ir_records.CONSUMED}
OUTPUT_OWNERSHIP = {"": ir_records.THROUGH, "owned": ir_records.OWNED,
                    "inline": ir_records.OWNED, "moved": ir_records.ALIAS,
                    "borrowed": ir_records.OWNED}
TRANSFER_EXIT = {ir_records.NEXT: "next", ir_records.BRANCH: "branch",
                 ir_records.JUMP: "jump", ir_records.RAISE: "raise",
                 ir_records.ENTER_FRAME: "enter", ir_records.RETURN: "return",
                 ir_records.YIELD: "yield", ir_records.EXIT: "exit"}
GUARD_EXIT = {ir_records.EXIT_GUARD: "miss", ir_records.DEOPT_GUARD: "deopt"}
# The fields that hold a conjunction of facts
CONJUNCTIONS = {ir_records.Guard: ("facts",), ir_records.UopRecord: ("results",)}
# The analyzer's objects, which a record names rather than holds
NAMED = (analyzer.Instruction, analyzer.Uop, analyzer.Family, analyzer.PseudoInstruction)


def normal(value, tokens: dict[int, str]):
    """value as plain data that two builds of the same file can compare:
    tokens by their text and place, analyzer objects by their names, the
    ids of tokens (which ir_error_effects keys its routes by) as tokens"""
    if isinstance(value, Token):
        return f"{value.text}@{value.line}:{value.column}"
    if isinstance(value, NAMED):
        return f"<{type(value).__name__} {value.name}>"
    if isinstance(value, analyzer.StackItem):
        return {"name": value.name, "size": value.size, "peek": value.peek,
                "nullable": value.nullable}
    if isinstance(value, enum.Enum):
        return value.name
    if isinstance(value, bool) or value is None or isinstance(value, (str, float)):
        return value
    if isinstance(value, int):
        return tokens.get(value, value)
    if dataclasses.is_dataclass(value) and not isinstance(value, type):
        fields = {f.name: normal(getattr(value, f.name), tokens)
                  for f in dataclasses.fields(value)}
        for name in CONJUNCTIONS.get(type(value), ()):
            # A conjunction of facts: their order says nothing
            fields[name] = sorted(fields[name], key=repr)
        return {"__type": type(value).__name__, **fields}
    if isinstance(value, dict):
        return {repr(normal(k, tokens)): normal(v, tokens) for k, v in value.items()}
    if isinstance(value, (set, frozenset)):
        return sorted((normal(v, tokens) for v in value), key=repr)
    if isinstance(value, (list, tuple)):
        return [normal(v, tokens) for v in value]
    if callable(value):
        return getattr(value, "__qualname__", repr(value))
    return repr(value)


def token_ids(analysis) -> dict[int, str]:
    """the id of every uop, statement and token of the handlers' bodies, by
    its name or place: ir_error_effects keys its routes by these ids"""
    found = {}

    def statements(stmt) -> None:
        if isinstance(stmt, list):
            for s in stmt:
                statements(s)
        elif isinstance(stmt, Stmt):
            first = next(iter(stmt.tokens()), None)
            found[id(stmt)] = (f"<{type(stmt).__name__} at {first.line}:{first.column}>"
                               if first is not None else f"<{type(stmt).__name__}>")
            for f in dataclasses.fields(stmt):
                statements(getattr(stmt, f.name))

    for inst in ir_generator.handler_instructions(analysis).values():
        for part in inst.parts:
            if isinstance(part, analyzer.Uop):
                found[id(part)] = f"<Uop {part.name}>"
                statements(part.body)
                for t in part.body.tokens():
                    found[id(t)] = f"{t.text}@{t.line}:{t.column}"
    return found


def strip_clauses(analysis) -> None:
    "every uop of analysis without its clauses: today's inference"
    for inst in analysis.instructions.values():
        for part in inst.parts:
            if isinstance(part, analyzer.Uop):
                object.__setattr__(part, "clauses", {})
    for uop in analysis.uops.values():
        object.__setattr__(uop, "clauses", {})


def normalized(analysis, records) -> dict:
    "every handler's record and error routes, normalized"
    tokens = token_ids(analysis)
    handlers = ir_generator.handler_instructions(analysis)
    result = {}
    for name, record in records.items():
        routes = ir_error_effects.analyze_error_effects(handlers[name])
        result[name] = {"record": normal(record, tokens), "error_routes": normal(routes, tokens)}
    return result


def build(path: str, declared: bool, numbers=None):
    """The analysis of path and its records; without declared, every uop's
    clauses removed first.  numbers: the opcode numbering (the tree's)"""
    analysis = analyzer.analyze_files([path], numbers)
    if not declared:
        strip_clauses(analysis)
    return analysis, ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))


def compare(inferred, declared, path: str = "") -> list[str]:
    "every place where two normalized values differ, as `path: inferred ..., declared ...`"
    if type(inferred) is not type(declared):
        return [f"{path}: inferred {short(inferred)}, declared {short(declared)}"]
    if isinstance(inferred, dict):
        lines = []
        for key in sorted(set(inferred) | set(declared)):
            if key not in inferred or key not in declared:
                lines.append(f"{path}.{key}: present in the "
                             f"{'declared' if key in declared else 'inferred'} record only")
            else:
                lines += compare(inferred[key], declared[key], f"{path}.{key}")
        return lines
    if isinstance(inferred, list):
        if len(inferred) != len(declared):
            return [f"{path}: inferred {len(inferred)} entries {short(inferred)}, "
                    f"declared {len(declared)} {short(declared)}"]
        lines = []
        for i, (a, b) in enumerate(zip(inferred, declared)):
            lines += compare(a, b, f"{path}[{i}]")
        return lines
    if inferred != declared:
        return [f"{path}: inferred {short(inferred)}, declared {short(declared)}"]
    return []


def short(value) -> str:
    text = json.dumps(value, sort_keys=True)
    return text if len(text) <= 200 else text[:197] + "..."


def declared_against_record(analysis, records) -> list[str]:
    """For each declared uop: its modes against the record's ownership, and
    its exits against the record's transfers and guards"""
    lines = []
    seen = set()
    handlers = ir_generator.handler_instructions(analysis)
    for name, record in sorted(records.items()):
        parts = [p for p in handlers[name].parts if isinstance(p, analyzer.Uop)]
        for part, urec in zip(parts, record.uops):
            contract = ir_contract.contract_of(part)
            key = (part.name, tuple(i.ownership for i in urec.inputs),
                   tuple(o.ownership for o in urec.outputs),
                   tuple(t.kind for t in urec.transfers), tuple(g.kind for g in urec.guards))
            if contract is None or key in seen:
                continue
            seen.add(key)
            where = f"{name}/{part.name}"
            for item, uin in zip(contract.inputs, urec.inputs):
                if item.name != "unused" and INPUT_OWNERSHIP[item.mode] != uin.ownership:
                    lines.append(f"{where}: input `{item.name}` is declared "
                                 f"{item.mode or 'a pass-through'}, the record has "
                                 f"{uin.ownership}")
            for item, uout in zip(contract.outputs, urec.outputs):
                if item.name != "unused" and OUTPUT_OWNERSHIP[item.mode] != uout.ownership:
                    lines.append(f"{where}: output `{item.name}` is declared "
                                 f"{item.mode or 'a pass-through'}, the record has "
                                 f"{uout.ownership}")
            exits = {TRANSFER_EXIT[t.kind] for t in urec.transfers if t.kind in TRANSFER_EXIT}
            exits |= {GUARD_EXIT[g.kind] for g in urec.guards}
            if exits != set(contract.exits):
                lines.append(f"{where}: declares exits({', '.join(sorted(contract.exits))}), "
                             f"the record has {', '.join(sorted(exits))}")
    return lines


def differences(inferred: dict, declared: dict) -> list[str]:
    "the differences between two builds' normalized handlers"
    lines = []
    for handler in sorted(set(inferred) | set(declared)):
        if handler not in inferred or handler not in declared:
            lines.append(f"{handler}: in one build only")
            continue
        lines += [handler + line for line in compare(inferred[handler], declared[handler])]
    return lines


def report(path: str, excepted: dict[str, str] | None = None, numbers=None) -> list[str]:
    "The report's lines: the differences, then the counts and the exceptions"
    analysis, records = build(path, declared=True, numbers=numbers)
    declared = normalized(analysis, records)
    bare, bare_records = build(path, declared=False, numbers=numbers)
    lines = differences(normalized(bare, bare_records), declared)
    lines += declared_against_record(analysis, records)
    uops = {p.name: p for inst in analysis.instructions.values() for p in inst.parts
            if isinstance(p, analyzer.Uop)}
    count = sum(ir_contract.contract_of(u) is not None for u in uops.values())
    lines.append(f"{len(records)} handlers compared; {count} of {len(uops)} uops declared")
    for name, why in sorted((excepted or {}).items()):
        lines.append(f"excepted {name}: {why}")
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("input", nargs="?", default=str(ROOT / "Python" / "bytecodes.c"))
    parser.add_argument("--excepted", default=None,
                        help="the seed's report: its uops left to the inference")
    args = parser.parse_args()
    excepted = None
    if args.excepted:
        with open(args.excepted) as f:
            excepted = json.load(f)["excluded"]
    print("\n".join(report(args.input, excepted)))


if __name__ == "__main__":
    main()
