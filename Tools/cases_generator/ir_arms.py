"""The register instruction arms as the handler tables list them.

Handler numbers are not stored: the cases generator computes them at each
build (analyzer.assign_opcodes for the stack opcodes, ir_generator.handler_ids
for the IR-only handlers, and an arm's id is num_old_handlers + its index).
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
    ninputs: int
    dst: int | None           # None: _PyIR_NO_DST
    borrow_mask: int
    fb_schema: str | None     # the schema name: Binary for FB_BINARY
    operands: str             # _PyIR_OPERANDS_*
    fixed_words: int          # operand words after the head
    writes: int
    transfer: str             # _PyIR_TRANSFER_*: the normal edge's ownership
    target: str = "_PyIR_LOC_NONE"   # where a jump target lives (flag FORWARD)
    # The words the observer reads at the edges of a branch: the format
    # (reads, writes, through) says what the edge leaves in the homes
    observer: str = "_PyIR_OBSERVER_NONE"
    # The fact of the taken edge where it differs (a loop's exhaustion)
    observer_taken: str | None = None
    # Set where the stack positions that the word reads and passes through
    # differ from its inputs (a window)
    reads: int | None = None
    through: int = 0


@dataclass(frozen=True)
class ArmEntry:
    """A register instruction arm (Python/register_ops) as the tables of
    this generator list it: its handler id is num_old_handlers + index.  An
    observed wrapper (a twin) is listed after the arms, with `observes` the
    index of the arm it wraps."""
    name: str                 # the C name: Add_Int
    index: int                # among all arms, in definition order
    region: str               # the plan: "inline", "shared" or "none" (split)
    escapes: bool
    stub: str | None          # the stub function of a shared arm
    generic: str | None       # the stub function of its family's generic arm
    dotted: str = ""          # Add.Int
    base: str = ""            # the first stack op of its family: BINARY_OP
    events: tuple[str, ...] = ()   # PY_MONITORING_EVENT_* names
    adaptive: bool = False    # the family adapts: its generic arm specializes
    is_generic: bool = False
    format: ArmFormat | None = None
    twin: int | None = None   # the index of this arm's observed wrapper
    observes: int | None = None  # for a wrapper: the index of its arm
    emitted: bool = True      # the interpreter has a case for it
