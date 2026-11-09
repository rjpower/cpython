"""Fixed-capacity occupancy of inline slots, by sampling (6c rung 5).

Usage: BUILD/python -I occupancy.py KERNEL.py LOOPS [VARIANT] [--every N]

Runs the kernel's setup(VARIANT), then run(LOOPS) under a profile hook that,
every N call events (default 200000), walks gc.get_objects() and reads the
shape of each object whose type has inline values.  The shape is read from
the header (_testinternalcapi.shape()), which does not materialise
__dict__.

The population is the objects live at a sample; the totals are the sums
over the samples.  Per object:
  allocated  the type's slot capacity (the object keeps its type's capacity
             across a __class__ change, after which it is in dictionary
             mode, so either way the current type's capacity is counted)
  named      slots with a name in the shape
  holes      slots the shape has passed but that hold no name (deletions)
An object in dictionary mode counts its capacity as allocated and nothing as
named.  The sampler itself is excluded; its rows are plain dicts, which
have no inline values.  The totals are reported for every object and for
the objects of the kernel's own classes.
"""
import gc
import importlib.util
import sys

import _testinternalcapi

FIELDS = ("objects", "allocated", "named", "holes", "dict_mode")
capacity_of = _testinternalcapi.type_slot_capacity
shape_of = _testinternalcapi.shape


def load(path):
    spec = importlib.util.spec_from_file_location("kernel", path)
    module = importlib.util.module_from_spec(spec)
    # Registered, as driver.py does, so pickle and copy find its classes.
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class Sampler:
    def __init__(self, every):
        self.every = every
        self.events = 0
        self.samples = 0
        self.by_type = {}
        self.capacity = {}

    def __call__(self, frame, event, arg):
        self.events += 1
        if self.events % self.every == 0:
            self.sample()

    def type_capacity(self, tp):
        try:
            return self.capacity[tp]
        except KeyError:
            c = self.capacity[tp] = capacity_of(tp)
            return c

    def sample(self):
        sys.setprofile(None)
        self.samples += 1
        for o in gc.get_objects():
            # The sampler is the script's only object with inline values.
            if o is self:
                continue
            cap = self.type_capacity(type(o))
            if cap is None:
                continue
            _, index, slots = shape_of(o)
            tp = type(o)
            row = self.by_type.get(tp)
            if row is None:
                row = self.by_type[tp] = dict.fromkeys(FIELDS, 0)
            row["objects"] += 1
            row["allocated"] += cap
            if index is None:
                row["dict_mode"] += 1
                continue
            named = sum(1 for s in slots if s is not None)
            row["named"] += named
            row["holes"] += len(slots) - named
        sys.setprofile(self)

    def report(self):
        print(f"samples {self.samples}, call events {self.events}")
        for label, keep in (("all", lambda tp: True),
                            ("kernel", lambda tp: tp.__module__ == "kernel")):
            t = dict.fromkeys(FIELDS, 0)
            for tp, row in self.by_type.items():
                if keep(tp):
                    for k in FIELDS:
                        t[k] += row[k]
            if not t["objects"]:
                print(f"{label}: no objects")
                continue
            unused = t["allocated"] - t["named"]
            print(f"{label}: objects {t['objects']}, "
                  f"allocated {t['allocated']}, named {t['named']}, "
                  f"holes {t['holes']}, dict_mode {t['dict_mode']}, "
                  f"unused {unused} ({100 * unused / t['allocated']:.1f}%)")
        worst = sorted(self.by_type.items(),
                       key=lambda kv: kv[1]["allocated"] - kv[1]["named"],
                       reverse=True)
        for tp, row in worst[:5]:
            name = f"{tp.__module__}.{tp.__qualname__}"
            print(f"  {name:40} objects {row['objects']:8} "
                  f"allocated {row['allocated']:9} named {row['named']:9} "
                  f"holes {row['holes']:6} dict_mode {row['dict_mode']:6}")


def main(argv):
    every = 200_000
    if "--every" in argv:
        i = argv.index("--every")
        every = int(argv[i + 1])
        del argv[i:i + 2]
    path, loops = argv[0], int(argv[1])
    variant = argv[2] if len(argv) > 2 else None
    kernel = load(path)
    setup = getattr(kernel, "setup", None)
    if setup is not None:
        setup(variant)
    sampler = Sampler(every)
    sys.setprofile(sampler)
    try:
        kernel.run(loops)
    finally:
        sys.setprofile(None)
    sampler.report()


if __name__ == "__main__":
    main(sys.argv[1:])
