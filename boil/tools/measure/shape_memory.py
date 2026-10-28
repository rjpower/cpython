"""Shape-table, plan-cache and instance sizes (note 09 section 9, recorded).

Usage: BUILD/python -I shape_memory.py [KERNEL.py ...]

For each kernel file of boil/bench/internal/kernels: run(1), then the bytes
of the shape tables (sys.getsizeof of each class with inline values that
the kernel defines, less an empty class's) and of the plan cache
(_testinternalcapi.type_cache_size) over all the classes it defines.

Without kernels, in a fresh process: sys.getsizeof of a four-attribute
instance and
current RSS after a collection over three batches of 10,000 transient
classes with an instance each, and how many of each batch are still alive.
"""
import gc
import importlib.util
import os
import sys
import weakref

import _testinternalcapi


def load(path):
    spec = importlib.util.spec_from_file_location("kernel", path)
    module = importlib.util.module_from_spec(spec)
    # Registered, as driver.py does, so pickle and copy find its classes.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def rss_kib():
    with open("/proc/self/statm") as f:
        return int(f.read().split()[1]) * os.sysconf("SC_PAGE_SIZE") // 1024


class Four:
    def __init__(self):
        self.a = 1
        self.b = 2
        self.c = 3
        self.d = 4


def main(paths):
    empty = sys.getsizeof(type("Empty", (), {}))
    for path in paths:
        kernel = load(path)
        if hasattr(kernel, "setup"):
            kernel.setup(None)
        kernel.run(1)
        classes = [v for v in vars(kernel).values()
                   if isinstance(v, type) and v.__module__ == "kernel"]
        # A class without inline values (__slots__, a builtin base) has no
        # shape table.
        shapes = sum(sys.getsizeof(c) - empty for c in classes
                     if _testinternalcapi.type_slot_capacity(c) is not None)
        plans = sum(_testinternalcapi.type_cache_size(c) for c in classes)
        print(f"{os.path.basename(path)}: {len(classes)} classes, "
              f"shape-table bytes {shapes}, plan-cache bytes {plans}")
    if paths:
        return
    print("getsizeof four-attribute instance", sys.getsizeof(Four()))
    # Transient classes in three batches: growth in the first batch can be
    # the allocator's one-time reservation, growth in every batch is
    # retention.  Each class is checked dead through a weak reference, so
    # a conservative root that keeps one alive is reported, not hidden.
    gc.collect()
    start = rss_kib()
    for batch in range(3):
        refs = []
        for i in range(10_000):
            C = type(f"C{i}", (), {})
            o = C()
            o.x = 1
            o.y = 2
            o.x
            refs.append(weakref.ref(C))
            del C, o
        gc.collect()
        alive = sum(r() is not None for r in refs)
        now = rss_kib()
        print(f"transient classes, batch {batch + 1} of 10,000: "
              f"RSS {now} KiB ({now - start:+} since the start), "
              f"{alive} still alive")


if __name__ == "__main__":
    main(sys.argv[1:])
