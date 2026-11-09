"""Per-type occupancy of inline slots, for the capacity rule (note 12 6.4).

Usage: python -I occupancy_types.py KERNEL.py LOOPS [VARIANT [EVERY]]

Runs the kernel under a profile hook that, every EVERY call events
(200,000 by default), samples every live object whose type has inline
slots. Prints one JSON row per type: type, whether the kernel defines it,
S (static attribute names over the MRO), capacity, basicsize, a histogram
of the slots its shape has passed (named and holes), and dictionary-mode
samples. capacity_rule.py sums block bytes over these rows."""
import gc, importlib.util, json, sys, collections
import _testinternalcapi
cap_of = _testinternalcapi.type_slot_capacity
shape_of = _testinternalcapi.shape

def load(path):
    spec = importlib.util.spec_from_file_location("kernel", path)
    m = importlib.util.module_from_spec(spec); sys.modules[spec.name] = m
    spec.loader.exec_module(m); return m

def static_names(tp):
    s = set()
    for c in tp.__mro__:
        s.update(c.__dict__.get("__static_attributes__", ()))
    return len(s)

class Sampler:
    def __init__(self, every):
        self.every, self.events = every, 0
        self.rows, self.caps = {}, {}
    def __call__(self, f, e, a):
        self.events += 1
        if self.events % self.every == 0:
            self.sample()
    def sample(self):
        sys.setprofile(None)
        for o in gc.get_objects():
            if o is self: continue
            tp = type(o)
            cap = self.caps.get(tp, -1)
            if cap == -1:
                cap = self.caps[tp] = cap_of(tp)
            if cap is None: continue
            _, index, slots = shape_of(o)
            r = self.rows.get(tp)
            if r is None:
                r = self.rows[tp] = {"hist": collections.Counter(), "dict_mode": 0}
            if index is None:
                r["dict_mode"] += 1; continue
            r["hist"][len(slots)] += 1   # slots the shape has passed (named + holes)
        sys.setprofile(self)

path, loops = sys.argv[1], int(sys.argv[2])
variant = sys.argv[3] if len(sys.argv) > 3 else None
k = load(path)
if hasattr(k, "setup"): k.setup(variant)
s = Sampler(int(sys.argv[4]) if len(sys.argv) > 4 else 200000)
sys.setprofile(s)
try:
    k.run(loops)
finally:
    sys.setprofile(None)
s.sample()
out = []
for tp, r in s.rows.items():
    out.append({"type": f"{tp.__module__}.{tp.__qualname__}", "kernel": tp.__module__ == "kernel",
                "S": static_names(tp), "cap": s.caps[tp], "basicsize": tp.__basicsize__,
                "hist": dict(r["hist"]), "dict_mode": r["dict_mode"]})
print(json.dumps(out))
