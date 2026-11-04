"""Static counts of compiled code, for pyjit/design/register-ir.md §2.3.

Run this file under a build interpreter, not under uv:

    BUILD/python -s -W ignore static_counts.py [--time-limit S] PATH...

Each PATH is a directory (every .py below it), a .py file, or @LIST (a file
with one path per line).  Every file is compiled with compile(..., "exec");
every code object reachable through co_consts is counted.  Prints one JSON
object.

Under the register-IR build (code objects have co_fbinit) it reports, per
code object, the distribution of slots, feedback entries, names, constants
and words; constant loads by kind; feedback entries by kind with their
memory at the uniform stride and at per-kind sizes; PUSH_NULL destinations;
and compile failures (a SystemError is the IR's limit error).  It needs the
build's own dis.py, opcode.py and _opcode_metadata.py on PYTHONPATH; run.py
arranges that.  Under upstream it reports code objects and instructions
(CACHE units excluded).

Limits: "slots" is nlocalsplus + co_stacksize, where nlocalsplus counts
varnames, cells that are not arguments, and free variables.  Constant kinds
come from the loaded object (LOAD_CONST, LOAD_COMMON_CONSTANT) or from the
handler (LOAD_SMALL_INT*, LOAD_CONST_INLINE_*).  The feedback kind is the
kind byte of each co_fbinit entry; entry 0 is shared and not counted.
"fb_sites" counts words that want an entry (an fb field, which is 0 past
the 4,095-entry limit, or a NOFB form), so it exceeds the entries only in
code objects past the limit.
"""

import collections, json, os, sys, time

IR = hasattr(compile("0", "s", "eval"), "co_fbinit")
if IR:
    import dis, opcode
    from dis import _unpack_words, _decode_word
    NAMES = dis._all_opname
    FORMATS = opcode._ir_formats
KINDS = ("COUNTER", "BRANCH", "UNARY", "BINARY", "ATTR", "GLOBAL", "CALL")
# pycore_feedback.h struct sizes.  BRANCH is 16 for POP_JUMP_IF_NONE and _NOT_NONE
# and 24 for POP_JUMP_IF_FALSE and _TRUE (a UNARY entry); the 8 extra bytes are
# added per word below, since the kind byte does not tell the two apart.
KIND_BYTES = (16, 16, 24, 32, 40, 32, 24)
BRANCH_TRUTH_EXTRA = 8
STRIDE = 40                                  # sizeof(_PyFbAttr)
SCALARS = (type(None), bool, int, float)


def files(paths):
    for p in paths:
        if p.startswith("@"):
            yield from (l.strip() for l in open(p[1:]) if l.strip())
        elif os.path.isdir(p):
            for dp, dn, fn in os.walk(p):
                dn.sort()
                yield from (os.path.join(dp, f) for f in sorted(fn) if f.endswith(".py"))
        else:
            yield p


def walk(co):
    yield co
    for c in co.co_consts:
        if hasattr(c, "co_code"):
            yield from walk(c)


def nlocalsplus(co):
    args = set(co.co_varnames)
    return len(co.co_varnames) + sum(c not in args for c in co.co_cellvars) + len(co.co_freevars)


def const_kind(name, arg, co):
    if name.startswith("LOAD_SMALL_INT") or name == "LOAD_CONST_INLINE_INT":
        return "scalar"
    if name == "LOAD_CONST_INLINE_FLOAT":
        return "scalar"
    if name == "LOAD_CONST":
        v = co.co_consts[arg]
    elif name == "LOAD_COMMON_CONSTANT":
        v = opcode._common_constants[arg]
    else:
        return None
    if isinstance(v, SCALARS):
        return "scalar"
    for t, k in ((str, "str"), (bytes, "bytes"), (tuple, "tuple"), (frozenset, "frozenset")):
        if isinstance(v, t):
            return k
    return "code" if hasattr(v, "co_code") else "other"


def summary(xs):
    xs = sorted(xs)
    n = len(xs)
    q = lambda f: xs[min(n - 1, int(n * f))]
    return {"max": xs[-1], "p50": q(.5), "p99": q(.99), "p99.9": q(.999), "p99.99": q(.9999),
            "over255": sum(x > 255 for x in xs), "over2047": sum(x > 2047 for x in xs),
            "over4095": sum(x > 4095 for x in xs)}


def main(argv):
    limit = float("inf")
    if argv[:1] == ["--time-limit"]:
        limit, argv = float(argv[1]), argv[2:]
    start = time.monotonic()
    c = collections.Counter()
    dist = collections.defaultdict(list)
    failures = collections.Counter()
    for path in files(argv):
        if time.monotonic() - start > limit:
            c["files_skipped_time"] += 1
            continue
        try:
            src = open(path, "rb").read()
            top = compile(src, path, "exec", dont_inherit=True)
        except SystemError as e:
            failures[str(e)[:120]] += 1
            continue
        except (SyntaxError, ValueError, RecursionError, MemoryError):
            c["files_syntax_error"] += 1
            continue
        c["files"] += 1
        for co in walk(top):
            c["code_objects"] += 1
            if not IR:
                c["instructions"] += sum(i.opname != "CACHE" for i in __import__("dis").get_instructions(co))
                continue
            measure(co, c, dist)
    out = {"ir": IR, "counts": dict(c), "limit_failures": dict(failures)}
    out["per_code_object"] = {k: summary(v) for k, v in dist.items() if v}
    print(json.dumps(out, indent=1, sort_keys=True))


def measure(co, c, dist):
    fbinit = co.co_fbinit
    nl = nlocalsplus(co)
    nfb = len(fbinit) // 4
    kinds = [int.from_bytes(fbinit[4 * i:4 * i + 4], sys.byteorder) >> 24 for i in range(1, nfb)]
    for k in kinds:
        c["fb_" + KINDS[k]] += 1
    packed = sum(KIND_BYTES[k] for k in kinds)
    c["fb_bytes_stride"] += STRIDE * len(kinds)
    c["fb_bytes_per_kind"] += packed
    dist["fb_entries"].append(len(kinds))
    dist["fb_packed_8byte_units"].append(packed // 8)
    dist["slots"].append(nl + co.co_stacksize)
    dist["names"].append(len(co.co_names))
    dist["consts"].append(len(co.co_consts))
    nwords = sites = cmp_sites = truth_branches = 0
    for off, h, fields in _unpack_words(co.co_code):
        if FORMATS[h] is None:
            continue
        nwords += 1
        name = NAMES[h]
        # a site that wants an entry: an fb field (0 past the limit) or a NOFB form
        sites += 3 in FORMATS[h][1] or name.endswith("_NOFB")
        arg, dst, srcs, fb = _decode_word(h, fields, fbinit)
        # compare and contains sites need an entry (G10): count them, with or without one
        cmp_sites += name.startswith(("COMPARE_OP", "CONTAINS_OP"))
        # truth-test branches have the 24-byte UNARY-layout entry
        truth_branches += fb != 0 and name.startswith(("POP_JUMP_IF_FALSE", "POP_JUMP_IF_TRUE"))
        k = const_kind(name, arg, co)
        if k is not None:
            c["const_" + k] += 1
            c["const_loads"] += 1
        if name == "PUSH_NULL":
            c["push_null_dst_" + ("local" if dst is not None and dst < nl else "temp")] += 1
    packed += BRANCH_TRUTH_EXTRA * truth_branches
    c["fb_bytes_per_kind"] += BRANCH_TRUTH_EXTRA * truth_branches
    dist["fb_packed_8byte_units"][-1] = packed // 8
    dist["compare_sites"].append(cmp_sites)
    c["words"] += nwords
    c["fb_sites"] += sites
    dist["words"].append(nwords)
    dist["fb_sites"].append(sites)


if __name__ == "__main__":
    main(sys.argv[1:])
