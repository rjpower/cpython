"""Smoke test for attrs: generated slotted classes, validators, freezing."""

import importlib.metadata

import attrs

assert importlib.metadata.version("attrs") == "26.1.0"


class Base:
    __slots__ = ()

    def describe(self):
        return "base"


@attrs.define(order=True)
class Point(Base):
    x: int = attrs.field(converter=int)
    y: int = attrs.field(default=0, validator=attrs.validators.ge(0))
    tags: list = attrs.Factory(list)

    def describe(self):
        # Zero-argument super() needs the __class__ cell attrs rewrites.
        return f"{super().describe()}:{self.x}"


point = Point("3", 4)
assert point == Point(3, 4) < Point(3, 5) and point.describe() == "base:3"
assert repr(point) == "Point(x=3, y=4, tags=[])", repr(point)
assert attrs.asdict(attrs.evolve(point, y=9))["y"] == 9 and point.y == 4
assert [field.name for field in attrs.fields(Point)] == ["x", "y", "tags"]
assert "__slots__" in Point.__dict__ and not hasattr(point, "__dict__")


@attrs.frozen
class Name:
    value: str


name = Name("a")
assert hash(name) == hash(Name("a")) and Point(1).tags is not Point(1).tags
for action, error in [(lambda: Point(1, -1), ValueError),
                      (lambda: setattr(name, "value", "b"),
                       attrs.exceptions.FrozenInstanceError)]:
    try:
        action()
    except error:
        continue
    raise AssertionError(f"{error.__name__} was not raised")
