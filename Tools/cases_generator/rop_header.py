"""The migration check between `.rop` externs and pycore_ir_components.h.

While an old stack uop still reads a header row that an extern also
declares, the two must agree on the C symbol, the effects and the exits.
An extern names its symbol in `c SYMBOL`; a header row of that name is the
other statement.  Unit D deletes the header's ported rows and this module.
"""

from collections.abc import Mapping
from pathlib import Path

import ir_contract
from ir_contract import Row
from rop_model import Diagnostic, Extern


def header_rows(path: Path = ir_contract.COMPONENTS_HEADER) -> dict[str, Row]:
    return ir_contract.parse_rows(path.read_text(), path.name)


def disagreements(externs: Mapping[str, Extern], rows: Mapping[str, Row]
                  ) -> list[Diagnostic]:
    out = []
    for x in externs.values():
        if x.c is None or x.c.symbol not in rows:
            continue
        row = rows[x.c.symbol]
        effects = frozenset(e.effect.value for e in x.contract.effects)
        exits = frozenset(x.contract.exits)
        where = f"header row {row.kind}({row.name})"
        if effects != row.effects:
            out.append(Diagnostic(x.c.span, x.name, f"c {x.c.symbol}: effects "
                                  f"({', '.join(sorted(effects))}) disagree with {where}'s "
                                  f"({', '.join(sorted(row.effects))})"))
        if exits != row.exits:
            out.append(Diagnostic(x.c.span, x.name, f"c {x.c.symbol}: exits "
                                  f"({', '.join(sorted(exits))}) disagree with {where}'s "
                                  f"({', '.join(sorted(row.exits))})"))
    return out
