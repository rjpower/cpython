"""Shape-table, plan-cache and instance sizes (note 09 section 9, recorded).

Usage: BUILD/python -I shape_memory.py KERNEL.py [KERNEL.py ...]

For each kernel file of boil/bench/internal/kernels: run(1), then the bytes
of the shape tables (sys.getsizeof of each class the kernel defines, less
an empty class's) and of the plan cache (_testinternalcapi.type_cache_size)
over those classes.  Then sys.getsizeof of a four-attribute instance and
the RSS growth over 10,000 transient classes with an instance each.
"""
import gc
import importlib.util
import os
import sys

import _testinternalcapi


def load(path):
    spec = importlib.util.spec_from_file_location("kernel", path)
    module = importlib.util.module_from_spec(spec)
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
        shapes = sum(sys.getsizeof(c) - empty for c in classes)
        plans = sum(_testinternalcapi.type_cache_size(c) for c in classes)
        print(f"{os.path.basename(path)}: {len(classes)} classes, "
              f"shape-table bytes {shapes}, plan-cache bytes {plans}")
    print("getsizeof four-attribute instance", sys.getsizeof(Four()))
    gc.collect()
    before = rss_kib()
    for i in range(10_000):
        C = type(f"C{i}", (), {})
        o = C()
        o.x = 1
        o.y = 2
        o.x
        del C, o
    gc.collect()
    print("10,000 transient classes, RSS growth KiB", rss_kib() - before)


if __name__ == "__main__":
    main(sys.argv[1:])
