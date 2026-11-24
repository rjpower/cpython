"""Seed the declared contracts of the register IR from today's inference.

A one-shot tool outside the generator (pyjit regions89, unit G0b): for each
uop of Python/bytecodes.c without clauses it writes the modes and facts of
its stack items and its effects(...), exits(...) and feedback(...) clauses
from the record that today's inference gives it, and for each callee
without a row it writes a row into Include/internal/pycore_ir_components.h
from today's tables and the callee's C prototype.  The seed grants nothing
new: a uop is written only if the generator then builds the same record for
it (effects, guard facts and result facts) and verifies it; the others are
reported, with the error, for review (G0c).  It skips everything already
declared, so that it can run again after a merge.  G0d deletes it with the
inference.

    python Tools/cases_generator/ir_contract_seed.py --build BUILD_DIR [--report DIR]

BUILD_DIR is a configured build whose compiler flags compile Python/ceval.c:
the C types of the rows come from that compilation (gcc -aux-info and -dM).
"""

import argparse
import collections
import json
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field

import analyzer
import ir_contract
import ir_generator
import ir_leaf
import ir_records
from generators_common import ROOT
from ir_records import (ALLOCATES, MAY_RAISE, MAY_RELEASE, MAY_RUN_PYTHON, READS_HEAP,
                        WRITES_HEAP)

BYTECODES = ROOT / "Python" / "bytecodes.c"
HEADER = ir_contract.COMPONENTS_HEADER

EFFECT_NAME = {READS_HEAP: "reads", WRITES_HEAP: "writes", ALLOCATES: "allocates",
               MAY_RAISE: "raises", MAY_RELEASE: "releases", MAY_RUN_PYTHON: "runs_python"}
EXIT_NAME = {ir_records.NEXT: "next", ir_records.BRANCH: "branch", ir_records.JUMP: "jump",
             ir_records.RAISE: "raise", ir_records.ENTER_FRAME: "enter",
             ir_records.RETURN: "return", ir_records.YIELD: "yield", ir_records.EXIT: "exit"}
GUARD_EXIT = {ir_records.EXIT_GUARD: "miss", ir_records.DEOPT_GUARD: "deopt"}


def ordered_effects(effects) -> list[str]:
    "effects in the vocabulary's order, without those that runs_python covers"
    effects = set(effects)
    if "runs_python" in effects:
        effects -= ir_contract.COVERS["runs_python"] - {"runs_python"}
    return [e for e in ir_contract.EFFECTS if e in effects]


# ---------------------------------------------------------------------------
# Today's inference

def inferred_records(analysis) -> dict:
    """What the generator builds for every handler's uops, as text: per
    handler, per uop in order, its effects, guards (kind, facts, reason),
    results, transfers and inputs and outputs"""
    records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))
    result = {}
    for handler, record in records.items():
        uops = []
        for urec in record.uops:
            uops.append({
                "name": urec.name,
                "effects": sorted(urec.effects),
                "guards": [[g.kind, [str(f) for f in g.facts], g.reason, g.site is None,
                            g.input] for g in urec.guards],
                "results": [str(f) for f in urec.results],
                "transfers": [t.kind for t in urec.transfers],
                "inputs": [[i.name, i.value, i.ownership] for i in urec.inputs],
                "outputs": [[o.name, o.value, o.ownership] for o in urec.outputs],
                "leaf": urec.leaf is not None,
            })
        result[handler] = uops
    return result


def uop_facts(analysis, records) -> dict[str, dict]:
    """Per undeclared uop: its record's facts on its inputs (guards) and new
    outputs (results), keyed by item, in the record's kinds"""
    found: dict[str, dict] = {}
    for handler, uops in records.items():
        for u in uops:
            entry = found.setdefault(u["name"], {"guards": {}, "results": {}, "implicit": set(),
                                                  "effects": set(u["effects"]),
                                                  "transfers": set(), "guard_kinds": set(),
                                                  "outputs": u["outputs"]})
            values = {f"v{v}": name for name, v, _ in u["inputs"]}
            values.update({f"v{v}": name for name, v, _ in u["outputs"]})
            entry["transfers"] |= {t for t in u["transfers"] if t != ir_records.DEOPT}
            implicit = 0
            for kind, facts, reason, is_implicit, _ in u["guards"]:
                entry["guard_kinds"].add(kind)
                implicit += is_implicit
                for f in facts:
                    m = re.fullmatch(r"(\w+)\((.*?)(?:, (.*))?\)", f)
                    subject = values.get(m.group(2), m.group(2))
                    entry["guards"].setdefault(subject, set()).add((m.group(1), m.group(3) or ""))
            entry["implicit"].add(implicit)
            for f in u["results"]:
                m = re.fullmatch(r"(\w+)\((.*?)(?:, (.*))?\)", f)
                subject = values.get(m.group(2), m.group(2))
                entry["results"].setdefault(subject, set()).add((m.group(1), m.group(3) or ""))
    return found


def all_uops(analysis) -> dict:
    "every uop by name: the ops, and the bodies of single-uop instructions"
    found = {u.name: u for u in analysis.uops.values()}
    for inst in analysis.instructions.values():
        for part in inst.parts:
            if isinstance(part, analyzer.Uop):
                found.setdefault(part.name, part)
    return found


def callee_effects(analysis) -> dict[str, set[str]]:
    """The effects that today's inference gives each callee's call sites,
    over every undeclared uop (ir_records.effect_sites): the union"""
    found: dict[str, set[str]] = collections.defaultdict(set)
    for uop in all_uops(analysis).values():
        if uop.clauses:
            continue
        calls = {id(t) for t, _ in ir_contract.body_calls(uop)}
        names = {id(t): t.text for t, _ in ir_contract.body_calls(uop)}
        for t in names.values():
            found.setdefault(t, set())
        for tkn, effect in ir_records.effect_sites(uop):
            if id(tkn) in calls:
                found[names[id(tkn)]].add(EFFECT_NAME[effect or MAY_RUN_PYTHON])
    return found


# ---------------------------------------------------------------------------
# C types, by the compiler

@dataclass
class CType:
    ret: str
    params: tuple[str, ...]


def compiler_view(build: str, scratch: str) -> tuple[dict[str, CType], set[str]]:
    """The prototypes (gcc -aux-info) and the macro names (gcc -dM) that
    Python/ceval.c sees, compiled with the build's flags"""
    mk = os.path.join(scratch, "flags.mk")
    with open(mk, "w") as f:
        f.write("seed-flags:\n\t@echo $(PY_CORE_CFLAGS)\n")
    flags = subprocess.run(["make", "-s", "-f", "Makefile", "-f", mk, "seed-flags"], cwd=build,
                           capture_output=True, text=True, check=True).stdout.strip().splitlines()[-1].split()
    aux = os.path.join(scratch, "ceval.aux")
    ceval = str(ROOT / "Python" / "ceval.c")
    subprocess.run(["gcc", *flags, "-O0", "-w", "-fsyntax-only", "-aux-info", aux, ceval],
                   cwd=build, check=True)
    macros = subprocess.run(["gcc", *flags, "-E", "-dM", ceval], cwd=build, check=True,
                            capture_output=True, text=True).stdout
    names = set(re.findall(r"^#define (\w+)", macros, re.M))
    # A macro that only wraps the function of its own name (the readers'
    # PyValue_Borrow() wrappers) leaves the function as the callee
    wrappers = {m.group(1) for m in re.finditer(r"^#define (\w+)\([^)]*\) \(?(\w+)\)?\s*\(",
                                              macros, re.M) if m.group(1) == m.group(2)}
    types: dict[str, CType] = {}
    with open(aux) as f:
        for line in f:
            parsed = _aux_line(line)
            if parsed is not None:
                # A declaration without parameter names (NC) is preferred;
                # the first one seen otherwise
                name, ctype, declaration = parsed
                if name not in types or declaration:
                    types[name] = ctype
    for name in names - wrappers:
        # The call expands the macro, whatever function it names
        types.pop(name, None)
    return types, names


def _aux_line(line: str):
    m = re.match(r"/\* [^ ]*:\d+:(\w\w) \*/ (.*?);(?: /\* \((.*?)\) .*\*/)?$", line.strip())
    if m is None:
        return None
    decl, names = m.group(2), [n.strip() for n in (m.group(3) or "").split(",") if n.strip()]
    open_ = decl.find(" (")
    head = decl[:open_]
    if "(" in head:
        return None         # a function returning a function pointer
    words = head.replace("*", " * ").split()
    name = words[-1]
    ret = " ".join(w for w in words[:-1] if w not in ("static", "extern", "inline", "__inline"))
    params = []
    inner = decl[open_ + 2:decl.rfind(")")]
    for i, p in enumerate(ir_contract._split_text(inner)):
        p = p.strip()
        if i < len(names) and re.search(rf"\b{re.escape(names[i])}$", p):
            p = p[:-len(names[i])].strip()
        if p and p != "void":
            params.append(_ctype(p))
    return name, CType(_ctype(ret), tuple(params)), m.group(1)[0] == "N" and not names


def _ctype(text: str) -> str:
    text = re.sub(r"\s*\*\s*", " *", text.strip()).replace("_Bool", "bool")
    text = re.sub(r"\b(const )+", "const ", text)
    return re.sub(r"\s+", " ", text).replace("* *", "**").strip()


# ---------------------------------------------------------------------------
# Rows

TOPICS = (
    # group, the names it holds
    ("VALUES", r"_?PyValue_|_?PyLong|_?PyFloat|_?PyBool|Py_ARITHMETIC|_PyIR_|TAKE$|OWNER$"),
    ("CONTAINERS", r"_?Py(List|Tuple|Dict|AnyDict|Set|AnySet|FrozenSet|Slice|Range)|DK_|"
                   r"GETITEM|_PyDict|_PyList|_PyTuple|_PyStolen"),
    ("CALLS", r"call_|_PyFrame_|_?PyFunction|_?PyCFunction|_?PyMethod|_PyEval_Frame|"
              r"_PyEval_Vector|_PyThreadState_|_Py_EnterRecursive|_Py_LeaveRecursive|"
              r"_PyCallMethod|_PyObject_Vectorcall|PyObject_Vectorcall|_PyEval_Retire|"
              r"_Py_ReachedRecursionLimit|py_recursion|IS_PEP523_HOOKED|_PyDataStack"),
    ("GENERATORS", r"gen_|_?PyGen|_?PyCoro|coro_|PyAsyncGen|_PyEval_GetAwaitable|"
                   r"_PyEval_GetANext|_PyEval_GeneratorReturn|_PyForIter|_PyAsync"),
    ("ERRORS", r"Err|Exception|_PyEval_Format|monitor_reraise"),
    ("MONITORING", r"_Py_call_instrumentation|monitor|INSTRUMENTED|_Py_Instrument|"
                   r"_PyMonitoring|DTRACE|LLTRACE|_Py_EvalBreaker|check_periodics|"
                   r"_Py_(un)?set_eval_breaker"),
    ("SPECIALIZATION", r"_Py_Specialize|COUNTER|FB_|_PyFb|ORIGINAL_HANDLER|backoff"),
    ("EVALUATOR", r"DISPATCH|JUMP|LOAD_IP|GLOBALS|BUILTINS|LOCALS|GETLOCAL|SAVE_STACK|"
                  r"RELOAD_STACK|STAT_INC|INSTR_|PRE_DISPATCH|FRAME_CO_|COMPARISON_BIT|"
                  r"_PyEval_|_Py_atomic|FT_ATOMIC"),
    ("OBJECTS", r".*"),
)
GROUP_TITLES = {
    "CALLS": "Calls and frames",
    "GENERATORS": "Generators, coroutines and iteration",
    "ERRORS": "Errors",
    "MONITORING": "Monitoring and the eval breaker",
    "EVALUATOR": "The evaluator: dispatch, the frame's namespaces, atomics",
}
GROUP_ORDER = ("VALUES", "OBJECTS", "CONTAINERS", "CALLS", "GENERATORS", "ERRORS",
               "MONITORING", "SPECIALIZATION", "EVALUATOR", "LABELS", "TRANSACTIONS")


def topic(name: str) -> str:
    return next(g for g, rx in TOPICS if re.match(rx, name))


def vocabulary_fact(kind: str, arg: str) -> ir_contract.VFact | None:
    word = ir_contract.VOCABULARY_OF_KIND.get(kind)
    if word is None or word in ir_contract.FACTS_WITH_ARGUMENT and not re.fullmatch(r"\w+", arg):
        return None
    return ir_contract.VFact(word, arg if word in ir_contract.FACTS_WITH_ARGUMENT else "")


def predicate_test(name: str, arity: int) -> tuple[str, list[str]] | None:
    """The TEST result that ir_records' fact tables give a predicate, with
    the tables used; None if they give none in the vocabulary"""
    class One:
        def of(self, tokens):
            return "x"
    args = []
    for i in range(arity):
        args += ([","] if i else []) + [f"a{i}"]
    found = ir_records.predicate_facts([name, "(", *args, ")"], One())
    if not found:
        return None
    facts = [vocabulary_fact(f.kind, f.arg) for f in found]
    if None in facts:
        return None
    tests = [" ".join(str(f) for f in facts[i:i + 1]) for i in range(len(facts))]
    if arity > len(tests):
        tests += [""] * (arity - len(tests))
    return f"TEST({', '.join(tests)})", ["ir_records.predicate_facts"]


@dataclass
class SeedRow:
    name: str
    kind: str
    text: list[str]         # the row's lines, without continuations
    tables: list[str]
    group: str


GENERATOR_SOURCE = (ROOT / "Tools/cases_generator/generators_common.py").read_text()
INCLUDE_SOURCE = "\n".join(p.read_text() for p in sorted((ROOT / "Include").rglob("*.h")))


def call_sites(analysis) -> dict[str, list]:
    "Each callee's call sites: (uop, argument token texts)"
    sites = collections.defaultdict(list)
    for uop in all_uops(analysis).values():
        for tkn, args in ir_contract.body_calls(uop):
            sites[tkn.text].append((uop, args))
    return sites


def in_guard(uop, name: str) -> bool:
    texts = [t.text for t in ir_contract._tokens(uop)]
    for i, t in enumerate(texts):
        if t in ir_contract.GUARDS and i + 1 < len(texts) and texts[i + 1] == "(":
            if name in texts[i + 2:ir_records._close(texts, i + 1)]:
                return True
    return False


def arg_mode(ctype: str) -> str:
    return {"PyThreadState *": "tstate", "_PyInterpreterFrame *": "frame",
            "PyValueBorrow": "borrowed", "PyValue": "consumed", "PyObject *": "object",
            "destructor": "destructor"}.get(ctype, "scalar")


def value_result(name: str, effects: set[str], tables: list[str], mode: str = "owned") -> str:
    facts = ""
    if name in ir_records.INLINE_INT_RESULTS:
        mode, facts = "inline", " inline_int"
        tables.append("ir_records.INLINE_INT_RESULTS")
    elif name in ir_records.INT_RESULTS:
        facts = " exact(PyLong_Type)"
        tables.append("ir_records.INT_RESULTS")
    elif name in ir_records.FLOAT_RESULTS:
        facts = " exact(PyFloat_Type)"
        tables.append("ir_records.FLOAT_RESULTS")
    fallible = bool(effects & {"raises", "runs_python"})
    return f"{'VALUE_OR_NULL' if fallible and mode == 'owned' else 'VALUE'}({mode}{facts})"


def seed_rows(analysis, table, ctypes, macros, effects_of) -> tuple[list[SeedRow], dict]:
    """A row for every callee without one; the callees that no row can
    declare, with the reason"""
    sites = call_sites(analysis)
    rows: list[SeedRow] = []
    refused = {}
    for name in sorted(sites):
        if (name in table or name in ir_contract.DSL_CONSTRUCTS
                or ir_contract.CHECKED_ARITHMETIC.fullmatch(name)):
            continue
        tables: list[str] = []
        arity = max(len(a) for _, a in sites[name])
        effects = set(effects_of.get(name, ()))
        effect_tables = ["ir_records.effect_sites"]
        exits = []
        if name in ir_records.TRANSFER_TOKENS:
            exits = sorted({EXIT_NAME[k] for k in ir_records.TRANSFER_TOKENS[name]})
            tables.append("ir_records.TRANSFER_TOKENS")
        feedback = ""
        if name.startswith("_Py_Specialize_"):
            feedback = "FEEDBACK(read(entry), write(entry), write(handler))"
            tables.append("the specializing annotation")
        elif name == "PAUSE_ADAPTIVE_COUNTER":
            feedback = "FEEDBACK(write(counter))"
            tables.append("ceval_macros.h")
        guard = any(in_guard(u, name) for u, _ in sites[name])
        test = predicate_test(name, arity) if guard else None
        context = ir_records.CONTEXT_PREDICATES.get(name)
        if name in ir_leaf.IDENTITIES or name in ir_leaf.ERASED:
            which = "IDENTITIES" if name in ir_leaf.IDENTITIES else "ERASED"
            result = "SCALAR" if which == "IDENTITIES" else "NONE"
            if context is not None:
                result = f'CONTEXT("{context}")'
            text = [f"INTRINSIC({name}, ARGS({', '.join(['scalar'] * arity)}), {result}, "
                    "BUILD(gil))"]
            rows.append(SeedRow(name, "INTRINSIC", text, [f"ir_leaf.{which}"], topic(name)))
            continue
        ctype = ctypes.get(name)
        if ctype is None and name not in macros:
            if name in ir_generator.VIEW_TAKES:
                # A move of an owner out of its variable (ir_generator.VIEW_TAKES)
                text = [f"MACRO({name}, ARGS(consumed), VALUE(owned), "
                        f"EFFECTS({', '.join(ordered_effects(effects))}), EXITS())"]
                rows.append(SeedRow(name, "MACRO", text, ["ir_generator.VIEW_TAKES"]
                                    + effect_tables, topic(name)))
                continue
            if f'"{name}"' in GENERATOR_SOURCE:
                # The emitters replace it (generators_common.py): a generator
                # macro in the design's own sense
                text = [f"MACRO({name},", f"    ARGS({', '.join(['scalar'] * arity)}), "
                        f"{'NONE' if name in ('SAVE_STACK', 'RELOAD_STACK', 'SYNC_SP') else 'SCALAR'}, "
                        f"EFFECTS({', '.join(ordered_effects(effects))}), "
                        f"EXITS({', '.join(exits)}))"]
                rows.append(SeedRow(name, "MACRO", text, ["generators_common.py's expansion"]
                                    + tables + effect_tables, topic(name)))
                continue
            if re.search(rf"\b{re.escape(name)}\s*\(", INCLUDE_SOURCE):
                text = [f"INTRINSIC({name}, ARGS({', '.join(['scalar'] * arity)}), SCALAR, "
                        "BUILD(free_threaded))"]
                rows.append(SeedRow(name, "INTRINSIC", text,
                                    ["a definition under Py_GIL_DISABLED"], topic(name)))
                continue
            refused[name] = ("no C function or macro of this name is visible to ceval.c, "
                             "and no generator expands it: a call through a function "
                             "pointer (a local or a slot)")
            continue
        if ctype is not None and any("(" in p for p in ctype.params + (ctype.ret,)):
            ctype = None if name in macros else ctype
        if ctype is not None:
            kind = ("PRIMITIVE" if name in ir_leaf.PRIMITIVES
                    else "ROUTINE" if exits else "HELPER")
            tables.insert(0, "the C prototype")
            if kind == "PRIMITIVE":
                tables.append("ir_leaf.PRIMITIVES")
            fixed = [p for p in ctype.params if p != "..."]
            args = [arg_mode(p) for p in fixed]
            if name.startswith("_Py_Specialize_"):
                # The specializers read their operands (G0a's specializer rows)
                args = ["borrowed" if a == "consumed" else a for a in args]
                tables.append("G0a's specializer rows")
            if context is not None:
                result = f'CONTEXT("{context}")'
                tables.append("ir_records.CONTEXT_PREDICATES")
            elif test is not None and ctype.ret in ("bool", "int"):
                result = test[0]
                tables += test[1]
            elif ctype.ret == "PyValue":
                result = value_result(name, effects, tables)
            elif ctype.ret == "PyValueBorrow":
                result = "VALUE(borrowed)"
            elif ctype.ret == "void":
                result = "NONE"
            else:
                result = "SCALAR"
            params = ", ".join(ctype.params)
            head = f"{kind}({name}, {ctype.ret}, ({params}),"
            cols = [f"ARGS({', '.join(args)})", result,
                    f"EFFECTS({', '.join(ordered_effects(effects))})"]
            if kind == "ROUTINE":
                cols.append(f"EXITS({', '.join(exits)})")
            if feedback:
                cols.append(feedback)
            text = [head, "    " + ", ".join(cols) + ")"]
        else:
            kind = "MACRO"
            tables.insert(0, "a C macro (no static check)")
            args = []
            for i in range(arity):
                values = [a[i] for _, a in sites[name] if len(a) > i]
                args.append("scalar")
            if context is not None:
                result = f'CONTEXT("{context}")'
                tables.append("ir_records.CONTEXT_PREDICATES")
            elif test is not None:
                result = test[0]
                tables += test[1]
            elif name in ir_records.TRANSFER_TOKENS:
                result = "NONE"
            elif _assigned_value(name, sites[name]):
                result = value_result(name, effects, tables)
            else:
                result = "SCALAR"
            cols = [f"ARGS({', '.join(args)})", result,
                    f"EFFECTS({', '.join(ordered_effects(effects))})",
                    f"EXITS({', '.join(exits)})"]
            if feedback:
                cols.append(feedback)
            text = [f"MACRO({name},", "    " + ", ".join(cols) + ")"]
        rows.append(SeedRow(name, kind, text, tables + effect_tables, topic(name)))
    # The deallocators that bodies pass to a close, which ir_records.NOT_RELEASES
    # says release nothing
    passed = set()
    for name, calls in sites.items():
        row = table.get(name)
        for _, args in calls:
            for arg in args:
                if len(arg) == 1 and arg[0] in ir_records.NOT_RELEASES \
                        and arg[0].endswith("Dealloc") and arg[0] not in table:
                    passed.add(arg[0])
    for name in sorted(passed - {r.name for r in rows}):
        # The exact type a deallocator frees, by the name _Py<T>_ExactDealloc;
        # a deallocator of more than one type has no row
        m = re.fullmatch(r"_Py(\w+?)_ExactDealloc", name)
        if m is None or "Or" in m.group(1):
            refused[name] = "a deallocator of no single exact type"
            continue
        rows.append(SeedRow(name, "DESTRUCTOR",
                            [f"DESTRUCTOR({name}, Py{m.group(1)}_Type, callback_free)"],
                            ["ir_records.NOT_RELEASES"], topic(name)))
    return rows, refused


def _assigned_value(name: str, sites) -> bool:
    "whether some uop assigns a call of name to a stack output"
    for uop, _ in sites:
        outs = {o.name for o in uop.stack.outputs}
        texts = [t.text for t in ir_contract._tokens(uop)]
        for i, t in enumerate(texts):
            if t == "=" and i and texts[i - 1] in outs:
                rhs = ir_records._uncast(texts[i + 1:texts.index(";", i)])
                if rhs[:1] == [name]:
                    return True
    return False


def render_rows(header: str, rows: list[SeedRow]) -> str:
    "header with rows added to the end of their groups, new groups made"
    by_group = collections.defaultdict(list)
    for row in rows:
        by_group[row.group].append(row)
    lines = header.split("\n")
    for group in GROUP_ORDER:
        new = by_group.get(group)
        if not new:
            continue
        body = []
        for row in new:
            body.append(f"    /* seeded from {', '.join(dict.fromkeys(row.tables))} */")
            for k, line in enumerate(row.text):
                body.append(("    " if k == 0 else "    ") + line)
        start = next((i for i, l in enumerate(lines)
                      if l.startswith(f"#define _PyIR_COMPONENTS_{group}(")), None)
        if start is None:
            # A new group, before the transactions
            at = next(i for i, l in enumerate(lines)
                      if l.startswith("/* The register IR's transaction macros"))
            block = [f"/* {GROUP_TITLES[group]} */",
                     f"#define _PyIR_COMPONENTS_{group}(PRIMITIVE, HELPER, MACRO) \\"]
            block += [l + " \\" for l in body[:-1]] + [body[-1], ""]
            lines[at:at] = block
            total = next(i for i, l in enumerate(lines)
                         if l.startswith("#define _PyIR_COMPONENTS(PRIMITIVE"))
            end = total
            while lines[end].endswith("\\"):
                end += 1
            members = [l.strip().rstrip("\\").strip() for l in lines[total + 1:end + 1]]
            members.append(f"_PyIR_COMPONENTS_{group}(PRIMITIVE, HELPER, MACRO)")
            order = {f"_PyIR_COMPONENTS_{g}(PRIMITIVE, HELPER, MACRO)": i
                     for i, g in enumerate(GROUP_ORDER)}
            members.sort(key=lambda m: order.get(m, len(order)))
            lines[total + 1:end + 1] = (["    " + m + " \\" for m in members[:-1]]
                                        + ["    " + members[-1]])
            continue
        end = start
        while lines[end].endswith("\\"):
            end += 1
        lines[end] += " \\"
        lines[end + 1:end + 1] = [l + " \\" for l in body[:-1]] + [body[-1]]
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Uop clauses

@dataclass
class Clauses:
    inputs: dict[str, list[str]] = field(default_factory=dict)    # item: words
    outputs: dict[str, list[str]] = field(default_factory=dict)
    effects: list[str] = field(default_factory=list)
    exits: list[str] = field(default_factory=list)
    feedback: list[str] = field(default_factory=list)


class Gap(Exception):
    "the inference states something the vocabulary cannot say"


EXIT_ORDER = ir_contract.EXITS


def seed_clauses(uop, facts: dict, table, overrides: dict) -> Clauses:
    c = Clauses()
    effects = {EFFECT_NAME[e] for e in facts["effects"]}
    if MAY_RUN_PYTHON in facts["effects"]:
        effects = {"runs_python"}
    p = uop.properties
    if p.eval_breaker:
        effects.add("polls")
    if p.switches_frame or p.returns:
        effects.add("switches_frame")
    c.effects = ordered_effects(effects)
    exits = {EXIT_NAME[t] for t in facts["transfers"]}
    exits |= {GUARD_EXIT[k] for k in facts["guard_kinds"]}
    c.exits = [e for e in EXIT_ORDER if e in exits]
    feedback = set()
    for tkn, _ in ir_contract.body_calls(uop):
        row = table.get(tkn.text)
        if row is not None:
            feedback |= row.feedback
    c.feedback = [f"{m}({f})" for m, f in sorted(feedback)]
    # Requirements: the facts that a row requires of an input where the
    # body itself does not establish them first (the walk's refusals,
    # collected by main() into overrides)
    required = collections.defaultdict(set)
    for name, facts_ in overrides.get(REQUIRES, {}).items():
        required[name] |= facts_
    def words(fs) -> list[str]:
        return [str(f) for f in sorted(fs)]
    def vocab(found, what) -> set:
        result = set()
        for kind, arg in sorted(found):
            f = vocabulary_fact(kind, arg)
            if f is None:
                raise Gap(f"{what}: {kind}({arg})")
            if f != ir_contract.NONNULL:
                result.add(f)
        return result
    ins = {i.name: i for i in uop.stack.inputs}
    for item in uop.stack.inputs:
        if item.name == "unused":
            continue
        w = []
        if not item.peek:
            w.append(overrides.get(item.name, "consumed"))
        w += words(required.get(item.name, ()))
        if w:
            c.inputs[item.name] = w
    record_out = {name: own for name, _, own in facts["outputs"]}
    for item in uop.stack.outputs:
        if item.name == "unused":
            continue
        own = record_out.get(item.name)
        if item.peek:
            found = vocab(facts["guards"].get(item.name, ()),
                          f"a guard fact of `{item.name}`")
            w = words(found - required.get(item.name, set()))
        elif own == ir_records.ALIAS:
            source = _alias_source(uop, item.name)
            if source is None:
                raise Gap(f"`{item.name}` is an alias of no input it is assigned from")
            found = vocab(facts["guards"].get(source, ()), f"a guard fact of `{source}`")
            w = [f"moved({source})"] + words(found)
        else:
            found = vocab(facts["results"].get(item.name, ()), f"a result fact of `{item.name}`")
            w = ["owned"] + words(found)
        if w:
            c.outputs[item.name] = w
    # A guard fact of a value that is not an output, or of something other
    # than a value: the vocabulary has no place for it
    names = {o.name for o in uop.stack.outputs} | {
        o for o in (_alias_source(uop, x.name) for x in uop.stack.outputs) if o}
    for subject, found in facts["guards"].items():
        if subject not in names and subject not in ins:
            raise Gap(f"guard facts of `{subject}`: "
                      f"{', '.join(f'{k}({a})' for k, a in sorted(found))}")
        if subject in ins and subject not in names:
            vocab(found, f"a guard fact of the consumed input `{subject}`")
    return c


def _alias_source(uop, name: str) -> str | None:
    ins = {i.name for i in uop.stack.inputs}
    for where, rhs in ir_contract._assignments(uop, name):
        rhs = ir_records._uncast(rhs)
        if len(rhs) == 1 and rhs[0] in ins:
            return rhs[0]
        # `out = TAKE(in)`: the move of an input (ir_generator.VIEW_TAKES)
        args = ir_records._call(rhs, *sorted(ir_generator.VIEW_TAKES - {"PyValue_AsKnownObjectSteal"}))
        if args is not None and len(args) == 1 and len(args[0]) == 1 and args[0][0] in ins:
            return args[0][0]
    return None


def find_header(text: str, name: str) -> tuple[int, int, int, int] | None:
    """(start of the op keyword, start of the stack effect's `(`, end of the
    op's `)`, the body's `{`) of uop name's definition"""
    # An instruction's own uop is named after it with a leading underscore
    m = (re.search(rf"\bop\(\s*{re.escape(name)}\s*,", text)
         or re.search(rf"\binst\(\s*{re.escape(name[1:])}\s*,", text))
    if m is None:
        return None
    i = m.end()
    while text[i] != "(":
        i += 1
    effect = i
    depth = 0
    for j in range(effect, len(text)):
        depth += {"(": 1, ")": -1}.get(text[j], 0)
        if depth == 0:
            break
    close = text.index(")", j + 1)
    brace = text.index("{", close)
    return m.start(), effect, close, brace


def annotate_effect(inner: str, c: Clauses) -> str:
    "the stack effect's text with each item's modes and facts"
    before, sep, after = inner.partition("--")

    def side(text: str, words: dict[str, list[str]]) -> str:
        parts = ir_contract._split_text(text)
        out = []
        for part in parts:
            m = re.fullmatch(r"(\s*)(\w+)(\[[^\]]*\])?(\s*:\s*([\w ]+))?(\s*)", part)
            if m is None or "/" in part or m.group(2) not in words:
                out.append(part)
                continue
            existing = (m.group(5) or "").split()
            w = words[m.group(2)] + existing
            out.append(f"{m.group(1)}{m.group(2)}{m.group(3) or ''}: {' '.join(w)}{m.group(6)}")
        return ",".join(out)

    return side(before, c.inputs) + sep + side(after, c.outputs)


def render_uop(text: str, name: str, c: Clauses) -> str:
    start, effect, close, brace = find_header(text, name)
    depth = 0
    for j in range(effect, len(text)):
        depth += {"(": 1, ")": -1}.get(text[j], 0)
        if depth == 0:
            break
    stack = annotate_effect(text[effect + 1:j], c)
    line_start = text.rfind("\n", 0, start) + 1
    indent = re.match(r"\s*", text[line_start:]).group(0)
    head = text[line_start:effect]
    one_line = f"{head}({stack}))"
    if len(one_line) > 92:
        stack = " ".join(stack.split())
        one_line = f"{head.rstrip()}\n{indent}        ({stack}))"
        if len(one_line.split("\n")[-1]) > 92 and " -- " in stack:
            left, right = stack.split(" -- ", 1)
            one_line = (f"{head.rstrip()}\n{indent}        ({left}\n"
                        f"{indent}         -- {right}))")
    clauses = [f"effects({', '.join(c.effects)})", f"exits({', '.join(c.exits)})"]
    if c.feedback:
        clauses.append(f"feedback({', '.join(c.feedback)})")
    tail = text[close + 1:brace].strip()
    if tail:
        raise ValueError(f"{name}: text between its header and body: {tail!r}")
    new = f"{one_line}\n{indent}    {' '.join(clauses)}\n{indent}{{"
    return text[:line_start] + new + text[brace + 1:]


# ---------------------------------------------------------------------------
# The check: the generator's records over the seeded files

CHECK = r'''
import json, sys
sys.path.insert(0, sys.argv[1])
import analyzer, ir_contract, ir_generator, ir_records, ir_leaf
errors = {}
current = {}
def guarded(module, name):
    original = getattr(module, name)
    def call(*args, **kw):
        try:
            return original(*args, **kw)
        except Exception as e:
            key = current.get("uop") or current.get("handler")
            errors.setdefault(key, f"{type(e).__name__}: {e}")
            if name in ("record_guard_facts",):
                return (("<error>",), "")
            if name == "record_results":
                return ("<error>",)
            return None
    setattr(module, name, call)
original_verify_uop = ir_contract.verify_uop
def verify_uop(uop, *a, **kw):
    current["uop"] = uop.name
    try:
        return original_verify_uop(uop, *a, **kw)
    except Exception as e:
        errors.setdefault(uop.name, f"{type(e).__name__}: {e}")
ir_contract.verify_uop = verify_uop
original_verify_handler = ir_contract.verify_handler
def verify_handler(inst, *a, **kw):
    current["uop"] = None
    try:
        return original_verify_handler(inst, *a, **kw)
    except Exception as e:
        errors.setdefault("handler:" + inst.name, f"{type(e).__name__}: {e}")
ir_contract.verify_handler = verify_handler
guarded(ir_contract, "record_guard_facts")
guarded(ir_contract, "record_results")
for mod, name in ((ir_generator, "check_record"), (ir_leaf, "check_leaf"),
                  (ir_contract, "check_primitives")):
    original = getattr(mod, name)
    def make(original, name):
        def call(record, *a, **kw):
            try:
                return original(record, *a, **kw)
            except Exception as e:
                key = "record:" + getattr(record, "handler", name)
                errors.setdefault(key, f"{type(e).__name__}: {e}")
        return call
    setattr(mod, name, make(original, name))
analysis = analyzer.analyze_files([sys.argv[2]])
records = ir_generator.build_records(analysis, ir_generator.compute_formats(analysis))
uops = {u.name: u for u in analysis.uops.values()}
for inst in analysis.instructions.values():
    for part in inst.parts:
        if isinstance(part, analyzer.Uop):
            uops.setdefault(part.name, part)
out = {}
for handler, record in records.items():
    out[handler] = [{"name": u.name, "effects": sorted(u.effects),
                     "guards": [[g.kind, [str(f) for f in g.facts], g.reason, g.site is None,
                                 g.input] for g in u.guards],
                     "results": [str(f) for f in u.results],
                     "declared": u.name in uops and ir_contract.contract_of(uops[u.name]) is not None}
                    for u in record.uops]
json.dump({"errors": errors, "records": out}, sys.stdout)
'''


def check(scratch: str) -> dict:
    script = os.path.join(scratch, "check.py")
    with open(script, "w") as f:
        f.write(CHECK)
    run = subprocess.run([sys.executable, "-B", script, str(ROOT / "Tools/cases_generator"),
                          str(BYTECODES)], capture_output=True, text=True)
    if run.returncode:
        raise RuntimeError(run.stderr[-4000:])
    return json.loads(run.stdout)


def normal(key: str, value):
    "a record field with its facts in a canonical order"
    if key == "results":
        return sorted(value)
    if key == "guards":
        return [[g[0], sorted(g[1]), *g[2:]] for g in value]
    return value


FULL = r'''
import json, re, sys
sys.path.insert(0, sys.argv[1])
import analyzer, ir_contract_diff
lines = ir_contract_diff.report(sys.argv[2])
analysis, records = ir_contract_diff.build(sys.argv[2], declared=True)
found = {}
for line in lines[:-1]:
    m = re.match(r"(\w+)\.record\.uops\[(\d+)\]", line)
    n = re.match(r"(\w+)/(\w+): ", line)
    if m:
        names = [records[m.group(1)].uops[int(m.group(2))].name]
    elif n:
        names = [n.group(2)]
    else:
        handler = re.match(r"(\w+)", line).group(1)
        names = [u.name for u in records[handler].uops] if handler in records else []
    for name in names:
        found.setdefault(name, []).append(line)
json.dump(found, sys.stdout)
'''


def full_differences(scratch: str) -> dict[str, list[str]]:
    "ir_contract_diff's report on the seeded files, by uop"
    script = os.path.join(scratch, "full.py")
    with open(script, "w") as f:
        f.write(FULL)
    run = subprocess.run([sys.executable, "-B", script, str(ROOT / "Tools/cases_generator"),
                          str(BYTECODES)], capture_output=True, text=True)
    if run.returncode:
        raise RuntimeError(run.stderr[-4000:])
    return json.loads(run.stdout)


def differences(before: dict, after: dict) -> dict[str, list[str]]:
    "per uop name, how its seeded records differ from the inferred ones"
    found = collections.defaultdict(list)
    for handler, uops in before.items():
        for b, a in zip(uops, after.get(handler, ())):
            for key in ("effects", "guards", "results"):
                if normal(key, b[key]) != normal(key, a[key]):
                    found[b["name"]].append(f"{handler}: {key}: inferred {b[key]}, "
                                            f"declared {a[key]}")
    return found


# ---------------------------------------------------------------------------

OVERRIDE = re.compile(r"on raise, `(\w+)` is still owned, but the site declares consumed")
# DEAD(x) of an input the body neither releases nor moves: the skeleton
# closes it after the uop, on next and on raise, which is `closed` (F4)
CLOSE = re.compile(r"DEAD\(`(\w+)`\) drops an owned value")
REQUIREMENT = re.compile(r"passes `([^`]*)` to (\w+), which requires ([\w(), ]+) of it")
REQUIRES = "requires"     # the overrides' key of an input's requirements


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", required=True)
    parser.add_argument("--report", default=None, help="write the seed's report (JSON) here")
    parser.add_argument("--only", default=None, help="seed only these uops (comma-separated)")
    args = parser.parse_args()
    original_text = BYTECODES.read_text()
    original_header = HEADER.read_text()
    with tempfile.TemporaryDirectory() as scratch:
        analysis = analyzer.analyze_files([str(BYTECODES)])
        before = inferred_records(analysis)
        table = ir_contract.rows()
        ctypes, macros = compiler_view(args.build, scratch)
        rows, refused = seed_rows(analysis, table, ctypes, macros, callee_effects(analysis))
        header = render_rows(original_header, rows) if rows else original_header
        HEADER.write_text(header)
        table = ir_contract.parse_rows(header)
        facts = uop_facts(analysis, before)
        candidates = sorted(u.name for u in all_uops(analysis).values()
                            if not u.clauses and u.name in facts)
        if args.only:
            candidates = [n for n in candidates if n in args.only.split(",")]
        excluded: dict[str, str] = {}
        overrides: dict[str, dict] = collections.defaultdict(dict)
        rounds = 0
        while True:
            rounds += 1
            text = original_text
            seeded = []
            for name in candidates:
                if name in excluded:
                    continue
                uop = all_uops(analysis)[name]
                if any(n in refused for n in (t.text for t, _ in ir_contract.body_calls(uop))):
                    excluded[name] = "calls " + ", ".join(sorted(
                        {t.text for t, _ in ir_contract.body_calls(uop)} & set(refused)
                    )) + ", which no row can declare"
                    continue
                try:
                    c = seed_clauses(uop, facts[name], table, overrides[name])
                except Gap as e:
                    excluded[name] = f"vocabulary: {e}"
                    continue
                if find_header(text, name) is None:
                    excluded[name] = "no definition in bytecodes.c of this name"
                    continue
                text = render_uop(text, name, c)
                seeded.append(name)
            BYTECODES.write_text(text)
            try:
                result = check(scratch)
            except Exception:
                BYTECODES.write_text(original_text)
                HEADER.write_text(original_header)
                raise
            diffs = differences(before, result["records"])
            changed = False
            retried: set[str] = set()
            for key, error in result["errors"].items():
                name = key
                m = OVERRIDE.search(error)
                if m and name in seeded and overrides[name].get(m.group(1)) != "taken":
                    overrides[name][m.group(1)] = "taken"
                    retried.add(name)
                    changed = True
                    continue
                m = CLOSE.search(error)
                if m and name in seeded and overrides[name].get(m.group(1)) != "closed":
                    overrides[name][m.group(1)] = "closed"
                    retried.add(name)
                    changed = True
                    continue
                r = REQUIREMENT.search(error)
                if r and name in seeded:
                    uop = all_uops(analysis)[name]
                    views = ir_contract.Views(uop, table)
                    target = views.resolve(r.group(1).split(), locals_=False)
                    facts_ = {ir_contract.parse_fact(w, None)
                              for w in r.group(3).replace(",", " ").split()}
                    have = (overrides[name].setdefault(REQUIRES, {}).setdefault(target, set())
                            if target is not None else facts_)
                    if not facts_ <= have:
                        have |= facts_
                        retried.add(name)
                        changed = True
                        continue
                if name in seeded:
                    excluded[name] = error
                    changed = True
                elif key.startswith("handler:") or key.startswith("record:"):
                    handler = key.split(":", 1)[1]
                    inst = ir_generator.handler_instructions(analysis).get(handler) \
                        or analysis.instructions.get(handler)
                    parts = [p.name for p in inst.parts if isinstance(p, analyzer.Uop)] \
                        if inst is not None else []
                    culprits = [p for p in parts if p in seeded]
                    # A handler-level refusal: the seeded uops of the handler
                    # stay undeclared, so that the handler is mixed again
                    for p in culprits:
                        excluded[p] = f"{key}: {error}"
                        changed = True
            for name, lines in diffs.items():
                if name in seeded and name not in excluded and name not in retried:
                    excluded[name] = "record differs: " + " | ".join(lines[:3])
                    changed = True
            print(f"round {rounds}: {len(seeded)} seeded, {len(excluded)} excluded, "
                  f"{len(result['errors'])} errors", file=sys.stderr)
            if not changed and not result["errors"]:
                # The whole records, as the review script compares them
                for name, lines in full_differences(scratch).items():
                    if name in seeded and name not in excluded:
                        excluded[name] = "record differs: " + " | ".join(lines[:3])
                        changed = True
            if not changed:
                break
        report = {"rows": [{"name": r.name, "kind": r.kind, "group": r.group,
                            "tables": r.tables} for r in rows],
                  "refused_rows": refused, "seeded": seeded, "excluded": excluded,
                  "overrides": {k: v for k, v in overrides.items() if v},
                  "remaining_errors": result["errors"]}
        if args.report:
            with open(args.report, "w") as f:
                json.dump(report, f, indent=1, sort_keys=True,
                          default=lambda v: sorted(map(str, v)))
        print(f"{len(rows)} rows, {len(refused)} callees without a row, "
              f"{len(seeded)} uops seeded, {len(excluded)} not", file=sys.stderr)


if __name__ == "__main__":
    main()
