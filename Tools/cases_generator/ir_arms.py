"""The register instruction arms as the handler tables list them.

Handler numbers are not stored: the generators compute them at each
build (handler_table.py: the word kinds, then an arm's id is first_arm() + its
index).
"""

from dataclasses import dataclass


class HandlerLimitError(Exception):
    pass


# Handler ids fit in the low IR_HANDLER_BITS (9) bits of the handler field;
# the bits above them are per-site flags (Include/internal/pycore_ir.h).  The
# dispatch table has an entry for every id the field can hold.
IR_LIMIT = 512


@dataclass(frozen=True)
class ArmFormat:
    """What the per-handler tables hold for an arm, derived from its family's
    encoding and its declared effects and exits (rop_generator.arm_format).
    Values are the C names of pycore_ir.h's enumerations."""
    flags: tuple[str, ...]
    effects: tuple[str, ...]
    roles: tuple[str, str, str, str]
    imm: str
    fb: str
    fb_schema: str | None     # the schema name: Binary for FB_BINARY
    operands: str             # _PyIR_OPERANDS_*
    fixed_words: int          # operand words after the head
    transfer: str             # _PyIR_TRANSFER_*: the normal edge's ownership
    target: str = "_PyIR_LOC_NONE"   # where a jump target lives (flag FORWARD)
    # The word's homes (rop_generator.family_homes): the home group's parts
    # (its GROUP field names part 0), the homes that may be vacant when
    # read (bit h), the committed edges (bit e) and each home's state after
    # each normal edge (_PyIR_HOME_* in two bits at 2 * (6e + h)).  Home h
    # is field h for h < 4, and part h - 3 of the group for h = 4, 5.
    group: int = 0
    normal: int = 0           # bit e: an arm of the family takes edge e
    vacant: int = 0
    committed: int = 0
    edges: int = 0


@dataclass(frozen=True)
class ArmEntry:
    """A register instruction arm (Python/register_ops) as the tables of
    this generator list it: its handler id is first_arm() + index.  An
    observed wrapper (a twin) is listed after the arms, with `observes` the
    index of the arm it wraps."""
    name: str                 # the C name: Add_Int
    index: int                # among all arms, in definition order
    region: str               # the plan: "inline", "shared" or "none" (split)
    escapes: bool
    stub: str | None          # the stub function of a shared arm
    generic: str | None       # the stub function of its family's generic arm
    dotted: str = ""          # Add.Int
    events: tuple[str, ...] = ()   # PY_MONITORING_EVENT_* names
    adaptive: bool = False    # the family adapts: its generic arm specializes
    is_generic: bool = False
    format: ArmFormat | None = None
    twin: int | None = None   # the index of this arm's observed wrapper
    observes: int | None = None  # for a wrapper: the index of its arm
    emitted: bool = True      # the interpreter has a case for it
    trailing: bool = False  # a jump of the assembler: no twin, keeps the IP
    displacing: bool = False  # the word that monitoring puts over another
