"""Smoke test for markupsafe; the C speedups must be the active escaper."""

import importlib.metadata

import markupsafe
import markupsafe._speedups
from markupsafe import Markup, escape, escape_silent

assert importlib.metadata.version("markupsafe") == "3.0.4"
# markupsafe silently falls back to pure Python if _speedups fails to build
# or import, and this port exists to exercise the C extension.
assert markupsafe._escape_inner is markupsafe._speedups._escape_inner
assert markupsafe._speedups.__file__.endswith(".so")

assert escape("<a href='x'>&\"") == (
    "&lt;a href=&#39;x&#39;&gt;&amp;&#34;"
)
assert type(escape("plain")) is Markup and escape(5) == "5"
assert escape_silent(None) == "" and escape(None) == "None"
# Each string width takes a separate path through _speedups.
for char in ("é", "€", "\U0001f600"):
    text = (char + "<") * 1000
    assert escape(text) == (char + "&lt;") * 1000


class Html:
    def __html__(self):
        return "<b>safe</b>"


assert escape(Html()) == "<b>safe</b>"
assert Markup("<p>{}</p>").format("<i>") == "<p>&lt;i&gt;</p>"
assert Markup("<p>%s</p>") % "&" == "<p>&amp;</p>"
assert Markup("<em>a &amp; b</em>").striptags() == "a & b"
assert Markup("&lt;&#39;").unescape() == "<'"
assert Markup("a") + "<" == "a&lt;"
