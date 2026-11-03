"""Benchmark: escape and format mixed-width text through _speedups."""

from markupsafe import Markup, escape

TEXT = "<a href='x'>café &amp; €</a> " * 200
TEMPLATE = Markup("<p>{}</p>")
for _ in range(20000):
    escape(TEXT)
    TEMPLATE.format(TEXT)
