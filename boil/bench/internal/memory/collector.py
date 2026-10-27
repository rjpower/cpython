"""The collector under a live heap: keep L MB live, allocate short-lived objects.

setup(L) builds the live heap; each loop then allocates 1 GB of short-lived
objects, so the time per loop is the seconds per allocated GB at that live
heap size.  The run uses L in {16, 128, 512}.

Sizes are nominal, fixed as object counts so every build runs the same
objects: a MB or GB here is 2**20 or 2**30 bytes as the baseline build
(upstream CPython at 321925c92c9) allocates them, measured with tracemalloc
including the list slot that holds each object.

Live heap: a list of (k, [k]) for k in range(N), N = L * 2**20 // 168.  The
baseline allocates 168 bytes per unit.  Each unit holds a list, so CPython
cannot untrack the tuple, and the collector sees every unit as a container
for as long as it lives.

Short-lived objects: batches of [(j, [j]) for j in range(1000)], each batch
alive until the next replaces it.  The baseline allocates 136,202 bytes per
batch (the ints below 256 are cached), so 1 GB is 7,884 batches.
"""

LIVE_UNIT_BYTES = 168
BATCH_BYTES = 136_202
BATCHES_PER_GB = round(2**30 / BATCH_BYTES)
live = None


def setup(live_mb):
    global live
    n = int(live_mb) * 2**20 // LIVE_UNIT_BYTES
    live = [(k, [k]) for k in range(n)]


def run(loops):
    for _ in range(loops):
        for _ in range(BATCHES_PER_GB):
            batch = [(j, [j]) for j in range(1000)]
    del batch
