"""Benchmark: pure-Python YAML dump and load of a nested document."""

import yaml

DOCUMENT = {
    "items": [
        {"id": i, "name": f"item-{i}", "tags": ["a", "b"], "price": i / 4}
        for i in range(300)
    ],
}
for _ in range(5):
    assert yaml.safe_load(yaml.safe_dump(DOCUMENT)) == DOCUMENT
