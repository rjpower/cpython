#!/usr/bin/env python3
"""Remove exception saves and restores around plain releases.

Under the release contract (Include/internal/pycore_value.h, "Release
contract"), freeing an int or float box never runs Python and never changes
the current exception, and every other deallocator leaves the current
exception unchanged.  So

    PyObject *saved = PyErr_GetRaisedException();
    Py_XDECREF(item);
    _PyValue_RestoreException(saved);

is the same as ``Py_XDECREF(item);``.  This script makes that rewrite, and
the related ones of steps 6a-6c of the PyValue representation packet
(pyjit/design/proposals/pyvalue-representation-packet.md, section 3):

  --rules direct    rules 1-4: a save and a restore in one block with only
                    releases, detaches, blank lines and comments between.
  --rules helpers   rule 5: calls of a small release helper become the
                    release in the helper's body; the helper goes.
  --rules class-p   rule 6: ``|| PyErr_Occurred()`` after a call whose
                    failure the left term reports, and the dead NULL test
                    after Py_NewRef.  The term stays when an exception
                    raised earlier may still be pending (after a
                    PyIter_Next loop, for example): it tests that too.

It refuses anything it does not recognise and prints each refused site as
``file:line: class X: reason``.  The class letters are those of the section
3.1 classifier: A (only object releases), B (releases with a PyValue close),
C (array release), D (releases and other statements), E (nothing between),
F (no release), U (no save within 60 lines); P (class P), H (helper).

With --check it changes no file and prints the diff it would apply.
Generated files are only checked; change their generators instead.
"""

from __future__ import annotations

import argparse
import difflib
import os
import re
import sys
from dataclasses import dataclass, field

GENERATED = ("Python/Python-ast.c", "Python/generated_ir_cases.c.h",
             "Python/generated_jit_helpers.c.h", "Python/generated_cases.c.h",
             "Python/executor_cases.c.h", "Python/optimizer_cases.c.h",
             "Python/generated_tracer_cases.c.h")

POINTER_RELEASES = ("Py_DECREF", "Py_XDECREF", "Py_CLEAR")
VALUE_RELEASES = ("PyValue_CLOSE", "PyValue_XCLOSE", "PyValue_CLEAR",
                  "PyValue_Close", "_PyValue_CLOSE_POINTER",
                  "_PyValue_XCLOSE_POINTER")
RELEASES = POINTER_RELEASES + VALUE_RELEASES

IDENT = r"[A-Za-z_]\w*"
# An lvalue without calls: name, *name, a->b, a.b, a[i], and combinations.
# Index expressions may use members and + or -, but not ++ or --: the release
# macros evaluate their argument once, and a rewrite must not add side effects.
LVALUE = (r"\**[A-Za-z_][\w]*(?:(?:->|\.)[A-Za-z_]\w*"
          r"|\[(?:[\w\s]|->|\.|[+-](?![+-]))*\])*")

RESTORE_RE = re.compile(r"^\s*_PyValue_RestoreException\(\s*(" + IDENT + r")\s*\);\s*$")
RESTORE_ANY_RE = re.compile(r"_PyValue_RestoreException\(")
DETACH_DECL_RE = re.compile(
    r"^\s*(?P<type>PyValue|(?:const\s+)?" + IDENT + r"\s*\*+)\s*(?P<r>" + IDENT +
    r")\s*=\s*(?P<cast>\(\s*PyObject\s*\*\s*\)\s*)?(?P<x>" + LVALUE + r")\s*;\s*$")
DETACH_NULL_RE = re.compile(r"^\s*(?P<x>" + LVALUE + r")\s*=\s*(?P<null>NULL|PyValue_NULL)\s*;\s*$")
RELEASE_RE = re.compile(r"^(?P<indent>\s*)(?P<fn>" + IDENT + r")\((?P<arg>.*)\);\s*$")

# The section 3.1 classifier (build/pyjit-link-wave1/repr-step5-classify.py).
CL_RELEASE_OBJ = re.compile(r"^\s*(Py_X?DECREF|Py_CLEAR|Py_DECREF_MORTAL)\s*\(")
CL_RELEASE_VAL = re.compile(
    r"^\s*(PyValue_X?CLOSE\w*|PyValue_CLEAR|PyValue_Close|_PyValue_X?CLOSE_POINTER|"
    r"_PyValue_CLOSE_SPECIALIZED\w*|PyValue_CLOSE_SPECIALIZED)\s*\(")
CL_LOOP = re.compile(r"_PyObjectArray_Free|_PyValueTemps_Close|_PyValue_CloseStack")
CL_DETACH1 = re.compile(r"^\s*(?:[\w\s\*]+?\s\*?)?(\w+)\s*=\s*([\w>\.\-\*\[\]]+)\s*;\s*$")
CL_DETACH2 = re.compile(r"^\s*[\w>\.\-\*\[\]]+\s*=\s*(NULL|PyValue_NULL)\s*;\s*$")
CL_TRIVIAL = re.compile(r"^\s*(\{|\}|//.*|/\*.*\*/|\*.*|)\s*$")


def classify(lines: list[str], save: int | None, restore: int) -> str:
    if save is None:
        return "U"
    body = [l for l in lines[save + 1:restore] if not CL_TRIVIAL.match(l)]
    if not body:
        return "E"
    is_obj = [bool(CL_RELEASE_OBJ.match(l)) for l in body]
    is_val = [bool(CL_RELEASE_VAL.match(l)) for l in body]
    has_loop = any(CL_LOOP.search(l) for l in body)
    other = [l for l in body if not (CL_RELEASE_OBJ.match(l) or CL_RELEASE_VAL.match(l)
                                     or CL_DETACH1.match(l) or CL_DETACH2.match(l)
                                     or CL_LOOP.search(l))]
    if has_loop and not other:
        return "C"
    if not any(is_obj) and not any(is_val) and not has_loop:
        return "F"
    if other:
        return "D"
    return "B" if any(is_val) else "A"


def code_only(lines: list[str]) -> list[str]:
    """Each line with comments, strings and character literals blanked."""
    out = []
    in_comment = False
    for line in lines:
        chars = list(line)
        i = 0
        n = len(chars)
        while i < n:
            if in_comment:
                if line.startswith("*/", i):
                    chars[i] = chars[i + 1] = " "
                    in_comment = False
                    i += 2
                    continue
                chars[i] = " "
                i += 1
                continue
            c = line[i]
            if line.startswith("/*", i):
                in_comment = True
                chars[i] = chars[i + 1] = " "
                i += 2
                continue
            if line.startswith("//", i):
                for k in range(i, n):
                    chars[k] = " "
                break
            if c in "\"'":
                quote = c
                k = i + 1
                while k < n and line[k] != quote:
                    if line[k] == "\\":
                        chars[k] = " "
                        k += 1
                    if k < n:
                        chars[k] = " "
                    k += 1
                i = k + 1
                continue
            i += 1
        out.append("".join(chars))
    return out


def depths(code: list[str]) -> list[int]:
    """The brace depth at the start of each line.  Each branch of an #if
    starts at the depth of the #if, and the depth after the #endif is the
    depth after its first branch (branches that open a brace each open it)."""
    result = []
    depth = 0
    stack: list[list[int | None]] = []  # [depth at #if, depth after first branch]
    for line in code:
        result.append(depth)
        directive = re.match(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b", line)
        if directive:
            kind = directive.group(1)
            if kind.startswith("if"):
                stack.append([depth, None])
            elif stack and kind in ("elif", "else"):
                if stack[-1][1] is None:
                    stack[-1][1] = depth
                depth = stack[-1][0]
            elif stack and kind == "endif":
                start, first = stack.pop()
                if first is not None:
                    depth = first
            continue
        depth += line.count("{") - line.count("}")
    return result


def balanced(text: str) -> bool:
    level = 0
    for c in text:
        if c == "(":
            level += 1
        elif c == ")":
            level -= 1
            if level < 0:
                return False
    return level == 0


def norm(text: str) -> str:
    return re.sub(r"\s+", "", text)


@dataclass
class Refusal:
    line: int  # 0-based
    cls: str
    reason: str


@dataclass
class Edits:
    """Line edits: a deleted line maps to [], a replaced one to new lines."""
    replace: dict[int, list[str]] = field(default_factory=dict)

    def delete(self, i: int) -> None:
        self.replace[i] = []

    def apply(self, lines: list[str]) -> list[str]:
        out = []
        for i, line in enumerate(lines):
            if i in self.replace:
                out.extend(self.replace[i])
            else:
                out.append(line)
        return out


class Source:
    def __init__(self, path: str, text: str) -> None:
        self.path = path
        self.lines = text.split("\n")
        self.code = code_only(self.lines)
        self.depth = depths(self.code)

    def block_end(self, i: int) -> int:
        """The line that closes the block containing line i."""
        d = self.depth[i]
        for k in range(i + 1, len(self.lines)):
            if self.depth[k] + self.code[k].count("{") - self.code[k].count("}") < d:
                return k
        return len(self.lines) - 1

    def block_start(self, i: int) -> int:
        """The line that opens the block containing line i."""
        d = self.depth[i]
        for k in range(i - 1, -1, -1):
            if self.depth[k] < d:
                return k
        return 0

    def uses(self, name: str, start: int, end: int) -> list[int]:
        pattern = re.compile(r"\b" + re.escape(name) + r"\b")
        return [k for k in range(start, end + 1) if pattern.search(self.code[k])]


def is_comment_or_blank(code_line: str) -> bool:
    return not code_line.strip()


def generated(path: str) -> bool:
    p = path.replace(os.sep, "/")
    return any(p.endswith(g) for g in GENERATED)


# ---------------------------------------------------------------------------
# Rules 1-4: direct sites


@dataclass
class Site:
    save: int
    restore: int
    start: int  # first line of the site (a detach before the save, or the save)
    var: str
    declared_at_save: bool


def find_save(src: Source, restore: int, var: str) -> int | None:
    save = re.compile(r"\b" + re.escape(var) + r"\s*=\s*(?:_?PyErr_GetRaisedException)\(")
    for k in range(restore - 1, max(-1, restore - 61), -1):
        if save.search(src.lines[k]):
            return k
    return None


def direct_sites(src: Source, refusals: list[Refusal], edits: Edits) -> int:
    """Rules 1-4.  Return the number of sites rewritten."""
    lines, code = src.lines, src.code
    sites: list[tuple[Site, dict[int, list[str]]]] = []
    for i, line in enumerate(lines):
        if not RESTORE_ANY_RE.search(code[i]):
            continue
        stripped = line.lstrip()
        if (stripped.startswith(("PyAPI_FUNC", "void", "static", "extern"))
                or "_PyValue_RestoreException(PyObject" in line):
            continue
        m = re.search(r"_PyValue_RestoreException\(\s*([A-Za-z_][\w>.\-]*)\s*\)", line)
        if m is None:
            continue
        var = m.group(1)
        save = find_save(src, i, var)
        cls = classify(lines, save, i)
        if cls not in "AB":
            refusals.append(Refusal(i, cls, CLASS_REASONS[cls]))
            continue
        reason = check_direct(src, save, i, var)
        if isinstance(reason, str):
            refusals.append(Refusal(i, cls, reason))
            continue
        site, replacement = reason
        sites.append((site, replacement))

    # Rule 4: the saved variable's declaration.  A save that declares it
    # may go only if no line that stays uses it; an earlier declaration goes
    # when no line that stays uses it.  Refusing a site keeps its uses, so
    # repeat until nothing changes.
    while True:
        deleted = set()
        for site, repl in sites:
            deleted.update(range(site.start, site.restore + 1))
        refuse: list[tuple[int, str]] = []
        declarations: dict[int, list[int]] = {}
        for n, (site, repl) in enumerate(sites):
            if site.declared_at_save:
                end = src.block_end(site.save)
                later = [k for k in src.uses(site.var, site.restore + 1, end) if k not in deleted]
                if later:
                    refuse.append((n, f"saved variable {site.var} declared here is used at line {later[0] + 1}"))
                continue
            decl = find_declaration(src, site.var, site.save)
            if decl is not None:
                declarations.setdefault(decl, []).append(n)
        removals: dict[int, list[str]] = {}
        for decl, members in declarations.items():
            var = sites[members[0]][0].var
            end = src.block_end(decl)
            if [k for k in src.uses(var, decl, end) if k not in deleted and k != decl]:
                continue
            new = remove_declarator(lines[decl], var)
            if new is None:
                refuse.extend((n, f"declaration of {var} at line {decl + 1} cannot lose it")
                              for n in members)
            else:
                removals[decl] = new
        if not refuse:
            break
        for n, reason in sorted(refuse, reverse=True):
            site = sites[n][0]
            refusals.append(Refusal(site.restore, classify(lines, site.save, site.restore), reason))
            del sites[n]

    for site, repl in sites:
        for k in range(site.start, site.restore + 1):
            edits.replace[k] = repl.get(k, [])
    edits.replace.update(removals)

    unwrap_blocks(src, edits, {s.start for s, _ in sites})
    return len(sites)


def remove_declarator(line: str, var: str) -> list[str] | None:
    """The declaration line without var's declarator, or None if that is
    not a simple edit."""
    v = re.escape(var)
    if re.match(r"^\s*PyObject\s*\*\s*" + v + r"\s*(=\s*NULL\s*)?;\s*$", line):
        return []
    m = re.match(r"^(.*\S)\s*,\s*\*\s*" + v + r"\s*(?:=\s*NULL\s*)?;\s*$", line)
    if m and line.lstrip().startswith("PyObject"):
        return [m.group(1) + ";"]
    m = re.match(r"^(\s*PyObject\s*)\*\s*" + v + r"\s*(?:=\s*NULL\s*)?,\s*(\*.*)$", line)
    if m:
        return [m.group(1) + m.group(2)]
    return None


CLASS_REASONS = {
    "C": "array release (step 7)",
    "D": "releases and other statements (step 7)",
    "E": "nothing between save and restore (step 7)",
    "F": "no release between save and restore (step 7)",
    "U": "no matching save within 60 lines (step 7)",
}


def find_declaration(src: Source, var: str, before: int) -> int | None:
    """The declaration of var in a block that encloses line before."""
    decl2 = re.compile(r"^\s*PyObject\s*\*\s*" + re.escape(var) + r"\s*(?:=\s*NULL\s*)?[,;]|"
                       r"^\s*PyObject\s*\*.*,\s*\*\s*" + re.escape(var) + r"\s*(?:=\s*NULL\s*)?[,;]")
    for k in range(before - 1, -1, -1):
        if src.depth[k] == 0:
            return None
        if decl2.match(src.code[k]) and src.block_end(k) >= before:
            return k
    return None


def check_direct(src: Source, save: int, restore: int, var: str):
    """Rules 1-3 for one site: a reason to refuse, or (site, replacement)."""
    lines, code = src.lines, src.code
    if lines[restore].rstrip().endswith("\\") or lines[save].rstrip().endswith("\\"):
        return "inside a macro definition"
    if not RESTORE_RE.match(code[restore]):
        return "restore is not a statement of its own"
    m = re.match(r"^\s*(PyObject\s*\*\s*)?" + re.escape(var) + r"\s*=\s*PyErr_GetRaisedException\(\);\s*$", code[save])
    if m is None:
        return "save is not a statement of its own"
    declared = m.group(1) is not None
    d = src.depth[save]
    for k in range(save, restore + 1):
        if src.depth[k] != d or "{" in code[k] or "}" in code[k]:
            return "save and restore are not in one block"

    # Classify the lines between.
    detaches: dict[int, tuple[str, str, str, int]] = {}  # decl line -> (r, x, kind, null line)
    releases: list[int] = []
    start = save

    def scan_detach(k: int) -> tuple[str, str, str] | None:
        m = DETACH_DECL_RE.match(code[k])
        if m is None:
            return None
        kind = "value" if m.group("type") == "PyValue" else "pointer"
        if kind == "value" and m.group("cast"):
            return None
        return m.group("r"), m.group("x"), kind

    # A detach that immediately precedes the save.
    if save >= 2:
        before = scan_detach(save - 2)
        null = DETACH_NULL_RE.match(code[save - 1])
        if (before is not None and null is not None and norm(null.group("x")) == norm(before[1])
                and src.depth[save - 2] == d and src.depth[save - 1] == d
                and (null.group("null") == "PyValue_NULL") == (before[2] == "value")):
            detaches[save - 2] = (*before, save - 1)
            start = save - 2

    k = save + 1
    while k < restore:
        text = code[k]
        if is_comment_or_blank(text):
            k += 1
            continue
        if re.search(r"\b" + re.escape(var) + r"\b", text):
            return f"saved variable used between save and restore: {lines[k].strip()}"
        det = scan_detach(k)
        if det is not None:
            null = DETACH_NULL_RE.match(code[k + 1]) if k + 1 < restore else None
            if null is None or norm(null.group("x")) != norm(det[1]) or \
                    (null.group("null") == "PyValue_NULL") != (det[2] == "value"):
                return f"declaration that is not a detach: {lines[k].strip()}"
            detaches[k] = (*det, k + 1)
            k += 2
            continue
        rel = RELEASE_RE.match(text)
        if rel is not None and rel.group("fn") in RELEASES and balanced(rel.group("arg")):
            releases.append(k)
            k += 1
            continue
        if CL_RELEASE_OBJ.match(text) or CL_RELEASE_VAL.match(text):
            return f"release not in rule 2's list: {lines[k].strip()}"
        return f"other statement: {lines[k].strip()}"
    if not releases:
        return "no release between save and restore"

    # Rule 3: fold each detach into the release of its detached owner.
    replacement: dict[int, list[str]] = {}
    end = src.block_end(save)
    for decl, (r, x, kind, null_line) in sorted(detaches.items()):
        users = [k for k in src.uses(r, decl + 1, end)]
        rel_users = [k for k in releases if re.search(r"\b" + re.escape(r) + r"\b", code[k])]
        if users != rel_users or len(rel_users) != 1:
            if not users:
                return f"detached {r} is never released"
            return f"detached {r} is used outside one release (line {users[0] + 1})"
        k = rel_users[0]
        rel = RELEASE_RE.match(code[k])
        fn, arg = rel.group("fn"), rel.group("arg").strip()
        if arg != r:
            return f"detached {r} released through an expression: {lines[k].strip()}"
        earlier = [j for j in releases if j < k and j > decl]
        if earlier:
            return f"another release between the detach of {x} and its release"
        indent = rel.group("indent")
        if kind == "pointer" and fn in POINTER_RELEASES:
            new = [f"{indent}Py_CLEAR({x});"]
        elif kind == "value" and fn in ("PyValue_XCLOSE", "PyValue_CLEAR"):
            new = [f"{indent}PyValue_CLEAR({x});"]
        elif kind == "value" and fn == "PyValue_CLOSE":
            if not re.fullmatch(IDENT, x):
                return f"PyValue_CLOSE of a detached non-local {x}"
            new = [f"{indent}PyValue_CLOSE({x});", f"{indent}{x} = PyValue_NULL;"]
        else:
            return f"detach of a {kind} released by {fn}"
        replacement[decl] = []
        replacement[null_line] = []
        replacement[k] = new
    for k in range(start, restore + 1):
        if k in replacement:
            continue
        if k == save or k == restore:
            replacement[k] = []
        else:
            replacement[k] = [lines[k]]
    return Site(save, restore, start, var, declared), replacement


def unwrap_blocks(src: Source, edits: Edits, touched: set[int]) -> None:
    """Rule 3: a bare block left with one statement loses its braces."""
    lines, code = src.lines, src.code
    seen = set()
    for t in sorted(touched):
        opener = src.block_start(t)
        if opener in seen or src.depth[opener] >= src.depth[t]:
            continue
        seen.add(opener)
        if code[opener].strip() != "{" or opener in edits.replace:
            continue
        closer = src.block_end(t)
        if code[closer].strip() != "}" or closer in edits.replace:
            continue
        prev = opener - 1
        while prev >= 0 and is_comment_or_blank(code[prev]):
            prev -= 1
        prev_code = code[prev].rstrip()
        if prev_code.lstrip().startswith("#") or not prev_code.endswith((";", "{", "}", ":")):
            continue
        body: list[str] = []
        for k in range(opener + 1, closer):
            body.extend(edits.replace.get(k, [lines[k]]))
        statements = [b for b in body if b.strip() and not is_comment_or_blank(code_only([b])[0])]
        if len(statements) != 1 or not statements[0].rstrip().endswith(";"):
            continue
        if DETACH_DECL_RE.match(statements[0]):
            continue
        edits.delete(opener)
        edits.delete(closer)
        for k in range(opener + 1, closer):
            new = edits.replace.get(k, [lines[k]])
            edits.replace[k] = [b[4:] if b.startswith("    ") else b for b in new]


# ---------------------------------------------------------------------------
# Rule 5: release helpers


@dataclass
class Helper:
    name: str
    start: int  # the line with the return type
    end: int  # the closing brace
    params: list[str]
    release: str  # the release in the body, with {0} for the released argument
    deref: bool  # the body releases *param: a detach through the pointer
    status: str | None  # None, "pending" or "passthrough"


HELPER_HEAD_RE = re.compile(r"^(?P<name>[A-Za-z_]\w*)\((?P<params>[^()]*)\)\s*$")
PARAM_RE = re.compile(r"^(?:const\s+)?(?P<type>PyValue|PyObject|[A-Za-z_]\w*)\s*(?P<stars>\**)\s*(?P<name>[A-Za-z_]\w*)$")

# Status helpers whose bodies were read and mapped by hand, as rule 5 asks:
# the body, normalised, and the release it reduces to.  The body must match
# exactly; anything else is refused.
STATUS_BODIES = {
    # setobject.c: the release is the only failure source, and it cannot fail
    # under the release contract.  The body releases only a refcounted key,
    # which is PyValue_XCLOSE's test.
    "set_close_key": (
        "if(!PyValue_RefcountOnObject(key)){returnstatus;}"
        "if(Py_REFCNT(PyValue_AsKnownObjectBorrow(key))>1){PyValue_CLOSE(key);returnstatus;}"
        "PyObject*prior=PyErr_GetRaisedException();PyValue_CLOSE(key);"
        "intfailed=PyErr_Occurred()!=NULL;_PyValue_RestoreException(prior);"
        "returnfailed?-1:status;",
        "PyValue_XCLOSE({0})"),
    # setobject.c: set_close_key of the stolen object, or nothing for NULL.
    "set_close_object": (
        "returnobj==NULL?status:set_close_key(PyValue_FromPyObjectSteal(obj),status);",
        "Py_XDECREF({0})"),
}


def find_helpers(src: Source) -> dict[str, Helper]:
    helpers = {}
    lines, code = src.lines, src.code
    for i in range(1, len(lines) - 1):
        if src.depth[i] != 0:
            continue
        m = HELPER_HEAD_RE.match(code[i])
        if m is None or not code[i - 1].lstrip().startswith("static") or code[i + 1].strip() != "{":
            continue
        end = next((k for k in range(i + 2, len(lines)) if src.depth[k] == 1 and code[k].strip() == "}"), None)
        if end is None:
            continue
        body = [k for k in range(i + 2, end) if code[k].strip()]
        if len(body) > 20:
            continue
        params = [p.strip() for p in m.group("params").split(",")]
        parsed = [PARAM_RE.match(p) for p in params]
        if not all(parsed):
            continue
        helper = match_helper(src, m.group("name"), code[i - 1].strip(), parsed, body, i - 1, end)
        if helper is not None:
            helpers[helper.name] = helper
    return helpers


def match_helper(src: Source, name: str, ret: str, params, body: list[int],
                 start: int, end: int) -> Helper | None:
    code = src.code
    stmts = [code[k].strip() for k in body]
    names = [p.group("name") for p in params]
    ret_type = ret.split()[-1]
    if name in STATUS_BODIES:
        shape, release = STATUS_BODIES[name]
        if "".join(norm(s) for s in stmts) == shape and len(params) == 2:
            return Helper(name, start, end, names, release, False, "passthrough")
        return None
    if len(params) != 1:
        return None
    param = names[0]

    def release_of(stmt: str) -> tuple[str, bool] | None:
        m = RELEASE_RE.match(stmt)
        if m is None or m.group("fn") not in RELEASES:
            return None
        arg = norm(m.group("arg"))
        if arg == param:
            return m.group("fn") + "({0})", False
        if arg == "*" + param:
            return m.group("fn") + "({0})", True
        return None

    if ret_type == "void" and len(stmts) == 1:
        r = release_of(stmts[0])
        if r is not None:
            return Helper(name, start, end, names, r[0], r[1], None)
    # A save, the release, then a restore only when something was saved.
    if ret_type == "void" and len(stmts) == 5:
        m = re.match(r"^PyObject\s*\*\s*(" + IDENT + r")\s*=\s*PyErr_GetRaisedException\(\);$", stmts[0])
        r = release_of(stmts[1])
        if (m and r and norm(stmts[2]) == f"if({m.group(1)}!=NULL){{"
                and norm(stmts[3]) == f"_PyValue_RestoreException({m.group(1)});" and stmts[4] == "}"):
            return Helper(name, start, end, names, r[0], r[1], None)
    if ret_type == "int" and len(stmts) == 2 and norm(stmts[1]) == "returnPyErr_Occurred()?-1:0;":
        r = release_of(stmts[0])
        if r is not None:
            return Helper(name, start, end, names, r[0], r[1], "pending")
    return None


def helper_argument(helper: Helper, arg: str) -> str | None:
    """The released expression for a call's argument."""
    arg = arg.strip()
    if not helper.deref:
        return arg
    m = re.fullmatch(r"(?:\(\s*PyObject\s*\*\*\s*\)\s*)?&\s*(" + LVALUE + r")", arg)
    if m:
        return m.group(1)
    if re.fullmatch(LVALUE, arg):
        return "*" + arg
    return None


def split_args(text: str) -> list[str] | None:
    args, level, cur = [], 0, ""
    for c in text:
        if c in "([{":
            level += 1
        elif c in ")]}":
            level -= 1
        if c == "," and level == 0:
            args.append(cur)
            cur = ""
        else:
            cur += c
    args.append(cur)
    return [a.strip() for a in args] if level == 0 else None


def helper_sites(src: Source, refusals: list[Refusal], edits: Edits) -> int:
    """Rule 5.  Return the number of calls rewritten."""
    lines, code = src.lines, src.code
    helpers = find_helpers(src)
    count = 0
    removed: set[str] = set()
    changed = True
    while changed:
        changed = False
        for helper in helpers.values():
            if helper.name in removed:
                continue
            dead = set()
            for other in removed:
                dead.update(range(helpers[other].start, helpers[other].end + 1))
            call_re = re.compile(r"\b" + re.escape(helper.name) + r"\s*\(")
            calls = [k for k, c in enumerate(code)
                     if call_re.search(c) and k not in dead
                     and not (helper.start <= k <= helper.end)]
            pending: dict[int, list[str]] = {}
            refused: list[Refusal] = []
            for k in calls:
                new = rewrite_call(src, helper, k)
                if isinstance(new, str):
                    refused.append(Refusal(k, "H", f"{helper.name}: {new}"))
                else:
                    pending[k] = new
            # Forward declarations are calls without a body.
            decls = [k for k in refused if re.match(r"^\s*static\b.*\b" + re.escape(helper.name) + r"\s*\(.*\);\s*$", lines[k.line])]
            refused = [r for r in refused if r not in decls]
            new_calls = {k: v for k, v in pending.items() if k not in edits.replace}
            for k, v in new_calls.items():
                edits.replace[k] = v
            count += len(new_calls)
            if refused:
                continue
            users = [inc for inc in included_files(src) if call_re.search(inc[1])]
            if users:
                refusals.append(Refusal(helper.start, "H", f"{helper.name}: also called in "
                                        f"{users[0][0]}; the helper stays"))
                continue
            for r in decls:
                edits.delete(r.line)
            for k in range(helper.start, helper.end + 1):
                edits.delete(k)
            # A comment block directly above the helper describes it.
            k = helper.start - 1
            if k >= 0 and lines[k].rstrip().endswith("*/"):
                first = k
                while first >= 0 and "/*" not in lines[first]:
                    first -= 1
                if first >= 0 and lines[first].lstrip().startswith("/*"):
                    for c in range(first, k + 1):
                        edits.delete(c)
            if helper.end + 1 < len(lines) and not lines[helper.end + 1].strip():
                edits.delete(helper.end + 1)
            removed.add(helper.name)
            changed = True
    # Calls of helpers that stay, for the step 7 list.
    dead = set()
    for other in removed:
        dead.update(range(helpers[other].start, helpers[other].end + 1))
    for helper in helpers.values():
        if helper.name in removed:
            continue
        call_re = re.compile(r"\b" + re.escape(helper.name) + r"\s*\(")
        for k, c in enumerate(code):
            if (call_re.search(c) and k not in dead and k not in edits.replace
                    and not (helper.start <= k <= helper.end)
                    and not re.match(r"^\s*static\b", lines[k])):
                new = rewrite_call(src, helper, k)
                reason = new if isinstance(new, str) else "not rewritten"
                refusals.append(Refusal(k, "H", f"{helper.name}: {reason}; the helper stays"))
    return count


def included_files(src: Source) -> list[tuple[str, str]]:
    """The text of the local files that src includes (generated cases,
    ceval_macros.h): they share its static helpers."""
    result = []
    base = os.path.dirname(src.full_path)
    for line in src.lines:
        m = re.match(r'^\s*#\s*include\s+"([^"]+)"', line)
        if m and os.path.exists(os.path.join(base, m.group(1))):
            with open(os.path.join(base, m.group(1)), encoding="utf-8") as f:
                result.append((m.group(1), "\n".join(code_only(f.read().split("\n")))))
    return result


def rewrite_call(src: Source, helper: Helper, k: int) -> list[str] | str:
    line, code = src.lines[k], src.code[k]
    if line.rstrip().endswith("\\"):
        return "call inside a macro definition"
    name = re.escape(helper.name)
    m = re.match(r"^(?P<indent>\s*)(?P<prefix>return\s+|\(void\)\s*|(?P<target>" + LVALUE +
                 r")\s*=\s*)?" + name + r"\((?P<args>.*)\);\s*$", code)
    if m is None or not balanced(m.group("args")):
        return "call is not a statement of its own"
    args = split_args(m.group("args"))
    if args is None or len(args) != len(helper.params):
        return "cannot split the arguments"
    indent = m.group("indent")
    released = helper_argument(helper, args[0])
    if released is None:
        return f"argument {args[0]} is not an lvalue or its address"
    release = indent + helper.release.format(released) + ";"
    # code blanks comments; keep a trailing comment of the call on the release.
    trailing = line[len(code.rstrip()):].strip()
    if trailing:
        release += " " + trailing
    prefix = (m.group("prefix") or "").strip()
    if helper.status is None:
        if prefix:
            return "result of a void helper used"
        return [release]
    if helper.status == "pending":
        if prefix and prefix != "(void)":
            return "status used (PyErr_Occurred() after the release)"
        return [release]
    # passthrough: the helper returns its status argument.
    status = args[1]
    if not re.fullmatch(r"-?\d+|" + IDENT, status):
        return f"status argument {status} is not a name or a constant"
    if not prefix or prefix == "(void)":
        return [release]
    if prefix.startswith("return"):
        return [release, f"{indent}return {status};"]
    target = m.group("target")
    if target == status:
        return [release]
    return [release, f"{indent}{target} = {status};"]


# ---------------------------------------------------------------------------
# Rule 6: class P


P_TERM_RE = re.compile(r"\s*\|\|\s*PyErr_Occurred\(\)")
STANDALONE_RE = re.compile(r"^\s*(?:\}\s*else\s+)?if\s*\(\s*PyErr_Occurred\(\)\s*\)")
CALL = IDENT + r"\s*\(.*\)"
FAILURE_TERMS = [
    # (pattern of the left term, what it tests)
    (re.compile(r"^(?P<x>" + LVALUE + r")\s*==\s*NULL$"), "null"),
    (re.compile(r"^(?P<x>" + LVALUE + r")\s*<\s*0$"), "negative"),
    (re.compile(r"^(?P<x>" + LVALUE + r")\s*==\s*-1$"), "minus-one"),
    (re.compile(r"^!\s*(?P<x>" + LVALUE + r")$"), "false"),
]
IN_CONDITION_TERMS = [
    re.compile(r"^(?P<call>" + CALL + r")\s*(?:==\s*NULL|<\s*0|==\s*-1)$"),
    re.compile(r"^!\s*(?P<call>" + CALL + r")$"),
]


def top_level(text: str, tokens: tuple[str, ...]) -> bool:
    """Whether text has one of tokens outside parentheses."""
    level = 0
    i = 0
    while i < len(text):
        c = text[i]
        if c in "([":
            level += 1
        elif c in ")]":
            level -= 1
        elif level == 0 and any(text.startswith(t, i) for t in tokens):
            return True
        i += 1
    return False


def condition_of(code_line: str) -> tuple[int, int] | None:
    """The span of the condition of an if statement that starts the line."""
    m = re.match(r"^\s*(?:\}\s*else\s+)?if\s*\(", code_line)
    if m is None:
        return None
    level = 1
    for i in range(m.end(), len(code_line)):
        if code_line[i] == "(":
            level += 1
        elif code_line[i] == ")":
            level -= 1
            if level == 0:
                return m.end(), i
    return None


def previous_statement(src: Source, k: int) -> tuple[int, str] | None:
    """The statement before line k in the same block, joined."""
    code = src.code
    j = k - 1
    while j >= 0 and not code[j].strip():
        j -= 1
    if j < 0 or not code[j].rstrip().endswith(";") or src.depth[j] != src.depth[k]:
        return None
    end = j
    while j > 0:
        prev = code[j - 1].rstrip()
        if not prev.strip() or prev.endswith((";", "{", "}", ":")) or prev.lstrip().startswith("#"):
            break
        j -= 1
    if src.depth[j] != src.depth[k]:
        return None
    return j, " ".join(code[i].strip() for i in range(j, end + 1))


def assigned_call(statement: str, x: str) -> str | None:
    """The function called if statement is ``x = f(...);`` or a declaration."""
    target = r"\s*".join(re.escape(part) for part in re.findall(r"\w+|\S", x))
    m = re.match(r"^(?:(?:const\s+)?" + IDENT + r"(?:\s+" + IDENT + r")*(?:\s*\*+\s*|\s+))?" +
                 target + r"\s*=\s*(?:\(\s*" + IDENT + r"\s*\**\s*\)\s*)?(?P<fn>" +
                 IDENT + r")\s*\((?P<args>.*)\)\s*;$", statement)
    if m is None or not balanced(m.group("args")):
        return None
    if re.search(r"(?<![=!<>])=(?!=)", m.group("args")):
        return None
    return m.group("fn")


JUMP_RE = re.compile(r"^(?:goto\s+\w+|return\b[^;]*|break|continue)\s*;$")
CHECKED_JUMP_RE = re.compile(r"^(?:\}\s*else\s+)?if\s*\((?P<cond>.*)\)\s*"
                             r"(?:goto\s+\w+|return\b[^;]*|break|continue)\s*;$")
# Calls that cannot raise: releases (the release contract), new references,
# borrows, size and type macros.
QUIET_CALLS = set(RELEASES) | {"Py_INCREF", "Py_XINCREF", "assert", "strlen", "memcpy", "memset"}
QUIET_CALL_RE = re.compile(
    r"^(?:Py_X?NewRef|Py_SIZE|Py_TYPE|Py_IS_TYPE|Py_SET_SIZE|Py_REFCNT|_Py_ID|_PyObject_CAST|"
    r"_PyType_CAST|_PyType_Name|Py\w*_GET_SIZE|Py\w*_GET_ITEM|Py\w*_SET_ITEM|Py\w*_GET_LENGTH|"
    r"Py\w*_GET_VALUE|Py\w*_SET_VALUE|Py_ARRAY_LENGTH|\w+_CAST|_PyInterpreterState_GET|"
    r"_PyThreadState_GET|PyVectorcall_NARGS|_PyType_IsReady|_PyObject_IsUniquelyReferenced|"
    r"PyErr_Occurred|"
    r"_PyTuple_GET_\w+|_?Py\w+_Check(?:Exact)?|PyValue_(?:MakeHeapSafe|FromObjectBorrow|"
    r"FromPyObjectSteal|AsKnownObjectBorrow|AsKnownObjectSteal|Is\w+|DUP|TYPE))$")
CLEARING_RE = re.compile(r"^(?:[\w\s\*]*=\s*)?(?:PyErr_GetRaisedException|PyErr_Clear)\(\);$")


def quiet(text: str) -> bool:
    """Whether every call in text is one that cannot raise."""
    if re.match(r"^assert\s*\(", text):
        return True
    for m in re.finditer(r"\b(\w+)\s*\(", text):
        name = m.group(1)
        if name in ("if", "while", "for", "switch", "return", "sizeof"):
            continue
        if name not in QUIET_CALLS and not QUIET_CALL_RE.match(name):
            return False
    return True


def statement_start(src: Source, k: int) -> int:
    j = k
    while j > 0:
        prev = src.code[j - 1].rstrip()
        if not prev.strip() or prev.endswith((";", "{", "}", ":")) or prev.lstrip().startswith("#"):
            break
        j -= 1
    return j


def if_head(src: Source, k: int) -> int | None:
    """The first line of the if statement whose chain contains the block
    opened on line k, or None if that block is not an if or else branch."""
    while True:
        start = statement_start(src, k)
        header = " ".join(src.code[i].strip() for i in range(start, k + 1))
        if re.match(r"^if\b", header):
            return start
        if re.match(r"^\}\s*else\b", header):
            k = src.block_start(start)
            continue
        if re.match(r"^else\b", header):
            j = start - 1
            while j >= 0 and not src.code[j].strip():
                j -= 1
            if j < 0 or src.code[j].strip() != "}":
                return None
            k = src.block_start(j)
            continue
        return None


def earlier_failure(src: Source, j: int) -> str | None:
    """Why an exception raised before line j may still be pending at j, or
    None.  `|| PyErr_Occurred()` also tests such an exception (the end of a
    PyIter_Next loop, an unchecked call), so it is load-bearing there.  Walk
    back over statements that cannot raise to the decisive line: the start of
    a function or loop body, a jump, a clear, or an if block that ends in a
    jump.  Branch conditions on the way must not raise."""
    code = src.code
    k = j - 1
    while True:
        while k >= 0 and (not code[k].strip() or code[k].lstrip().startswith("#")):
            k -= 1
        if k < 0:
            return "no statement before the call"
        line = code[k].strip()
        if line.endswith("{"):
            if line == "{" and src.depth[k] == 0:
                return None
            start = statement_start(src, k)
            header = " ".join(code[i].strip() for i in range(start, k + 1))
            if re.match(r"^(?:for|while|do)\b", header):
                return None
            head = if_head(src, k)
            if head is None:
                return f"first statement of a block opened by '{header[:40]}'"
            cond = " ".join(code[i].strip() for i in range(head, statement_start(src, k) + 1))
            if not quiet(cond.split("{")[0]):
                return f"inside a branch whose condition can raise: '{cond[:40]}'"
            k = head - 1
            continue
        if line.endswith(":"):
            return "first statement after a label"
        if line.startswith("}"):
            opener = src.block_start(k)
            start = statement_start(src, opener)
            header = " ".join(code[i].strip() for i in range(start, opener + 1))
            if line != "}" or not re.match(r"^(?:\}\s*else\s+)?if\b", header):
                return f"after the end of a block opened by '{header[:40]}'"
            b = k - 1
            while b > opener and not code[b].strip():
                b -= 1
            if not JUMP_RE.match(code[b].strip()):
                return "after an if block that falls through"
            return None
        prev = previous_statement(src, k + 1)
        if prev is None:
            return f"after '{line[:40]}'"
        first, stmt = prev
        if JUMP_RE.match(stmt) or CLEARING_RE.match(stmt):
            return None
        m = CHECKED_JUMP_RE.match(stmt)
        if m and balanced(m.group("cond")):
            return None
        if quiet(stmt):
            k = first - 1
            continue
        return f"after the unchecked statement '{stmt[:50]}'"


def class_p_sites(src: Source, refusals: list[Refusal], edits: Edits) -> int:
    """Rule 6.  Return the number of terms dropped and branches deleted."""
    lines, code = src.lines, src.code
    count = 0
    for k, text in enumerate(code):
        if STANDALONE_RE.match(text):
            refusals.append(Refusal(k, "P", "standalone PyErr_Occurred() test (step 7)"))
            continue
        if not P_TERM_RE.search(text):
            continue
        if lines[k].rstrip().endswith("\\"):
            refusals.append(Refusal(k, "P", "inside a macro definition"))
            continue
        span = condition_of(text)
        if span is None:
            refusals.append(Refusal(k, "P", "not the condition of an if statement that fits on its line"))
            continue
        cond = text[span[0]:span[1]].strip()
        m = re.fullmatch(r"(?P<left>.*?)\s*\|\|\s*PyErr_Occurred\(\)", cond)
        if m is None or top_level(m.group("left"), ("||", "&&", "?")):
            refusals.append(Refusal(k, "P", f"other condition: {cond}"))
            continue
        left = m.group("left").strip()
        dead_branch = False
        if any(p.match(left) and balanced(p.match(left).group("call")) for p in IN_CONDITION_TERMS):
            why = earlier_failure(src, k)
            if why is not None:
                refusals.append(Refusal(k, "P", f"an earlier failure may be pending: {why}"))
                continue
        else:
            term = next(((p.match(left), kind) for p, kind in FAILURE_TERMS if p.match(left)), None)
            if term is None:
                refusals.append(Refusal(k, "P", f"other left term: {left}"))
                continue
            x = term[0].group("x")
            prev = previous_statement(src, k)
            fn = assigned_call(prev[1], x) if prev else None
            if fn is None:
                refusals.append(Refusal(k, "P", f"left term {left} does not test the previous statement's call"))
                continue
            why = earlier_failure(src, prev[0])
            if why is not None:
                refusals.append(Refusal(k, "P", f"an earlier failure may be pending: {why}"))
                continue
            if fn == "Py_NewRef" and term[1] == "null":
                dead_branch = True
        if not dead_branch:
            new = lines[k][:span[0]] + P_TERM_RE.sub("", lines[k][span[0]:span[1]], count=1) + lines[k][span[1]:]
            if code_only([new])[0].count("(") != code_only([new])[0].count(")"):
                refusals.append(Refusal(k, "P", "rewrite would unbalance the line"))
                continue
            edits.replace[k] = [new]
            count += 1
            continue
        # Py_NewRef never returns NULL: the branch is dead.
        if text.rstrip().endswith("{"):
            end = src.block_end(k + 1) if k + 1 < len(lines) else None
            nxt = end + 1
            while nxt < len(lines) and not code[nxt].strip():
                nxt += 1
            if (end is None or code[end].strip() != "}" or
                    (nxt < len(lines) and code[nxt].lstrip().startswith("else"))):
                refusals.append(Refusal(k, "P", "dead branch after Py_NewRef has an else"))
                continue
            for i in range(k, end + 1):
                edits.delete(i)
        elif text.rstrip().endswith(";") and not code[k + 1].lstrip().startswith("else"):
            edits.delete(k)
        else:
            refusals.append(Refusal(k, "P", "dead branch after Py_NewRef is not a simple statement"))
            continue
        count += 1
    return count


# ---------------------------------------------------------------------------


def process(path: str, root: str, rules: str, check: bool) -> tuple[int, list[str], str | None]:
    rel = os.path.relpath(path, root)
    with open(path, encoding="utf-8") as f:
        text = f.read()
    src = Source(rel, text)
    src.full_path = path
    refusals: list[Refusal] = []
    edits = Edits()
    if rules == "direct":
        count = direct_sites(src, refusals, edits)
    elif rules == "helpers":
        count = helper_sites(src, refusals, edits)
    elif rules == "class-p":
        count = class_p_sites(src, refusals, edits)
    else:
        raise SystemExit(f"unknown rules {rules}")
    new_lines = edits.apply(src.lines)
    new_text = "\n".join(new_lines)
    report = [f"{rel}:{r.line + 1}: class {r.cls}: {r.reason}"
              for r in sorted(refusals, key=lambda r: r.line)]
    diff = None
    if new_text != text:
        diff = "".join(difflib.unified_diff(
            text.splitlines(True), new_text.splitlines(True),
            fromfile="a/" + rel, tofile="b/" + rel))
        if not check:
            if generated(rel):
                report.append(f"{rel}:1: class -: generated file, not written; change its generator")
            else:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(new_text)
    return count, report, diff


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("files", nargs="+")
    parser.add_argument("--rules", choices=("direct", "helpers", "class-p"),
                        default="direct")
    parser.add_argument("--check", action="store_true",
                        help="change no file; print the diff instead and exit 1 "
                             "if any file would change")
    parser.add_argument("--root", default=os.getcwd(),
                        help="paths in the report are relative to this")
    args = parser.parse_args(argv)
    total = 0
    would_change = False
    for path in args.files:
        if not path.endswith((".c", ".h")):
            print(f"{path}:1: class -: not a C file, not processed")
            continue
        count, report, diff = process(path, args.root, args.rules, args.check)
        total += count
        for line in report:
            print(line)
        if args.check and diff:
            sys.stdout.write(diff)
            would_change = True
    print(f"# {total} sites rewritten" + (" (check only)" if args.check else ""),
          file=sys.stderr)
    # Like a linter: with --check, exit 1 if any file would change.
    return 1 if would_change else 0


if __name__ == "__main__":
    sys.exit(main())
