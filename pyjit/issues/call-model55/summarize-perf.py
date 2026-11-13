"""Summarize completed sealed PMU captures without pooling campaigns."""

import argparse
import json
from pathlib import Path
import re
import statistics
import subprocess


def summarize_stat(folder, rows):
    summary = {}
    for case in dict.fromkeys(row['case'] for row in rows):
        case_rows = [row for row in rows if row['case'] == case]
        events = case_rows[0]['stat_validation']['events']
        summary[case] = {}
        for event in events:
            name = event['event']
            values = {role: [] for role in ('upstream', 'candidate')}
            for role, samples in values.items():
                selected = sorted((row for row in case_rows if row['role'] == role),
                                  key=lambda row: row['round'])
                for row in selected:
                    metric = next(metric for metric in row['stat_validation']['events']
                                  if metric['event'] == name)
                    assert metric['valid'] and metric['pcnt_running'] == 100
                    samples.append(metric['count_per_loop'])
            assert len(values['candidate']) == len(values['upstream']) == 3
            ratios = [a / b for a, b in zip(values['candidate'], values['upstream'])]
            summary[case][name] = dict(
                paired_ratios=ratios, geomean=statistics.geometric_mean(ratios),
                min=min(ratios), max=max(ratios), per_loop=values)
    (folder / 'normalized-summary.json').write_text(json.dumps(summary, indent=2))
    return summary


def summarize_record(folder, rows):
    summary, commands = {}, []
    for row in rows:
        command = ['perf', 'report', '--stdio', '--no-children', '--call-graph',
                   'none', '--percent-limit', '.5', '-i', row['perf_data']]
        name = f"flat-{row['role']}-{row['case']}.txt"
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        (folder / name).write_text(result.stdout)
        (folder / (name + '.stderr')).write_text(result.stderr)
        commands.append(dict(command=command, returncode=0, output=name))
        samples = []
        for line in result.stdout.splitlines():
            match = re.match(r'\s*([\d.]+)%.*?\[\.\]\s+(.*)', line)
            if match:
                samples.append(dict(percent=float(match[1]), symbol=match[2]))
        summary[f"{row['role']}-{row['case']}"] = samples
    (folder / 'flat-commands.json').write_text(json.dumps(commands, indent=2))
    (folder / 'flat-summary.json').write_text(json.dumps(summary, indent=2))
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('folder', type=Path)
    args = parser.parse_args()
    status = json.loads((args.folder / 'status.json').read_text())
    assert status['complete'] and status['stable_identity']
    assert status['failure'] is None and status['postflight_error'] is None
    rows = json.loads((args.folder / 'invocations.json').read_text())
    assert len(rows) == status['scheduled_invocations'] == status['valid_invocations']
    assert all(row['valid'] for row in rows)
    summarize = summarize_stat if status['mode'] == 'stat' else summarize_record
    print(json.dumps(summarize(args.folder, rows), indent=2))


if __name__ == '__main__':
    main()
