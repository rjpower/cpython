"""Dynamic instruction counts of one kernel, for register-ir.md §2.4.

Run this file under a build interpreter, not under uv:

    BUILD/python -s -W ignore dynamic_counts.py KERNEL.py [METADATA.h]

The kernel module is loaded; setup(None) runs if it exists; run(1) runs once
to warm up (compiling, specialising and seeding the executing copies) and
once more between sys.monitoring set_events(INSTRUCTION) and
set_events(0).  Only events between those two calls count: the second
run(1), plus about ten instructions of this script around that call.  Each event is named with this build's dis.
NOT_TAKEN is counted separately and excluded from the total; upstream's
EXTENDED_ARG is included, since it is a dispatch there.  Prints one JSON
object.

Counting INSTRUCTION events replaces each handler with its instrumented
twin, so the counts are words executed, not the uninstrumented dispatch
path: the skipped RESUME and the JIT admission handlers are not seen, and
monitoring reports a specialised instruction under its generic name.

Under the register-IR build, METADATA.h (pycore_opcode_metadata.h of the
same commit; run.py extracts it) gives window extents, and the script also
estimates slot traffic per executed word:
- reads: register inputs, plus the slot named in the immediate of
  LOAD_FAST*, LOAD_DEREF, STORE_DEREF, MAKE_CELL and LOAD_FAST_AND_CLEAR;
  for a window word, the number of values its stack instruction pops;
- writes: dst, or the slot in the immediate of STORE_FAST, DELETE_FAST and
  LOAD_FAST_AND_CLEAR; for a window word, the number of values pushed;
- null stores: register inputs that name a temporary, and every popped
  window value; borrowed inputs that are not consumed are counted too,
  because the format tuple does not export borrow_mask, so this is an
  upper bound.  Window extents come from the stack effect, so values that
  stay in place (FOR_ITER's iterator) count as read and written.
"""

import collections, functools, importlib.util, json, re, sys

IMM_READS = {"LOAD_FAST", "LOAD_FAST_BORROW", "LOAD_FAST_CHECK", "LOAD_FAST_AND_CLEAR",
             "LOAD_DEREF", "STORE_DEREF", "MAKE_CELL", "DELETE_DEREF"}
IMM_WRITES = {"STORE_FAST", "DELETE_FAST", "LOAD_FAST_AND_CLEAR", "MAKE_CELL"}
CONSTS = ("LOAD_CONST", "LOAD_SMALL_INT", "LOAD_COMMON_CONSTANT")


def stack_effects(path):
    text = open(path).read()
    out = []
    for fn in ("_PyOpcode_num_popped", "_PyOpcode_num_pushed"):
        i = text.index("int %s(int opcode, int oparg)" % fn)
        body = text[i:text.index("default:", i)]
        out.append({n: e for n, e in re.findall(r"case (\w+):\s*return (.*?);", body)})
    return out


def main(kernel, metadata=None):
    import dis
    spec = importlib.util.spec_from_file_location("_kernel", kernel)
    mod = importlib.util.module_from_spec(spec)
    sys.modules["_kernel"] = mod
    spec.loader.exec_module(mod)
    if hasattr(mod, "setup"):
        mod.setup(None)
    mod.run(1)
    counts = collections.Counter()
    M = sys.monitoring
    M.use_tool_id(3, "ir-count")
    M.register_callback(3, M.events.INSTRUCTION,
                        lambda code, off: counts.__setitem__((code, off), counts[code, off] + 1))
    M.set_events(3, M.events.INSTRUCTION)
    mod.run(1)
    M.set_events(3, 0)

    ir = hasattr(mod.run.__code__, "co_fbinit")
    if ir:
        import opcode
        popped, pushed = stack_effects(metadata)
    c = collections.Counter()
    names = collections.Counter()
    cache = {}
    for (code, off), n in counts.items():
        if code not in cache:
            ins = [i for i in dis.get_instructions(code) if i.opname != "CACHE"]
            cache[code] = ({i.offset: k for k, i in enumerate(ins)}, ins)
        index, ins = cache[code]
        k = index[off]
        i = ins[k]
        if i.opname == "NOT_TAKEN":
            c["not_taken"] += n
            continue
        names[i.opname] += n
        c["total"] += n
        nxt = ins[k + 1].opname if k + 1 < len(ins) else ""
        if i.opname in ("COMPARE_OP", "CONTAINS_OP", "IS_OP") and nxt.startswith("POP_JUMP"):
            c["fusable_compare_branch"] += n
        if i.opname in ("CALL", "CALL_KW") and nxt == "POP_TOP":
            c["call_pop_top"] += n
        if i.opname in CONSTS:
            c["const_loads"] += n
            v = i.argval
            if i.opname == "LOAD_SMALL_INT" or v is None or isinstance(v, (bool, int, float)):
                c["const_scalar"] += n
        if ir:
            traffic(code, i, n, c, popped, pushed, opcode)
    out = {"kernel": kernel, "ir": ir, "counts": dict(c), "top": names.most_common(15)}
    for key in ("PUSH_NULL", "STORE_FAST", "STORE_FAST_STORE_FAST", "UNPACK_SEQUENCE", "COPY"):
        out["counts"]["op_" + key] = names[key]
    out["counts"]["load_fast_any"] = sum(v for k, v in names.items() if k.startswith("LOAD_FAST"))
    print(json.dumps(out, indent=1, sort_keys=True))


@functools.cache
def ir_formats():
    """(flags, roles, imm, fb, base) per handler, from the instruction
    contract (register IR v2 B0a)"""
    import _opcode
    return [None if r is None else (r.flags, r.roles, r.imm, r.fb, r.base)
            for r in _opcode.get_ir_contract()]


def traffic(code, i, n, c, popped, pushed, opcode):
    args = set(code.co_varnames)
    nl = (len(code.co_varnames) + sum(x not in args for x in code.co_cellvars)
          + len(code.co_freevars))
    import dis
    flags, roles, imm, fbloc, base = ir_formats()[dis._all_opmap[i.handler]]
    if flags & 2:   # _PyIR_WINDOW
        name = opcode.opname[base]
        oparg = i.arg or 0
        r = eval(popped[name], {"oparg": oparg})
        w = eval(pushed[name], {"oparg": oparg})
        c["slot_reads"] += n * r
        c["slot_writes"] += n * w
        c["null_stores"] += n * r
        return
    # LOAD_FAST* that write a temporary: under pyvalue D4 (no borrow in a home)
    # the _BORROW forms take a count for a mortal object (register-ir.md G2).
    if i.opname.startswith("LOAD_FAST") and i.dst is not None and i.dst >= nl:
        c["load_fast_to_temp"] += n
        c["load_fast_borrow_to_temp"] += n * (i.opname == "LOAD_FAST_BORROW")
    r = len(i.srcs) + (i.opname in IMM_READS)
    w = (i.dst is not None) + (i.opname in IMM_WRITES)
    c["slot_reads"] += n * r
    c["slot_writes"] += n * w
    c["null_stores"] += n * sum(s >= nl for s in i.srcs)


if __name__ == "__main__":
    main(*sys.argv[1:])
