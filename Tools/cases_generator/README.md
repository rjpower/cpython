# Tooling to generate the interpreter

The interpreter's behavior is defined by the register instruction arms in
`Python/register_ops/*.rop`. The stack ops that `co_code` and `.pyc` files
hold are data, in `stack_ops.py`: transport that the translator lowers to
register instructions.

What's currently here:

- `rop_syntax.py`, `rop_model.py`, `rop_check.py`, `rop_verify.py`,
  `rop_catalog.py`, `rop_primitives.py`: the definition language,
  its checker and the declared catalogs
- `rop_cgen.py`: lowering of the checked definitions to C
- `rop_generator.py`: writes the interpreter cases, the completion arms of
  `gen_return`, the dispatch tables, the stencil declarations, and
  `Include/internal/pycore_rop_metadata.h`
- `ir_metadata_generator.py`: writes the handler formats and other tables of
  the register IR to `Include/internal/pycore_ir_metadata.h`
- `regcfg_metadata_generator.py`: writes the translator's rows of the stack
  ops to `Include/internal/pycore_regcfg_metadata.h`
- `handler_table.py`, `ir_arms.py`: the handler ids, which are dense and
  independent of the stack opcodes
- `stack_ops.py`: the stack ops, their pinned numbers and the translator's rows
- `opcode_id_generator.py`, `opcode_metadata_generator.py`,
  `py_metadata_generator.py`: write the opcode ids and metadata from
  `stack_ops.py`
- `lexer.py`, `cwriter.py`, `generators_common.py`: helpers

`make regen-cases` runs the generators.
