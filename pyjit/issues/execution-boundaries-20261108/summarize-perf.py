"""Validate the final logging counter capture and normalize by outer iterations."""

from collections import defaultdict
import hashlib
import json
from pathlib import Path
from statistics import median


EVIDENCE = Path(
    '/home/power/code/cpython/.worktrees/python-boil-the-ocean/build/'
    'pyjit-link-wave1/execution-boundaries-20261108'
)


def main():
    capture = EVIDENCE / 'perf-stat-v2'
    status = json.loads((capture / 'status.json').read_text())
    assert status['status'] == 'diagnostic-complete'
    assert status['failed_invocations'] == 0
    rows = json.loads((capture / 'invocations.json').read_text())
    assert len(rows) == status['scheduled_invocations'] == 18
    heads = {'upstream': '3a440b65695', 'candidate': '4b3d739d867'}
    counts = defaultdict(list)
    running = []
    for row in rows:
        assert row['valid'] and row['case'] == 'logging_silent'
        assert row['child']['native_before'] == row['child']['native_after']
        identity = row['child']['native_before']
        assert identity['git'][2] == heads[row['role']]
        assert identity['affinity'] == [15] and not identity['jit_enabled']
        assert not identity['natural_method_api_present']
        assert identity['hash_randomization'] == 0
        for event in row['stat_validation']['events']:
            assert event['valid'] and event['pcnt_running'] >= 99
            running.append(event['pcnt_running'])
            counts[row['role'], event['event']].append(event['count_per_loop'])
    assert all(len(values) == 3 for values in counts.values())
    hashes = json.loads((EVIDENCE / 'perf-identities/input-hashes.json').read_text())
    for name, expected in hashes.items():
        assert hashlib.sha256(Path(name).read_bytes()).hexdigest() == expected, name
    events = {}
    for name in sorted({name for role, name in counts}):
        upstream = median(counts['upstream', name])
        candidate = median(counts['candidate', name])
        events[name] = {
            'upstream_per_loop': upstream,
            'candidate_per_loop': candidate,
            'candidate_over_upstream': candidate / upstream,
            'upstream_raw': counts['upstream', name],
            'candidate_raw': counts['candidate', name],
        }
    result = {'invocations': len(rows), 'minimum_running_percent': min(running),
              'inputs_unchanged': True, 'events': events}
    with (EVIDENCE / 'perf-summary.json').open('x') as output:
        json.dump(result, output, indent=2)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
