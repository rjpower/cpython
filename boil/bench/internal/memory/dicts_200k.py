"""200k small dicts alive: a list of {'a': i, 'b': str(i)} for i in range(200_000).

The workload of the former boil/tools/rss.py "dicts" row.  Each loop builds
the list and drops it.
"""


def run(loops):
    for _ in range(loops):
        alive = [{'a': i, 'b': str(i)} for i in range(200_000)]
        del alive
