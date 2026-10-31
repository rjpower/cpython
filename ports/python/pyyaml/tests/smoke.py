"""Smoke test for PyYAML, including the libyaml-backed C loader and dumper
(see ../README.md)."""

import datetime
import importlib.metadata

import yaml

assert yaml.__version__ == "6.0.3", yaml.__version__
assert importlib.metadata.version("pyyaml") == "6.0.3"

DOCUMENT = {
    "name": "ports",
    "count": 3,
    "ratio": 0.5,
    "flags": [True, False, None],
    "when": datetime.date(2026, 10, 23),
    "nested": {"list": [1, [2, 3]], "text": "multi\nline café"},
}
text = yaml.safe_dump(DOCUMENT, allow_unicode=True)
assert yaml.safe_load(text) == DOCUMENT
assert yaml.load(text, Loader=yaml.SafeLoader) == DOCUMENT

# Anchors, aliases and explicit tags go through the composer.
first, second = yaml.safe_load_all("a: &x [1, 2]\nb: *x\n---\n- !!str 5\n")
assert first == {"a": [1, 2], "b": [1, 2]} and first["a"] is first["b"]
assert second == ["5"]


class Point(yaml.YAMLObject):
    yaml_tag = "!Point"
    yaml_loader = yaml.SafeLoader


point = yaml.safe_load("!Point {x: 1, y: 2}")
assert (point.x, point.y) == (1, 2)
try:
    yaml.safe_load("a: [1, 2")
except yaml.YAMLError:
    pass
else:
    raise AssertionError("malformed YAML was accepted")

# The C extension must be present, and agree with the pure-Python path.
assert yaml.__with_libyaml__
assert yaml.load(text, Loader=yaml.CSafeLoader) == DOCUMENT
assert yaml.load(text, Loader=yaml.CLoader) == DOCUMENT
c_text = yaml.dump(DOCUMENT, Dumper=yaml.CSafeDumper, allow_unicode=True)
assert yaml.load(c_text, Loader=yaml.CSafeLoader) == DOCUMENT
assert yaml.safe_load(c_text) == DOCUMENT
try:
    yaml.load("a: [1, 2", Loader=yaml.CSafeLoader)
except yaml.YAMLError:
    pass
else:
    raise AssertionError("malformed YAML was accepted by the C loader")
