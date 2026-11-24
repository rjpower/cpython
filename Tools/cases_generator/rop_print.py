"""Print the data model back as `.rop` source.

`parse(print_file(f))` equals `f` for every parsed file `f` (spans aside):
the printer is the model's canonical concrete syntax.
"""

from rop_model import (Access, Advance, Arm, Block, BoolLit, Branch, Call, CallTerm,
                       Cases, CaseDecl, Contract, Decl, Each, EdgeFact, EffectDecl,
                       EnumDecl, EnumType, Exit, Expr, Extern, FactUse, Family, Goto,
                       Graph, Guard, HandleType, Let, Macro, Name, Nat, Param,
                       PairType, Point, Publish, Result, Returns, ScalarType,
                       SchemaType, SchemaVar, SourceFile, SpanLit, Stmt, Target, Term,
                       Type, WrapType)


def type_text(t: Type) -> str:
    if isinstance(t, ScalarType):
        return t.scalar.value
    if isinstance(t, HandleType):
        return t.handle.value
    if isinstance(t, WrapType):
        return f"{t.wrap.value}<{type_text(t.arg)}>"
    if isinstance(t, PairType):
        return f"Pair<{type_text(t.first)}, {type_text(t.second)}>"
    if isinstance(t, SchemaType):
        schema = t.schema.name if isinstance(t.schema, SchemaVar) else t.schema.value
        return f"{t.ctor.value}<{schema}>"
    assert isinstance(t, EnumType)
    return f"Enum<{t.name}>"


def expr_text(e: Expr) -> str:
    if isinstance(e, Name):
        return e.name
    if isinstance(e, Nat):
        return str(e.value)
    if isinstance(e, BoolLit):
        return "true" if e.value else "false"
    if isinstance(e, Call):
        return f"{e.callee}({_join(e.args)})"
    assert isinstance(e, SpanLit)
    return f"[{_join(e.items)}]"


def _join(exprs: tuple[Expr, ...]) -> str:
    return ", ".join(expr_text(e) for e in exprs)


def _params(params: tuple[Param, ...]) -> str:
    return "(" + ", ".join(f"{p.name}:{type_text(p.type)}" for p in params) + ")"


def _names(names: tuple[str, ...]) -> str:
    return "(" + ", ".join(names) + ")"


def _schemas(schemas: tuple[str, ...]) -> str:
    return f"<{', '.join(schemas)}>" if schemas else ""


def _fact(f: FactUse) -> str:
    type_arg = f"<{f.type_arg.name}>" if f.type_arg else ""
    return f"{f.fact.value}{type_arg}({', '.join(f.subjects)})"


def _result(r: Result) -> str:
    if isinstance(r, Returns):
        return f"-> {type_text(r.type)}"
    return "cases(" + ", ".join(f"{c.name}{_params(c.params)}" for c in r.cases) + ")"


def _effect(e: EffectDecl) -> str:
    return e.effect.value + (f" on {_names(e.on)}" if e.on else "")


def _contract(c: Contract, feedback: bool, exits: bool) -> list[str]:
    lines = [f"  effects({', '.join(_effect(e) for e in c.effects)})"]
    if feedback or c.feedback:
        accesses = ", ".join(f"{a.mode.value}({a.field})" for a in c.feedback)
        lines.append(f"  feedback({accesses})")
    if exits or c.exits:
        lines.append(f"  exits{_names(c.exits)}")
    if c.requires:
        lines.append(f"  requires({', '.join(_fact(f) for f in c.requires)})")
    if c.ensures:
        facts = ", ".join(f"{e.edge}.{_fact(e.fact)}" for e in c.ensures)
        lines.append(f"  ensures({facts})")
    return lines


def _target(t: Target) -> str:
    return f"{t.label}({_join(t.args)})"


def _stmt(s: Stmt) -> str:
    if isinstance(s, Guard):
        return f"guard {expr_text(s.cond)} else miss;"
    if isinstance(s, Let):
        return f"let {s.name}:{type_text(s.type)} = {expr_text(s.value)};"
    assert isinstance(s, Publish)
    bindings = ", ".join(f"{b.target} = {expr_text(b.value)}" for b in s.bindings)
    debts = ", ".join(f"{d.kind.value}({d.subject})" for d in s.debts)
    return f"publish [{bindings}] retire [{debts}] at {s.point};"


def _term(t: Term) -> list[str]:
    if isinstance(t, Goto):
        return [f"goto {_target(t.target)};"]
    if isinstance(t, Branch):
        return [f"branch {expr_text(t.cond)} -> {_target(t.if_true)}, "
                f"{_target(t.if_false)};"]
    if isinstance(t, CallTerm):
        return ([f"call {t.callee}({_join(t.args)}) {{"]
                + [f"  {e.case} -> {_target(e.target)};" for e in t.edges] + ["}"])
    if isinstance(t, Each):
        limit = "length" if t.limit is None else str(t.limit)
        return [f"each {expr_text(t.over)} limit {limit} body {t.body} "
                f"exhaust {_target(t.exhaust)};"]
    if isinstance(t, Advance):
        return [f"advance {t.body};"]
    assert isinstance(t, Exit)
    return [f"exit {t.kind}({_join(t.args)});"]


def _graph(g: Graph) -> list[str]:
    lines = ["{"]
    for b in g.blocks:
        lines.append(f"  {b.label}{_params(b.params)}:")
        lines.extend(f"    {_stmt(s)}" for s in b.stmts)
        lines.extend(f"    {line}" for line in _term(b.term))
    lines.append("}")
    return lines


def _string(text: str) -> str:
    return f'"{text}"'


def decl_lines(d: Decl) -> list[str]:
    if isinstance(d, EnumDecl):
        return [f"enum {d.name}{_names(d.members)};"]
    if isinstance(d, Family):
        return [f"family {d.name}{_params(d.params)}",
                f"  exits{_names(d.exits)};"]
    if isinstance(d, Point):
        return [f"point {d.name}({d.frame.value}, {d.pc.value}, {d.exc.value}) "
                f"keep{_names(d.keep)} spent{_names(d.spent)} defs{_names(d.defs)} "
                f"held{_names(d.held)};"]
    if isinstance(d, Arm):
        return ([f"arm {d.name} {d.region.value}"]
                + _contract(d.contract, True, True) + _graph(d.graph))
    if isinstance(d, Macro):
        head = [f"macro {d.name}{_schemas(d.schema_params)}{_params(d.params)}",
                f"  {_result(d.result)}"] + _contract(d.contract, True, False)
        if d.graph is None:
            return head + ["  pending;"]
        return head + _graph(d.graph)
    assert isinstance(d, Extern)
    lines = [f"extern {d.kind.value} {d.name}{_schemas(d.schema_params)}"
             f"{_params(d.params)}", f"  {_result(d.result)}"]
    lines += _contract(d.contract, False, False)
    if d.test:
        lines.append(f"  test({', '.join(_fact(f) for f in d.test)})")
    if d.c is None:
        return lines + ["  pending;"]
    lines.append(f"  c {d.c.symbol} as {d.c.convention}")
    if d.c.evidence:
        items = [f"    {e.key}: {_string(e.text)}" for e in d.c.evidence]
        lines += ["  evidence("] + [item + "," for item in items[:-1]] + [items[-1] + ")"]
    lines[-1] += ";"
    return lines


def print_file(f: SourceFile) -> str:
    return "\n\n".join("\n".join(decl_lines(d)) for d in f.decls) + "\n"
