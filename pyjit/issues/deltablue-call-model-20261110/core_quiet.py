"""Core-local interference gate for benchmark runs pinned to one CPU.

The system load average counts runnable tasks on every CPU. On this 16-CPU host
it rejects runs while other sessions use unrelated cores, yet it reacts too
slowly to catch a short burst on the measured core. This gate instead reads
per-CPU busy time from /proc/stat for the three places where other work can
perturb a pinned run:

* the measured CPU itself, after subtracting the child's own CPU time;
* its SMT siblings, which share the core's execution units, op cache and L1/L2;
* the remaining CPUs that share its L3.

Other L3 domains are not consulted.
"""

from dataclasses import dataclass
import os
from pathlib import Path
import resource
import time

USER_HZ = os.sysconf('SC_CLK_TCK')


def cpu_list(text: str) -> set[int]:
    cpus = set()
    for part in text.strip().split(','):
        low, _, high = part.partition('-')
        cpus.update(range(int(low), int(high or low) + 1))
    return cpus


def topology(cpu: int) -> tuple[set[int], set[int]]:
    """Return (SMT siblings, other CPUs sharing the last-level cache) of `cpu`."""
    base = Path(f'/sys/devices/system/cpu/cpu{cpu}')
    siblings = cpu_list((base / 'topology/thread_siblings_list').read_text()) - {cpu}
    l3 = cpu_list((base / 'cache/index3/shared_cpu_list').read_text()) - siblings - {cpu}
    return siblings, l3


def busy_seconds() -> dict[int, float]:
    """Busy (non-idle, non-iowait) seconds per CPU since boot."""
    result = {}
    for line in Path('/proc/stat').read_text().splitlines():
        name, *fields = line.split()
        if not name.startswith('cpu') or name == 'cpu':
            continue
        values = [int(value) for value in fields[:8]]
        idle = values[3] + values[4]
        result[int(name[3:])] = (sum(values) - idle) / USER_HZ
    return result


def child_cpu_seconds() -> float:
    usage = resource.getrusage(resource.RUSAGE_CHILDREN)
    return usage.ru_utime + usage.ru_stime


@dataclass(frozen=True)
class Limits:
    # Fractions of wall time. One /proc/stat tick is 10 ms, so the foreign and
    # sibling allowances also carry a fixed two-tick quantization margin.
    foreign: float = 0.02
    sibling: float = 0.03
    l3_mean: float = 0.25
    margin_s: float = 2 / USER_HZ


class CoreGate:
    def __init__(self, cpu: int, limits: Limits = Limits()):
        self.cpu = cpu
        self.limits = limits
        self.siblings, self.l3 = topology(cpu)

    def _window(self, start, end, wall, own=0.0) -> dict:
        measured = end[self.cpu] - start[self.cpu]
        sibling = max((end[c] - start[c] for c in self.siblings), default=0.0)
        l3 = [end[c] - start[c] for c in self.l3]
        foreign = max(0.0, measured - own)
        sample = {
            'wall_s': wall, 'own_cpu_s': own, 'foreign_s': foreign,
            'sibling_busy': sibling / wall, 'l3_mean_busy': (sum(l3) / len(l3) / wall) if l3 else 0.0,
            'loadavg': os.getloadavg(),
        }
        limits = self.limits
        sample['ok'] = (foreign <= limits.foreign * wall + limits.margin_s
                        and sibling <= limits.sibling * wall + limits.margin_s
                        and sample['l3_mean_busy'] <= limits.l3_mean)
        return sample

    def wait_quiet(self, deadline: float, journal: list, window_s: float = 1.0) -> dict:
        """Block until one idle window passes the limits; journal each failed window."""
        while True:
            start, t0 = busy_seconds(), time.monotonic()
            time.sleep(window_s)
            sample = self._window(start, busy_seconds(), time.monotonic() - t0)
            if sample['ok']:
                return sample
            journal.append({'phase': 'core-quiet-wait', **sample})
            if time.monotonic() >= deadline:
                raise TimeoutError(f'core-quiet deadline expired: {sample}')

    def begin(self) -> tuple:
        return busy_seconds(), child_cpu_seconds(), time.monotonic()

    def end(self, token: tuple) -> dict:
        """Judge the interval since `begin`, crediting reaped children's CPU time as own."""
        start, own_start, t0 = token
        return self._window(start, busy_seconds(), time.monotonic() - t0,
                            child_cpu_seconds() - own_start)
