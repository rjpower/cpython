# Compact F+N+L composition: cold source peer review

Source GO for coordinator regeneration and qualification at
`82afe7aef2d`. No unresolved source blocker found. This is a bounded geometry
composition, not a selected winner. The reviewer changed no runtime source and
ran no generator, compiler, interpreter, test, profiler or timing.

Compared the composition against golden51
`0e5e61f4c6bc820f2dd1a5f9ee6f7f1768834254` and component endpoints
F `7ea40d5d67ccb5a9250b8313646efb537c7ff934`,
N `c2cc6777e4d72e6688854c0423459971fc81b355`, and
L `b48fe2d39896fef83d795bba3ec9c95af68cc235`.
The resolved shared initializer consumes only function and prepared frame-object
owners; it neither allocates nor reconstructs removed caches. Lazy-import
identity checks compare the function-derived globals with the actual mapping
owner. GLOBALS/BUILTINS/LOCALS macros use their respective accessors.

Execution authority remains independent of mutable function code: f_executable
pins the executing code, ir_words retains its selected image identity, and feedback
comes from the adjacent _PyCodeExec prefix. Ordinary allocation, static cleanup
and the entry trampoline retain their real prefixes. The embedded thread base
has no words and receives no new feedback access. Fallback refresh updates the
word/current-PC authority without reconstructing feedback through func_code.
Generator call-reload emission also uses the derived feedback accessor.

N and L retain distinct ownership: namespaces derive from the strong function;
the nullable supplied mapping is visited exactly once on PyFrameObject.
Interpreter-frame traversal visits that object, function, code and homes, without
a duplicate mapping edge. Precise root walking retains function-kind checks for
the None-function cleanup shim. Unobserved thread records remain untracked;
complete observer/heap publication tracks them. The existing precise and
conservative root paths therefore retain the mapping through staging, while
heap cycles remain collectible. Unique retirement still detaches the mapping
into C, retires homes, then mapping, then function; escaped takeover retains the
same mapping and independent code/function owners. No derived namespace read
was added to the clear/deallocation sequence.

All L publication corrections survive composition: copies transfer homes and
frame_obj, establish destination owner/association and tracking before CREATED;
RETURN_GENERATOR selects its incoming recipient before the move. Direct C-API
conversion selects IR_NO_DST under the frame lock before Copy and publishes
complete generator metadata. The helper-aware suspension inference and its
positive/negative tests remain present; the current metadata row already says
SUSPENDS rather than PUSHES_FRAME. Root must regenerate the composed products.

Sentinel observation remains bounded to cold paths: complete-frame walks skip
the entry/base and two-word cleanup shim; LLTrace checks owner and function kind
before deriving globals, with no eager GLOBALS argument at either call site.
SafeGetCode uses executable validity without a blind function lookup, and GDB
checks the function class before environment access.

An exhaustive source-field search found no direct globals, builtins, locals or
feedback member access after their removal, including generated C. Header
geometry consumers use FRAME_SPECIALS_SIZE, sizeof or offsetof: deepfreeze,
chunk reservation, frame/generator tails, debug offsets and observer extents.
The predicted GIL header is 80 bytes (112 minus four pointer words); this remains
a native gate, not a measured layout result or proof of throughput. The existing
full physical-home move/NULL discipline, separate function/code pins, kwargs
materialization and recipient representation remain intact.

Root still owns matching regeneration/builds, actual release/debug geometry and
native dependencies, merged focused ownership/observer/GC/C-API qualification,
and direct balanced performance evidence. The composition combines recurring
function/prefix loads with eager mapping-record costs; isolated results or a
smaller predicted header cannot establish an admitted combined result.
