# Diagnostic sampler review

Use `diagnostic-sampler-v2.py`; the first draft is retained but is not the
execution packet. Coordinator source review accepts v2 after four corrections:
isolate the interpreter environment and bytecode cache, check executable ELF
segments rather than data segments, retain failed-child output, and reject an
empty sample stream. Syntax checks do not establish a successful profile.

Run only after elapsed-time and RSS measurements. Compile the existing sampler
separately, require depth one and `SAMPLER_US=0`, then enable its installed
SIGPROF handler only around the warmed benchmark call. Select loop counts from
the fresh matched timing medians. Retain commands, executable copies, ELF notes,
raw samples, maps and symbol reports. The source and all mapped build products
must remain frozen during collection and symbolization.

This is statistical leaf-PC sampling, without hardware instruction counts or
caller stacks. A PyValue function name does not imply an allocation. Attribute
only the work shown by actual symbols; unresolved library work remains
unresolved. The existing symbolizer is allowed only after executable PT_LOAD
offsets and virtual addresses have been checked. For later reanalysis, map the
original executable pathname to the retained executable copy; never symbolize
old addresses against a rebuilt interpreter or extension.
