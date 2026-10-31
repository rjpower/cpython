"""Smoke test for six: lazy moves, metaclass helpers and text helpers."""

import configparser
import importlib.metadata

import six
from six.moves import range as six_range
from six.moves import urllib

assert six.__version__ == "1.17.0", six.__version__
assert importlib.metadata.version("six") == "1.17.0"
assert not six.PY2 and six.string_types == (str,)

# six.moves is a meta-path importer that builds modules lazily.
assert six.moves.configparser.ConfigParser is configparser.ConfigParser
assert list(six_range(3)) == [0, 1, 2]
assert urllib.parse.quote("a b") == "a%20b"


class Meta(type):
    def __new__(mcls, name, bases, namespace):
        namespace["tagged"] = name
        return super().__new__(mcls, name, bases, namespace)


class Base:
    pass


@six.add_metaclass(Meta)
class Decorated(Base):
    pass


class Derived(six.with_metaclass(Meta, Base)):
    pass


assert type(Decorated) is Meta and Decorated.tagged == "Decorated"
assert type(Derived) is Meta and Derived.__mro__[1:] == (Base, object)
assert six.ensure_text(b"caf\xc3\xa9") == six.ensure_str("café")
assert list(six.iteritems({"a": 1})) == [("a", 1)]
try:
    six.raise_from(ValueError("outer"), KeyError("inner"))
except ValueError as error:
    assert isinstance(error.__cause__, KeyError)
else:
    raise AssertionError("raise_from did not raise")
