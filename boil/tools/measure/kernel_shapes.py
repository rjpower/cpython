"""Per-kernel shape counters and attribute sites (note 09 section 9).

Usage: BUILD/python -I kernel_shapes.py KERNEL.py LOOPS [VARIANT]

Runs the kernel's run(LOOPS), then prints the generalisations and shapes
re-versioned during the run (_testinternalcapi.shape_generalisations()),
and every LOAD_ATTR and STORE_ATTR site of the kernel's functions and
methods by its specialised form, so that a generic or megamorphic site
can be named.
"""
import importlib.util
import sys
import types
import dis

import _testinternalcapi


def load(path):
    spec = importlib.util.spec_from_file_location("kernel", path)
    module = importlib.util.module_from_spec(spec)
    # Registered, as driver.py does, so pickle and copy find its classes.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def sites(module):
    found = {}

    def walk(code, where):
        for ins in dis.get_instructions(code, adaptive=True):
            if ins.opname.startswith(("LOAD_ATTR", "STORE_ATTR")):
                found.setdefault(ins.opname, []).append(f"{where}:{ins.argval}")
        for const in code.co_consts:
            if isinstance(const, types.CodeType):
                walk(const, f"{where}.{const.co_name}")

    for name, obj in vars(module).items():
        if isinstance(obj, type) and obj.__module__ == "kernel":
            for attr, value in vars(obj).items():
                if isinstance(value, types.FunctionType):
                    walk(value.__code__, f"{name}.{attr}")
        elif isinstance(obj, types.FunctionType) and obj.__module__ == "kernel":
            walk(obj.__code__, name)
    return found


def main(path, loops, variant=None):
    kernel = load(path)
    if hasattr(kernel, "setup"):
        kernel.setup(variant)
    before = _testinternalcapi.shape_generalisations()
    kernel.run(loops)
    after = _testinternalcapi.shape_generalisations()
    print(f"generalisations {after[0] - before[0]}, "
          f"shapes re-versioned {after[1] - before[1]}")
    for opname, where in sorted(sites(kernel).items()):
        print(f"{opname:32} {len(where):3}  {', '.join(where)}")


if __name__ == "__main__":
    main(sys.argv[1], int(sys.argv[2]), *sys.argv[3:4])
