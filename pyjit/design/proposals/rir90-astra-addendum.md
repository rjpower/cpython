# PyJIT register semantics — implementation addendum

**Adopt a separate, typed definition language.** Its unit of execution is a register instruction family. An arm is a control-flow graph. A macro expands into that graph and has no handler number, dispatch entry, stack signature, or independent tracer translation. The 373 old uops are a coverage oracle. Appendix A specifies what to build. Appendix B checks what the replacement must account for.

I would change three things in v1. First, make arithmetic, adaptation, and ownership operations precise enough that a backend never has to interpret pseudocode. Second, put `SWAP`, most `COPY`s, and most discards in the first-call translator. Giving each a new executable instruction would preserve much of the original problem. Third, qualify the ADD performance claim. The present native template **already has one pair guard and tests for allocation failure only on overflow**. Register semantics simplify the interpreter and the generator. They do not, by themselves, make that native template shorter.

## 1. The definition language

### Choice

| | Restricted C with a typed builder | Separate language with typed labels |
|---|---|---|
| Parser | A builder compiled as C gets the host parser, but a Python generator still needs either a frontend dependency or a parser for the allowed subset. It must reject legal C that has no graph meaning. | Recursive descent over the existing lexer’s identifiers, numbers, punctuation, comments, and source spans. No C declarators, conversions, assignment expressions, preprocessor, or precedence table. |
| Checker | Must recover sequencing, short-circuit evaluation, address-taking, and failure edges from C, or encode them in builder calls. The latter is already a second language with C punctuation. | Labels establish joins, status calls enumerate edges, and ownership operations name the homes involved. The checker operates directly on the graph the backends consume. |
| Errors | A C type error can be good. A valid C expression that retains a stale borrow needs a second diagnostic from the graph checker. Macro expansion can put that diagnostic far from the definition. | Report the producing row, invalidating action, offending use, and expansion site. For example, “`slot` borrowed at line 18 crosses `PythonGetAttr` at line 21; used at line 23.” |
| Review | Familiar to C reviewers for leaf arithmetic. A reviewer must distinguish C that runs at build time from actions that run in the VM. | A reviewer sees the admission conditions, first effect, owner publication, and every exit in execution order. The small vocabulary is worth learning once. |
| Estimated new implementation | 450–850 lines of parsing, rejection, and normalization plus 900–1,600 lines of graph/type/ownership checking. | 250–400 lines of parsing plus 600–1,000 lines of graph/type/ownership checking. |

Those are planning estimates for this checkout, excluding tests, row declarations, and target generators. They are not measurements of V8. Here, `lexer.py` is 399 lines, `plexer.py` is 124, `parsing.py` is 894, and `ir_contract.py` is 4,914. Reuse token locations, row parsing, facts, effect coverage, and diagnostics. Do not port the C-expression ownership walker into the new frontend. Make lexing fail on every unconsumed non-trivia byte. V8’s useful precedent is the distinction between a typed, inlinable Torque macro and a callable builtin. SpiderMonkey’s useful precedent is that a failing guard leaves an IC restartable. Python needs a CFG after admission because allocation, reentry, suspension, and publication have different failure states. [V8 Torque manual](https://v8.dev/docs/torque) · [V8 CSA guide](https://v8.dev/docs/csa-builtins) · [SpiderMonkey CacheIR](https://firefox-source-docs.mozilla.org/js/cacheir.html) · [Existing lexer][lexer] · [Existing verifier][contract]

### Grammar

I recommend `Python/register_ops/` for definitions, with ordinary files grouped by language family. The following is the grammar of the executable definition language. `list(X)` means zero or more comma-separated `X`s. `nonempty(X)` means one or more. Keywords are contextual. `id` is an ASCII identifier, `nat` is an unsigned decimal integer, and `q` is a qualified name. Whitespace and C-style comments are trivia.

```ebnf
file      = { family | point | arm | macro } ;
q         = id, { ".", id } ;
param     = id, ":", type ;
params    = "(", list(param), ")" ;
args      = "(", list(expr), ")" ;

family    = "family", id, params, "layout", id, ";" ;
point     = "point", q, "(", frameRole, ",", pcRole, ",", excRole, ")",
            "keep", names, "spent", names, "defs", names, "held", names, ";" ;
frameRole = "current" | "caller" | "child" | "consumer" | "external" ;
pcRole    = "site" | "next_head" | "target" | "resume_head" | "saved_origin" ;
excRole   = "clear" | "pending" | "handled" | "inherited" ;
names     = "(", list(id), ")" ;

arm       = "arm", q, region, contract, graph ;
region    = "inline" | "shared" | "split" ;
macro     = "macro", id, params, "cases", "(", nonempty(caseDecl), ")",
            contract, graph ;
caseDecl  = id, params ;
contract  = "effects", "(", list(effect), ")",
            "feedback", "(", list(access), ")",
            "exits", names,
            [ "requires", "(", list(fact), ")" ],
            [ "ensures", "(", list(edgeFact), ")" ] ;
effect    = effectName, [ "on", names ] ;
access    = ("read" | "write"), "(", q, ")" ;
fact      = q, "(", list(q), ")" ;
edgeFact  = id, ".", fact ;

graph     = "{", block, { block }, "}" ;
block     = id, params, ":", { stmt }, term ;
stmt      = "guard", expr, "else", "miss", ";"
          | "let", id, ":", type, "=", expr, ";"
          | "publish", "[", list(binding), "]",
            "retire", "[", list(debt), "]", "at", q, ";" ;
binding   = q, "=", expr ;
debt      = ("spent" | "old" | "roots" | "owned"), "(", q, ")" ;
term      = "goto", target, ";"
          | "branch", expr, "->", target, ",", target, ";"
          | "call", q, args, "{", nonemptyStatusEdges, "}"
          | "each", expr, "limit", (nat | "length"),
            "body", id, "exhaust", target, ";"
          | "advance", id, ";"
          | "exit", id, args, ";" ;
nonemptyStatusEdges = statusEdge, { statusEdge } ;
statusEdge = id, "->", target, ";" ;
target    = id, args ;
expr      = q | nat | "true" | "false" | q, args | "[", list(expr), "]" ;
```

The type grammar is deliberately closed. New runtime concepts require a reviewed change to the type or schema catalog.

```ebnf
type = scalar | handle
     | ("Maybe" | "Span" | "InOut"), "<", type, ">"
     | "Pair", "<", type, ",", type, ">"
     | ("Fb" | "Recipe" | "Candidate" | "Rows" | "Ticket" | "Arm"),
       "<", schema, ">"
     | "Enum", "<", enumName, ">" ;

scalar = "Unit" | "Bool" | "U8" | "U16" | "U32" | "U64"
       | "I32" | "I64" | "F64" | "I51" | "Scaled51" | "Size" ;
handle = "B" | "O" | "Src" | "Dst" | "Local" | "Cell" | "Free"
       | "Name" | "Const" | "Head" | "Point" | "Site" | "Thread"
       | "Activation" | "Frame" | "NewFrame" | "DeadFrame" | "Code"
       | "Obj" | "Fun" | "Gen" | "Cursor" | "IterPair" | "Args"
       | "Anchors" | "Err" | "Route" | "Claim" | "Pin" | "Stamp" ;
schema = "Binary" | "Attr" | "Global" | "Truth" | "Compare" | "Contains"
       | "Item" | "Unpack" | "Iter" | "Send" | "Call" | "Super" ;
```

`enumName`, layouts, facts, effects, schema fields, and component names resolve against checked declaration tables. The effect names are the existing `reads`, `writes`, `allocates`, `raises`, `releases`, `runs_python`, `polls`, and `switches_frame`. An `Enum` is a declared finite domain such as the numeric operation or comparison sense. `B` is a borrowed PyValue and `O` is an owning PyValue. An inline `O` satisfies its ownership obligation without a reference count. `Maybe<B>` can contain VM NULL. Python `None` is a present value. Typed `Src` and `Dst` bindings carry the home class and the permitted consume/discard behavior. Facts can refine these types without inventing a C pointer type.

The following rules complete the grammar.

1. The first block is `entry()`. Names are single assignment. A use must be dominated by its definition. A `goto` binds block parameters simultaneously. A status edge prepends its row’s declared payload to the explicit target arguments. The checker requires exactly one edge for each status. The only implicit environment names are the declared execution context `thread`, `activation`, `frame`, `site`, and `next_head`.
2. A nested call expression must be a total, read-only row or macro. The outermost expression of `let` may also be an infallible action. A fallible or multi-status operation must use `call`. `_` discards only `Unit` or a non-linear scalar. An owner, error, claim, pin, or ticket cannot be discarded that way.
3. A guard can read operands, immutable fields, and a coherent cache recipe. Its true edge acquires exactly the facts its `TEST` row declares. `miss` requires the original operand identities, no outstanding new obligations, and no language-visible action. Every later failure has a typed continuation. A macro’s exits are its declared cases. An arm’s exits are drawn from `next`, `branch`, `jump`, `miss`, `again`, `raise`, `enter`, `return`, `yield`, `exit`, and `redirect`.
4. `each` evaluates a checked span once. The body receives an element and an index. `advance bodyLabel` is its sole backedge. A literal limit is a maximum number of entries examined, and `length` is the checked finite length captured at entry. The verifier rejects all other CFG cycles and recursive macro expansion. For a PIC, `Rows<Attr>` certifies at most four rows before the loop. For a dynamically sized argument vector, checked `length` avoids an artificial arity limit. A row span borrowed from mutable storage cannot cross reentry.
5. `publish` evaluates all inputs before mutating a home. It saves displaced owners once per physical home, detaches spent temporaries, installs **all** results, publishes the checkpoint, then discharges the listed debts in order. Two independently live outputs require two owners. A repeated operand does not create two claims on one temporary. A surviving local displaced after reentry supplies its **current** old owner. `keep`, `spent`, and `defs` describe logical roles after physical alias resolution. Unmentioned homes inherit their incoming state. In particular, `spent(x)` leaves a surviving source local alone.
6. A checkpoint records current frame, origin or successor, exception state, and named ownership changes. Restart permission is separate. A Python call that raised can leave all original homes intact and still forbid replay. Reentrant calls and releases carry the existing G0 home and observer contracts. A control-generation change abandons the old continuation. The checked boundary adapter reloads the frame and selected head before continuing after an escape.
7. There are **no C arithmetic operators**, casts, arbitrary pointer dereferences, assignments through pointers, or implicit integer promotions. Arithmetic and bit operations use declared `PRIMITIVE` rows. Examples are `and_u64`, `shr_u64`, `add_wrap_u64`, `add_checked_scaled`, and `sum_i51_wide`. Each declares its width, overflow or shift-count behavior, result facts, C implementation, and leaf lowering. Literal typing is contextual and range-checked. This costs a few short names and removes signed-overflow, shift, and conversion disagreements between C, the assembler, and SSA.

Layouts and checkpoint declarations are data, not executable helpers. A family must have one total default path. Conservative effects are allowed. An undeclared reachable effect, missing result disposition, missing exit, or claimed fact that no predecessor proves is an error.

### Operation dictionary

This resolves the pseudocode names in v1. **P** is a language primitive. **M** is a typed macro expanded in every consumer. **R** is a trusted component row with a checked signature, effect declaration, and evidence. `ok(T)/err(Err)` describes tagged outcomes. Braces on a type denote a required or established fact. `K` in this table means a finite schema instantiation. The executable files use concrete types.

| Kind | Operation and typed signature | Meaning |
|---|---|---|
| P | `use(Src) → B`, `dup(B) → O`, `move(InOut<O>) → O`, `acquire(Src) → O` | Read, acquire a separate owner, take a uniquely named owner and vacate its home, or take a dying source/duplicate a survivor. `dup` requires presence. A nullable variant is a separate declared operation. |
| P | `publish(bindings, debts, Point) → Unit` | Parallel publication with ordered retirement. `old(Dst)`, `spent(Src)`, `roots(Anchors)`, and `owned(O)` are linear operands of this statement. They are not readable values. |
| P | `guard(Bool)`, `branch(Bool,L,L)`, `goto L(args)`, status `call`, bounded `each/advance`, `exit tag(args)` | These are exactly the control constructs above. V1’s `pass`, `finish`, and `case` denote a success edge, an ordinary label, and a tagged status edge. `error(z)` becomes an explicit `err` edge. |
| M | `anchor_for_call(Span<Src>) → Anchors`, `arg(Anchors,Size) → B` | Capture evaluated arguments in order. A writable caller local needs an independent anchor. A proven stable home can supply one. For statically sized ADD the anchors fit in fixed scratch. A variable-sized adapter that must allocate has a separate fallible reservation. |
| M | `retire_failed_call(Anchors,Span<Src>,Err) → Err`, `retire_abandoned(Anchors,Route) → Route` | Preserve the pending exception while closing the specified owners. On a redirect, discharge only the anchors still owned by the abandoned action, then resolve the current route. |
| M | `adapt_before_use(Fb<K>, operands…) → run()/changed(Ticket<K>)` | Per-family policy expansion. `again(Ticket<K>)` consumes proof of a published, different executable plan. Details in §2. |
| R | `inline_int(B) → Bool`, `both_inline_int(B,B) → Bool`, `int_scaled(B{inline_int}) → Scaled51`, `unscale(Scaled51) → I51`, `unbox_i51(B{inline_int}) → I51` | V1’s `InlineInt` is the first predicate. Pair admission uses the existing `PyValue_BothInlineInt` conjunction row. |
| R | `add_checked_scaled(Scaled51,Scaled51) → fits(Scaled51)/overflow()`, `sum_i51_wide(I51,I51) → I64`, `from_int_scaled(Scaled51) → O{inline_int}`, `box_i64(I64{outside_i51}) → ok(O{exact_int})/err(Err)` | Replaces v1’s underspecified `checked_add_i64` and `+`. A decoded sum of two int51s fits in signed 64 bits. The overflow edge proves that this particular boxing row is applicable. |
| R, M | `value_binary_op(Enum<Numeric>,B,B) → ok(O)/err(Err)` **R**. `binary_at_origin(Enum<Numeric>,Anchors) → ok(O)/err(Err)/redirected(Route)` **M** | The macro records the origin and control stamp, invokes the existing helper once, and checks the frame on return. If a redirect wins, it also discharges any uncommitted result. The row retains reflected-operation and `NotImplemented` semantics. |
| R | `is_object(B) → Bool`, `as_object(B{object}) → Obj`, `present(Maybe<B>) → Bool`, `absent(Maybe<B>) → Bool`, `is_null(Maybe<B>) → Bool` | These spell v1’s `guard_object`, `guard_present`, and `guard_null`. Refinement is tied to the tested value. |
| R | `admitted_instance_slot(Fb<Attr>,Name) → hit(Recipe<Attr>)/absent()`, `instance_version(Obj,Recipe<Attr>) → Bool`, `valid_inline_values(Obj) → Bool`, `borrow_value_slot(Obj,Recipe<Attr>) → Maybe<B>` | `instance_version` implements `guard_current_nonzero_type_version`. `valid_inline_values` implements `guard_managed_inline_values_valid`. The slot reader requires the dominating type/name/recipe and storage proofs. An arbitrary integer offset is insufficient. |
| R | `PythonGetAttr(B,Name) → ok(O)/err(Err)`, `nonzero_type_version(Obj) → hit(U32)/absent()`, `live_shape_rows(Fb<Attr>,Name) → hit(Rows<Attr>)/absent()`, `current_shape(Obj) → U32` | Generic `_PyValue_GetAttr` and the read-only poly probe. Row kind, version, shape, and offset are read-only recipe projections. The poly macro tests them before dup. |
| R | `default_eval(Thread) → Bool`, `plain_function(B,U32) → hit(Fun)/absent()`, `exact_binding(Fun,Maybe<Src>,Args) → Bool`, `frame_ready_space(Thread,Code) → Bool`, `recursion_budget(Thread) → Bool` | V1’s five exact-call guards. Include standard eval/vectorcall, current code/version, argument shape, prepared code, capacity, and recursion. A bound method additionally guards absent explicit self and callback-free expansion. |
| R, M | `reserve_exact_frame(Thread,Code) → ok(NewFrame)/err(Err)` **R**. `bind_exact(NewFrame,Src,Maybe<Src>,Args) → Unit`, `save_continuation(Frame,Site,Head,Dst) → Unit`, `link_child(NewFrame,Frame) → Frame` **M**. `start_frame(Frame) → ok()/err(Err)` **M** | `link_child` is the publishing portion of v1’s `link_and_enter`. `exit enter(frame,head)` transfers control. Binding commits once. No callback-capable close can expose a partially initialized child. First-entry preparation that can reenter precedes the unchecked reservation or uses a slow continuation with revalidation. |
| R | `exact_list(B) → Bool`, `index_from_getiter(IterPair) → hit(Size)/absent()`, `current_length(Obj{list}) → Size`, `borrow_published_list_element(Obj{list},Size{in_bounds}) → B`, `next_tagged_index(Size{incrementable}) → Cursor` | The old `guard_exact_list`, `guard_index_from_getiter`, and index arithmetic. V1’s `unsigned` comparison becomes a declared size/index comparison. A tagged integer alone does not prove its producer or bounds. |
| M | `complete_for(Frame,IterPair,Head,O) → branch(Head)/err(Err)/redirected(Route)`, `complete_send(Route,O) → delivered(Head)/returned(Route)/err(Err)/redirected(Route)` | Expand the declared consumer and PRE/POST observation cases. A folded return is a real return. The completion value is owned on entry to the macro and accounted for on every outcome. |
| R | `generator_or_coro(B) → hit(Gen)/absent()`, `created_accepts(Gen,B) → Bool`, `claimable(Gen) → Bool`, `claim(Gen) → ok(Claim)/err(Err)` | V1’s generator/coroutine, created-send, and claim guards. `claim` changes state. A guard failure after a successful claim is illegal. |
| M | `save_send_continuation(Frame,Site,Head,Dst,Head,Dst) → Unit`, `link_generator_and_exception_state(Claim,Frame) → Frame`, `publish_current_and_account_recursion(Frame) → Unit`, `install_resume_input(Frame,O) → ok()/err(Err)` | Name both yielded and completed destinations. Install the current frame and exception chain before any callback on resume. An error after entry belongs to the child. |
| M | `publish_origin(Frame,Site) → Unit`, `pin_departing_after_last_guard(Activation,Frame) → Pin`, `acquire_heap_safe(Src) → O` | Record the source PC, install the new execution pin before releasing the old one, and obtain a result that can survive frame retirement. Pin replacement propagates escape and rechecks the current control state. |
| M | `unlink_and_leave_recursion(Frame) → Pair<DeadFrame,Frame>`, `restore_generator_exception_state_if_needed(DeadFrame,Frame) → Unit`, `retire_frame_holding_result(DeadFrame,O) → O` | Unlink before releasing frame-owned values. Keep an independent result root throughout retirement. The pair’s second component can be the evaluator entry frame. |
| M | `current_return_continuation(Frame) → Route`, `complete_return(Route,O) → ordinary(Frame,Head)/external(O)/redirected(Route)/err(Err)` | Re-read the caller and recipient **after** retirement. The error case includes the consumer checkpoint for a failed generator completion. |
| P | `next()`, `branch(Head)`, `jump(Head)`, `miss()`, `again(Ticket<K>)`, `raise(Point,Err)`, `enter(Frame,Head)`, `return(Frame,Head)`, `yield(Route)`, `exit(O)`, `redirect(Route)` | Arm exit constructors. A route carries its frame and target. A pending exception or result is transferred exactly once. |

A macro can use a narrow C row for an indivisible runtime action such as claiming a generator. A macro that binds a child, publishes its continuation, calls into Python, and decides whether to retry must expose those steps to the graph. Declaring the whole old handler as a component would defeat both the verifier and the tracer.

### ADD and instance-value GetAttr in final syntax

These are the two required ADD arms. The additional numeric and text admissions in Appendix A use the same family and layout. `Add.Int` has a distinct error checkpoint because a failed box leaves both evaluated inputs in their homes. `Add.Generic` has already executed Python when its helper returns an error.

```text
family Add(dst:Dst, x:Src, y:Src, fb:Fb<Binary>) layout ABCF;

point Add.BoxFailed(current,site,pending) keep(x,y) spent() defs() held();
point Add.OpFailed(current,site,pending) keep() spent(y,x) defs() held();
point Add.Done(current,next_head,clear) keep() spent(y,x) defs(dst) held();

arm Add.Int inline
  effects(writes,allocates,raises,releases) feedback() exits(next,miss,raise)
  ensures(next.exact_int(dst))
{
  entry():
    let l:B = use(x);
    let r:B = use(y);
    guard both_inline_int(l,r) else miss;
    let a:Scaled51 = int_scaled(l);
    let b:Scaled51 = int_scaled(r);
    call add_checked_scaled(a,b) {
      fits -> Fits();
      overflow -> Overflow();
    }
  Fits(s:Scaled51):
    goto Finish(from_int_scaled(s));
  Overflow():
    let full:I64 = sum_i51_wide(unbox_i51(l),unbox_i51(r));
    call box_i64(full) {
      ok -> Finish();
      err -> Failed();
    }
  Failed(e:Err):
    exit raise(Add.BoxFailed,e);
  Finish(z:O):
    publish [dst=z] retire [spent(y),spent(x),old(dst)] at Add.Done;
    exit next();
}

arm Add.Generic shared
  effects(runs_python,raises)
  feedback(read(entry),write(entry),write(handler))
  exits(next,again,raise,redirect)
{
  entry():
    call adapt_before_use_add(fb,x,y) {
      run -> Run();
      changed -> Retry();
    }
  Retry(t:Ticket<Binary>):
    exit again(t);
  Run():
    let a:Anchors = anchor_for_call([x,y]);
    call binary_at_origin(NB_ADD,a) {
      ok -> Result();
      err -> Failed();
      redirected -> Gone();
    }
  Result(z:O):
    publish [dst=z] retire [roots(a),spent(y),spent(x),old(dst)] at Add.Done;
    exit next();
  Failed(e:Err):
    let saved:Err = retire_failed_call(a,[y,x],e);
    exit raise(Add.OpFailed,saved);
  Gone(route:Route):
    exit redirect(retire_abandoned(a,route));
}
```

`retire_abandoned` is an action. Under the grammar’s nested-expression rule the last block is written in the executable file as follows. This also makes the revalidation after releasing anchors visible.

```text
  Gone(route:Route):
    let current_route:Route = retire_abandoned(a,route);
    exit redirect(current_route);
```

For attribute access, a recipe is evidence for **this site’s name** and an admitted lookup path. A type version and an offset from unrelated observations cannot be combined. The duplicate precedes any release of the receiver.

```text
family GetAttr(dst:Dst, obj:Src, name:Name, fb:Fb<Attr>) layout ABFN;
point GetAttr.Done(current,next_head,clear) keep() spent(obj) defs(dst) held();

arm GetAttr.Instance inline
  effects(reads,writes,releases)
  feedback(read(entry.kind),read(entry.type_version),read(entry.offset))
  exits(next,miss)
{
  entry():
    call admitted_instance_slot(fb,name) {
      hit -> Check();
      absent -> Miss();
    }
  Check(r:Recipe<Attr>):
    let v:B = use(obj);
    guard is_object(v) else miss;
    let o:Obj = as_object(v);
    guard instance_version(o,r) else miss;
    guard valid_inline_values(o) else miss;
    let item:Maybe<B> = borrow_value_slot(o,r);
    guard present(item) else miss;
    let result:O = dup(item);
    publish [dst=result] retire [spent(obj),old(dst)] at GetAttr.Done;
    exit next();
  Miss():
    exit miss();
}
```

The slot can be vacant on a valid instance. That is a miss and goes through normal attribute lookup, including descriptor precedence and missing-attribute behavior. The pilot’s unchecked slot read is admitted on the selected GIL build. Another build needs an independently declared stable-read protocol or uses generic lookup. [Current instance load][instance] · [Slot and validity rows][slotrows]

### What the interpreter actually saves on integer ADD

Here is the current generated C data path, condensed only by removing declarations, statistics, assertions, `(void)` casts, and the common dispatch prologue. `MISS()` below abbreviates the existing miss-stat update and `IR_DEOPT_BINARY_OP(IR_D(ir_word))`. Both original guard tests have that failure path. [Current generated case][oldadd]

```c
value = IR_VIEW(IR_C(ir_word));
if (!PyValue_IsInlineInt(value)) { MISS(); }
left = IR_VIEW(IR_B(ir_word));
if (!PyValue_IsInlineInt(left)) { MISS(); }
right = value;
int64_t a = _PyValue_IntScaled(left);
int64_t b = _PyValue_IntScaled(right);
if (__builtin_add_overflow(a, b, &sum)) {
    res = _PyValue_BoxInt64(PyValue_InlineInt64(left)
                            + PyValue_InlineInt64(right));
} else {
    res = _PyValue_FromIntScaled(sum);
}
if (PyValue_IsNull(res)) { PROCESS_ERROR_AT(RAISE, 0, this_instr); }
l = left;
r = right;
ir_own_B = IR_DETACH(ir_word, B, l);
ir_own_C = IR_DETACH(ir_word, C, r);
value = r;
value = l;
IR_STORE_DST(ir_word, A, res);
PyValue_XCLOSE_SPECIALIZED(ir_own_C, _PyLong_ExactDealloc);
PyValue_XCLOSE_SPECIALIZED(ir_own_B, _PyLong_ExactDealloc);
PyValue_XCLOSE(ir_old);
DISPATCH();
```

Proposed generated C for the same compact head follows. `miss_add` enters the miss adapter with original homes. `ADD_BOX_FAILED` is a generated checkpoint index. In an assert build the generator retains the home and vacancy checks.

```c
PyValue *const px = &frame->base[IR_B(ir_word)];
PyValue *const py = &frame->base[IR_C(ir_word)];
PyValue *const pd = &frame->base[IR_A(ir_word)];
PyValueBorrow x = PyValue_Borrow(*px);
PyValueBorrow y = PyValue_Borrow(*py);
if (!PyValue_BothInlineInt(x, y)) {
    goto miss_add;
}
int64_t a = _PyValue_IntScaled(x);
int64_t b = _PyValue_IntScaled(y);
int64_t sum;
PyValue result;
if (__builtin_add_overflow(a, b, &sum)) {
    goto add_overflow;
}
result = _PyValue_FromIntScaled(sum);

add_commit:
if (!(ir_word & IR_F_LOCAL_B)) {
    _PyFrame_WriteHome(frame, px, PyValue_NULL);
}
if (!(ir_word & IR_F_LOCAL_C)) {
    _PyFrame_WriteHome(frame, py, PyValue_NULL);
}
PyValue old = *pd;
_PyFrame_WriteHome(frame, pd, result);
_PyValue_CheckHome(*pd);
PyValue_XCLOSE(old);
DISPATCH();

add_overflow:
result = _PyValue_BoxInt64(PyValue_InlineInt64(x) + PyValue_InlineInt64(y));
if (PyValue_IsNull(result)) {
    PROCESS_ERROR_AT(RAISE, ADD_BOX_FAILED, this_instr);
}
goto add_commit;
```

Both input clear operations are safe even if the two bindings name the same temporary. The pair proof says both words are inline and neither owns a count. The displaced destination is read **after** clearing spent sources. It is installed **before** its old owner can run a finalizer. Keeping the three home addresses across the cold box is justified by that particular helper’s no-Python/no-finalizer contract. A general allocating or reentrant row does not grant that permission. [Boxing row][box] · [Home writer][home] · [Existing detach and destination macros][detach]

| Hot, fitting add, two surviving input locals and a fresh temporary destination | Today | Proposed |
|---|---:|---:|
| Executed generated-C source statements by the convention below | 20 | 15 |
| Distinct operand fields needed on success | 3 — A, B, C | 3 — A, B, C |
| Release-C evaluations of operand-index extraction before compiler CSE | 4 — B, C, A, A | 3 — B, C, A |
| Type-test branches | 2 | 1 |
| Result-NULL branches on the fitting path | 1 | 0 |
| Calls to specialized input-close macros in the source | 2 | 0 |

Count one executed initializer, assignment, branch test, or top-level macro call as one statement. Exclude dispatch, common prologue, stats, debug checks, declarations without work, and setup of bookkeeping locals. Count the old alias assignments even though GCC can erase them. For the old count the ledger is two reads, two guards, five aliases, two scales, overflow test, encode, NULL test, two detaches, destination publication, and three closes. For the new count it is three addresses, two reads, one guard, two scales, overflow test, encode, two home-class tests, old load, write, and close. A dying input adds an old-path field extraction inside `IR_DETACH`; the proposed C has already formed its address. D is consulted on the miss/adaptation route. These are source and decode counts, not retired CPU instructions.

### What the template generator receives

I ran the checkout’s `Builder(record).build()` and `simplify()` in memory for `BINARY_OP_ADD_INT`. The input is five old uop instances. The builder made **19 blocks and 25 operations** before simplification. The simplified reachable graph has **8 blocks and 17 operations**, excluding terminators. Its hot and cold paths are below. The final column is the register-native leaf input I would require. Names are normalized for comparison.

| Path | Current simplified leaf graph feeding the template emitter | Proposed leaf graph from `Add.Int` |
|---|---|---|
| Admission | `view B, view C, prim BothInlineInt, br hit/miss` | `read x, read y, prim BothInlineInt, br hit/miss` |
| Arithmetic | `IntScaled x, IntScaled y, add_ovf, br overflow/fit` | Same four operations |
| Fit | `FromIntScaled sum → commit(result)` | Same |
| Overflow | `InlineInt64 x, InlineInt64 y, add, call BoxInt64, IsNull, br raise/commit` | Same operations, with `BoxFailed` attached to the raising edge |
| Commit | `detach B if temp; detach C if temp; load A if local; store A; XCLOSE old if local; end` | `publish` lowered to the same home-class-conditional operations |
| Failure | `deopt` to the word’s slow form, or `raise` | `miss` before action, or `raise(BoxFailed)` after the failed box |

For the same two-local/fresh-destination shape, both emitters have **eight hot data operations and two conditional transfers**. They bind three field indexes at compile time and execute **zero native operand-index decodes** on a hit. The present emitted code already has two loads, one combined predicate and branch, two scales, add and overflow branch, retag, and store. Expanding the current x86 lowerings gives 17 assembler instruction requests for that body and those registers, excluding word-entry and continuation glue. A local destination adds the displaced-owner path. A dying temporary adds a clear. G1c should measure those cases separately before changing register assignment or a value primitive. The pilot should preserve the present fast body while removing the work of reconstructing it from five stack fragments. [Current template][template] · [Template builder][builder] · [x86 primitives][lower] · [Existing guard/boxing regression test][addtest]

## 2. Specialization and adaptation

Give **family definitions** ownership of arm admission, nomination priority, retry policy, and eligibility for a poly arm. Give **component rows** ownership of atomic counter operations, type/descriptor inspection, and coherent cache storage. The executing code object owns the counters and recipes. A specializer in C may answer “this name on this type has a valid instance-slot proof.” It must not choose an arbitrary handler number or rewrite the instruction behind the graph’s back.

| Decision or operation | Owner | Contract |
|---|---|---|
| Normal dispatch | Generated family/arm table | The compact head selects a compatible arm. A wide head selects through its executing side entry. The immutable family and operands remain recoverable. |
| Admission on every execution | Arm’s guard prefix | Check current inputs and versions. A successful nomination can go stale before its first hit. |
| When to sample | Declared `adapt_before_use_F` macro | Shared/no-feedback and OFF sites go straight to the operation. Otherwise call `tick`. The policy declares its reads and writes. |
| Counter arithmetic | `tick(Fb<K>) → quiet()/due()`, `backoff(Fb<K>) → Unit`, cooldown on successful install | Rows around the existing adaptive-counter representation. `tick` handles the shared-header case without writing it. Keep the present cooldown/backoff constants for the control. |
| What to nominate | Declared per-family macro | Ordered tests over typed evidence. ADD can test the pair directly. GetAttr requests classification of the current type, immutable name, lookup hooks, descriptor precedence, version, storage kind and slot. Call requests callable convention, version, and binding shape. |
| Inspect Python runtime state | Narrow C rows | Return absent or a typed recipe. A recipe carries all correlated guards and payload together. Nomination that cannot obtain evidence without reentry can decline. If reentrant sampling is retained, anchor evaluated arguments, revalidate, and continue this execution through the generic body. |
| Record and choose poly | Declared cache policy over typed rows | Deduplicate the same full key and refresh its payload. Preserve the first observation. The second compatible observation can select poly. Four distinct rows fit. A fifth selects MEGA/backoff. Initially retain the current distinct-type-version rule for shape and method PICs. |
| Publish an arm | `install(Fb<K>,Candidate<K>) → changed(Ticket<K>)/same()/blocked()` | Build or reserve optional storage first. In a callback-free interval validate the site generation, publish the whole recipe/table, then publish the executable selection. Preserve word length, operand bits, width, and observer wrapping. Failure to allocate optional cache storage leaves the Python operation executable. |
| Native miss | Generated miss adapter | Record the miss and enter the generic **operation body once** on original inputs. Sampling may update future execution. Do not bounce from a compiled miss back to the same failing native plan. An incompatible executed plan can mark a unit stale. Hits on different rows of one admitted PIC do not. |

For example, this is a two-arm pilot policy in the definition language. `int_candidate` constructs a candidate for `Add.Int` in the DSL. Its constructor requires the pair fact on the `Chosen` edge. It does not take a handler number from C. When P2 admits the additional ADD arms, `Other` runs their nomination tests before backoff.

```text
macro adapt_before_use_add(f:Fb<Binary>, x:Src, y:Src)
  cases(run(),changed(t:Ticket<Binary>))
  effects(reads,writes)
  feedback(read(entry),write(entry),write(handler))
  exits(run,changed)
{
  entry():
    call tick(f) {
      quiet -> Run();
      due -> Sample();
    }
  Sample():
    branch both_inline_int(use(x),use(y)) -> Chosen(), Other();
  Chosen():
    let c:Candidate<Binary> = int_candidate(x,y);
    call install(f,c) {
      changed -> Changed();
      same -> Run();
      blocked -> Run();
    }
  Other():
    let _:Unit = backoff(f);
    exit run();
  Changed(t:Ticket<Binary>):
    exit changed(t);
  Run():
    exit run();
}
```

`again` requires a fresh ticket, original operand identities, and no outstanding owner or language effect. Counter progress alone cannot mint a ticket. A stale/blocked install consumes the nomination attempt and sets a cooldown or leaves adaptation disabled, so repeated entry cannot spin. A monitored head maintains its underlying selection separately. A compiled site binds a version **together with** its payload or reloads a coherent live row. Free-threaded publication needs a versioned snapshot or equivalent synchronization and safe payload lifetime before enabling these arms there.

This changes a real ordering in the checkout. `specialize_handler()` currently calls `set_opcode` before resetting the counter and calling `fb_record`. The latter stores observations; separate C predicates select shape and method poly. Also, current CALL and CALL_KW specializers can refresh call state and **continue** after sampling. Translating every `_SPECIALIZE_*` as an unconditional retry would be wrong. [Current installation and poly selection][spec] · [Current counter/PIC storage][fb] · [Current CALL_KW adaptation][callkw] · [Completion ABI, including AGAIN][abi]

## 3. Pilot decoder and lowering

Keep the stack **decoder**. Remove stack positions from the definition of the operation it emits.

| Location | Pilot change |
|---|---|
| `flowgraph.c`, `decode_instructions`, `jump_target_unit`, `decode_locations`, `decode_exceptions` | Keep public opcode validation, `EXTENDED_ARG` accumulation, cache skipping, source unit, public targets, protected intervals, handler depth, and `lasti`. A source unit remains an origin even if it emits no head. [Public decoder][decode] |
| `split_instruction`, `decode_code`, `cfg_to_regcfg`, `_PyCfg_LowerCode` | Split the four public local superinstructions in order and preserve both operations’ origin. Normalize borrowed loads as candidates for stable-source use. Consume `NOT_TAKEN` as edge metadata. Map `BINARY_OP/NB_ADD` to `Add`, the method bit of `LOAD_ATTR` to `GetMethod` versus `GetAttr`, calls to callable/optional self/ordered arguments, and return to its named source. Decode a branch’s sense, keep-on-edge rule, and interrupt behavior here. [Splitting and translation][split] · [CFG handoff][handoff] |
| `regcfg.c`, `_PyRegCfg_FormatOf`, `mark_blocks`, `convert_instr`, `convert`, `canonicalise`, `canonicalise_below` | Add a source-op translation table and a translator stack of **value identities**. Pilot rules issue named register operations and edge arguments. Stop deriving their interfaces from `_PyOpcode_num_popped/pushed`, window size, or “through” stack positions. At a merge, assign block parameters for values live on that edge. Retain the old converter only for disjoint, unported families during migration. [Current converter][convert] |
| `regcfg.c`, `transfer`, liveness, `_PyRegCfg_OpSlots`, `compact_op`, `_PyRegCfg_Compact`, `_PyRegCfg_Verify` | Visit a family’s explicit uses, definitions, live-throughs, optional operands, and outcome-specific deaths. Allocate homes after coalescing compatible edge assignments. Check handler ingress as well as normal edges. Derive the empty-return proof from vacant scratch homes. Stack depth one is no longer the proof. [Liveness and slots][liveness] · [Physical verifier][physical] |
| `regcfg_passes.c`, `SwapRename`, `CopyProp`, `StoreElim`, `UnpackLocals`, `PopTop`, `Discard` | Replace the region-specific swap cleanup for translated code with symbolic renaming and edge assignment. Keep the useful producer-to-local and direct-unpack folds, expressed against named outputs. Fold discarded results into a producer’s publication only if the origin, release, and observer phases allow it. [Swap pass][swap] · [Unpack and discard passes][unpackpass] |
| `TruthFold`, `FuseJump`, `KwNames`, `ElideNull`, `CallRoles`, constant-slot assignment | Emit `BranchTruth`/`BranchNone` or a two-origin `TestBranch` when legal. Treat virtual NULL, keyword names, and self presence as typed call roles. Keep hidden constant homes and source snapshots where needed. Preserve proof that a writable local still denotes the evaluated argument after intervening code. [Pass order][passes] · [Call-role proof][roles] |
| `ir_assemble.c`, `choose_handler[s]`, `lower_completions`, `place_generator_boundaries`, `place_entries` | Choose immutable family and initial arm. Bind both continuation and destination explicitly. For mixed pilot/legacy calls use one completion ABI. Place optional feedback independently from required semantics such as an operation selector or attribute name. Preserve the old boundary adapter for an unported consumer. [Handler selection][choose] · [Completion lowering][completions] |
| `encode_operand_words`, `layout`, `encode`, `assemble_words`, `make_ir`, `_PyIR_Assemble` | Encode named operands into A–D, compact call payloads, or the wide descriptor. Resolve targets after edge blocks exist. Promote out-of-range heads monotonically, then freeze PCs. Emit EH, source maps, PC maps, native entry locations, and return recipients from the final layout. No payload is an entry. [Operand payloads][payload] · [Layout and encoding][layout] · [Source and PC maps][srcmap] |

**Placement of shuffle logic.** `SWAP(k)` permutes identities in the compatibility translator’s vector. `COPY(k)` adds a use of an identity. It can be a rename while a stable owner covers both uses. If the values must survive independently, acquire a second owner before the first can disappear or be rebound. `POP_TOP` removes a use and records a retirement obligation at that source phase. It can vanish when nothing is owed, be discharged by the producer or outgoing edge, or require a standalone publication at an observable boundary. An object whose last close may execute Python cannot simply be forgotten.

Put parallel assignment resolution **after home allocation in `regcfg.c` and before assembler layout**. Remove identities, schedule acyclic transfers, and break a cycle with one rooted scratch owner. Take all values needed for the parallel update before losing any source. Publish the resulting homes before callback-capable closes. Split critical normal edges. For an exceptional edge, put its assignment in the unwinder’s ingress recipe for the raising checkpoint. An ordinary edge block appended after a possibly raising instruction cannot implement that edge. Keep a distinct legal-entry map for debugger jumps, with the saved comprehension binding and phase. An erased shuffle’s source location may require an event or a `Nop`, even when it requires no `Move`.

**Executed `Move` estimates.** I reran the two v1 functions with `-I -B -X jit=off` after 200 warm calls and inspected their adaptive register words. The release executable identifies itself as `d344a3dcd5a`, GCC 15.2.0, on a dirty build branch. The source and generated-code comparisons in this addendum are at `dd7e484be64f`.

| Function | Observed old heads | Predicted new executed `Move` heads |
|---|---|---:|
| `def comp(xs): return [x for x in xs]` | Five static `SWAP`s. Two execute before the loop and one after it on every normal call. The other two are on the exceptional restoration path. No shuffle executes per item. | **0 per normal call, 0 per iteration.** Give the saved outer binding, list, receiver, cursor, and item their eventual homes from the start. `SaveLocal` and restoration still transfer ownership once each. An EH ingress recipe can use the same homes. |
| `def unpack(p): a,b=p; return b` | `RESUME`, `LOAD_FAST_BORROW`, `UNPACK_SEQUENCE_TUPLE`, `RETURN_VALUE_EMPTY`. The existing unpack already names the two locals. | **0 per normal call.** Read `p` directly and publish the two fresh local destinations. The generic iterator path may need anchors and staging, which are real semantic work. |

These predictions count executable `Move` heads, not register copies inside `SaveLocal`, a helper, or a publication. They apply to these functions. For `a, obj.x = p`, the attribute store remains ordered and fallible. For a merge with irreconcilable home assignments, emit the residual move and count it. [Existing unpack interface][unpackold] · [Release disassembler and raw-word interface][dis]

## 4. Compressed migration

Use an integration train with disjoint family ownership. A port lands its semantic definition, all entry routes, its observer bindings, and deletion of its old family provider in **one unit**. An old shared fragment stays only while an unported family references it. For example, remove `NB_ADD` from the old numerical routing when `Add` lands. The old generic binary fragment can still serve other selectors until their unit lands. A build-time uniqueness check rejects two providers of any `(immutable family, public lowering, build mode)`.

The pilot uses normal first-call lowering, interpreter dispatch, and baseline entry. A switch that sends an inconvenient miss back to the old implementation of the same family does not pass the gate. Keep the train private until the freeze gate passes. The coordinator can publish the qualified train as a single integration merge if mainline must never expose a mixed-source revision.

| Stage | Work and staffing | Agent-hours | Elapsed working hours with stated parallelism |
|---|---|---:|---:|
| Foundation | Three Opus units in parallel. One owns grammar, typed CFG and diagnostics. One owns ownership/checkpoints and product interfaces. One owns decoder, home assignment, layout and mixed-family ABI. Agree on a small executable schema fixture first. | 12–18 | 4–6 |
| Pilot P1–P5 | Five Sonnet family units at 1–3 hours each. Three Opus units at 3–6 hours each integrate the graph backends, adaptation/observation, and real compiled call/return routing. Prove the two ADD arms, instance and poly lookup, one exact Python call with its real return, and the needed home/branch operations first. Complete the families’ other admitted routes before deleting their old providers. | 14–33 | 4–8 |
| Language freeze | Two Opus reviews at 2–3 hours, one adversarial verifier review and one end-to-end product/ABI review. Freeze syntax, graph serialization, row/result ABI, and diagnostic fixtures. | 4–6 | 2–3 |
| Wave 1 | Nine independent Sonnet batches, 1A–1I. Two Opus reviewers cover callback-sensitive item/attribute access, fused branches, and integration. Merge complete family units continuously. | 13–35 | 3–6 |
| Wave 2 | Six independent Sonnet batches, 2A–2F. Three Opus reviewers cover calls, suspension/completion, and observers/EH. Their common continuation and checkpoint ABI was frozen in the pilot. | 12–30 | 3–7 |
| Delete and qualify | One Opus integration unit at 3–5 hours and one Sonnet deletion/census unit at 1–2 hours. Run the last provider check before removing the migration bridge. | 4–7 | 3–5 |
| **Total** | Peak staffing is nine writers and two reviewers in Wave 1. | **59–129** | **19–35** |

The critical path is graph/owner ABI → all pilot products plus physical frame switch → freeze → joined family waves → deletion. With a qualified pilot already integrated, the mixed-source tail is approximately **11–21 working hours**. These are agent-work estimates. Build queues, quiet benchmark time, and investigation of a newly discovered correctness failure add wall time. Do not serialize nine independent ports on a single generated-file edit. Generate aggregate outputs at integration from the family files.

**Freeze gate.** Require all of the following on the same candidate.

- Interpret the pilot CFG through generated C, execute the generated function forms through their real ABI, execute an inline template, and force a shared-stub hit, miss, raise, and `again`. Enter an exact Python callee and return to a compiled caller by the resident switch. Exercise a mixed-tier caller and a callee starting before `RESUME`. Check departing execution pins, invalidation during release, and `RESUME_CHECK`.
- Expand the **same arms** to tracer SSA. Execute the pilot SSA in a test evaluator against the C path, with guard failure, overflow boxing failure, an effectful reflected add, receiver mutation, and a call barrier. Check frame reconstruction and effect order on side exits. This qualifies SSA expansion; production trace recording and optimization remain subsequent work.
- Refuse a missing status, a guard after action, a stale interior borrow, a mismatched version/offset, a duplicate move, a missing owner on one join, a branch into a payload, a retry without a ticket, and a control transfer whose current frame or owner map is unspecified.
- Run `off`, `on`, and `always`, release and assert. Exercise input/destination aliasing, weakrefs/finalizers, writable `f_locals`, descriptor shadowing, PEP 523, recursion, EH, `sys.settrace`, `sys.monitoring`, `f_lineno`, lost cache storage, and artificially forced wide operands. Compare public bytecode, source offsets, traceback locations, semantic results, and home/owner states. Test PRE and POST observer failures separately.
- Record first-call cost, interpreter time, baseline time, generated hot/cold size, dynamic move counts, native instructions, and PIC misses with the established GCC/computed-goto control. Run the project’s correctness gates at every unit and its quiet all-29 selection gates at pilot, wave joins, and deletion. Use a matching frozen executable for differential qualification. The supplied release binary is suitable for the labeled probes above.

At deletion remove the stack-body-to-IR emitter, stack-to-owner simulation, C-text leaf recognizer for handler semantics, exchange/replacement machinery, window skeletons, inferred transfer classifiers, and legacy semantic routing. Retain public bytecode numbering, compatibility stack effects needed by the compiler/decoder, the public disassembler, typed C components, the leaf machine lowerings, and ordinary first-call stack decoding. G1c becomes a measured improvement to `Add.Int`; G2 is `GetAttr`/`GetMethod` recipe selection and probing. G3a pins, switch glue, and entry checks are pilot infrastructure. G4 can build on proven `enter`/`return` graphs. G6 forwards only a copy of a value already written through to its owning home, invalidated at independent entry, escape, redirect, or transfer. [G3 frame-switch contract][g3] · [Project validation][validation]

## Appendix A. New instruction set

A row below defines a semantic family. A mode in parentheses is an immutable selector or operand shape. It may get its own compact handler ID. A named arm is a graph admission. Macros shared by several rows have no dispatch entry.

**Operand notation.** `d` is an owning destination, `s/x/y/o/v/k` are source homes, `l` is a visible local, `c` a cell/free home, `n` a name index, `k₀` a constant index, `i` a checked immediate/enum, `t` a target head, `f` typed optional feedback, `ss/dd` ordered source/destination vectors, `p` an iterator receiver/cursor pair, `r` a continuation, and `a` an ordered argument descriptor with explicit self/keyword roles. `?` means a typed absence is permitted. A generic arm admits every input allowed by the family signature, including cases that must raise a Python exception.

**Exit notation.** `N` next, `B` conditional target, `J` jump, `E` enter child, `T` return to caller, `Y` suspend, `X` leave evaluator, `R` raise, `D` redirect. `*` adds guard `miss` and policy `again` where the family is adaptive. Outcomes of suspended callees are delivered by their continuation. Observer wrappers add their declared `R/D` to the wrapped edge. Every row inherits the checked home/read/publication operations from §1. The component column names the semantic dependencies in addition to that common machinery. Names in monospace are existing rows or C entry points where available. Named macros and new typed rows are identified by their function. The implementation emits the exact transitive dependency manifest, including field reads and indirect slot protocols, from each checked arm.

### Values, control, arithmetic, and lookup

| Family and operands | Arms and admission | Exits | Components | Batch |
|---|---|---|---|---|
| `Nop(origin)` | Origin or legal-entry boundary requires an executable head | N | Source/observer map | P1 |
| `Const(d, kind, k₀/i)` | Pool for a code constant, Small for an immediate, Common for an interpreter root. Inline-int/float proofs refine Pool | N | `_PyValue_CodeConstant`, `_PyValue_CommonConstant`, `PyValue_FromInlineInt`, `dup` | P1 |
| `LocalRead(d,l)` | Bound with a producer proof, Checked otherwise | N,R | `_PyValue_BoundLocal`, unbound-name formatting, `dup` | P1 |
| `LocalWrite(l,s)` | Store present value, or restore a saved binding including unbound | N | `acquire`, home exchange, displaced-owner close | P1 |
| `SaveLocal(d,l)` | Comprehension or equivalent scope saves and vacates a visible binding | N | Move nullable binding, publish vacant local | P1 |
| `LocalDelete(l)` | Bound deletes, unbound raises | N,R | Home take, unbound error, close | P1 |
| `Move(dd,ss,retire)` | Residual edge assignment or separately observable retirement. Its layout can encode a single pair compactly | N | Parallel publication, rooted cycle break, typed duplication | P1 |
| `Add(d,x,y,f)` | Int for two inline ints. Float for the admitted inline float/number pair. Text for exact strings. Extended for a valid registered two-type action. Generic for Python addition | N,R,D,* | Integer rows in §1, `PyValue_FloatOperandValues`, `PyValue_FromDouble`, Unicode concat, `_PyBinaryOp_ExtendDescr` and typed action, `value_binary_op(NB_ADD)` | P2 |
| `Truth(d,s,invert,f)` | Bool, exact int including boxed, exact list, None, exact str, plain versioned type, admitted truth slot, generic. Invert applies after a successful truth test | N,R,* | `PyValue_IsNone`, `_PyLong_IsZero`, list/string length, `_PyValue_IsTrue`, `_PyValue_IsTrueSlow` | P5 |
| `Jump(t,pollMode)` | Direct. Interruptible backedge runs its poll. Delegating no-interrupt edge omits it | J; poll can R,D,X | Poll macro, static target, warmup hook | P5 |
| `BranchTruth(s,t,sense,keepMode,f)` | Same truth admissions as `Truth`. Keep mode preserves the evaluated owner only on the specified edge | N,B,R,* | `TruthTest`, edge reporting, conditional publication | P5 |
| `BranchNone(s,t,sense)` | Identity with `None`, either sense | N,B | `PyValue_IsNone`, selected-edge retirement | P5 |
| `Binary(d,x,y,op,f)` | Immutable ordinary/in-place selector. Int for supported multiply, subtract, floor divide, remainder, bitwise and shifts with range/divisor/count guards. Float for supported mixed numeric arithmetic and nonzero division. TextLocal for in-place concat with proven target identity. Extended for matched descriptor. Generic for every selector | N,R,* | Checked integer primitives, `_PyValue_MultiplyInt64`, `_PyValue_LshiftInt64`, float rows, `value_unicode_append`, typed extension action, `value_binary_op` | 1A |
| `Unary(d,s,op)` | Inline int/float negate where admitted, positive or invert by protocol, generic negate otherwise | N,R | `PyValue_FromInt64`, `PyValue_FromDoubleNotNaN`, `_PyValue_Negative`, `_PyValue_Positive`, `_PyValue_Invert` | 1A |
| `Compare(d,x,y,relation,resultMode,f)` | Two inline ints, two inline floats, exact strings for EQ/NE, generic rich comparison. Result mode requests rich result or fallible truth conversion | N,R,* | `_PyValue_RichCompare`, `_PyUnicode_Equal`, IEEE comparison, `_PyValue_IsTrue` | 1B |
| `Identity(d,x,y,sense)` | All PyValues, including inline identity rules | N | `PyValue_Is` | 1B |
| `Contains(d,item,container,sense,f)` | Admitted exact set/frozenset, dict representation, exact sequence with checked contains slot, or generic | N,R,* | `_PySet_Contains`, `_PyDict_ContainsValue`, `_Py_containsvaluefunc`, `_PyValue_Contains` | 1B |
| `TestBranch(x,y,test,t,sense,f)` | Identity, compare, or membership plus branch when both source origins and the intermediate result are recoverable. Each inherits its test’s admissions | N,B,R,* | The **same** `IdentityTest`, `CompareValues`, `ContainsTest` macros, then selected-edge publication | 1B |
| `GetAttr(d,o,n,f)` | Instance, Shape, PolyShape, Module, Hint, ValueSlot, PointerSlot, Class, NonDescriptor, GetSet, Property, Overridden, Generic. Detailed admissions below | N,E,R,D,* | §1 recipe readers, `_PyObject_InlineValues`, `_PyObject_ValueSlot`, `_PyValueGuarded_Load`, `_PyGetSet_ValueRead`, `_PyValue_GetAttr`, child binding/continuation | P3 |
| `GetMethod(dCallable,dSelf?,o,n,f)` | MethodValues, NoDict, LazyDict, ClassMethod, PolyMethod, ordinary attribute with absent self, Generic. Detailed admissions below | N,R,D,* | Unshadowed-method proof, guarded descriptor/classmethod load, `_PyClassMethod_GetFunc`, `_PyValue_GetMethod`, two-result publish | P3 |
| `AttrWrite(o,n,v?,mode,f)` | Delete uses the deletion protocol. Set has Instance, Shape, ShapeAdd, Hint, Slot, and Generic arms | N,R,* | `_PyValue_SetAttr`, `_PyObject_ValueSlot`, slot fill/exchange, shape transition and insertion-order rows, dict update, scoped lock | 1D |
| `SuperLookup(d,dSelf?,super,cls,self,n,mode,f)` | BuiltinAttr for exact builtin `super` and a type. BuiltinMethod additionally admits unbound-method delivery. Generic preserves invocation of a replaced `super` | N,R,* | `super_call`, `_PySuper_LookupValue`, `_PyValue_GetAttr`, call-observer adapter | 1D |
| `SpecialLookup(dFn,dSelf?,o,which)` | Type-directed special lookup for the specified protocol, including sync/async context entry and exit | N,R | Typed version of `IR_LOOKUP_SPECIAL`, `_PyEval_SpecialMethodCanSuggest`, formatted protocol error | 1D |

`Binary.op` covers add-in-place and both ordinary and in-place forms of subtraction, multiplication, matrix multiplication, true/floor division, modulo, power, both shifts, AND, OR, and XOR. Ordinary addition is `Add`. Subscription is `GetItem`. The generic selector is immutable even when two forms share a fast arithmetic macro.

**Attribute admissions.** These are separate guard/action cases within the two lookup families.

| Arm | Required evidence on entry |
|---|---|
| Instance | Admitted type/name instance slot, matching live nonzero type version, valid inline values, present value. |
| Shape / PolyShape | One matching version/shape/offset recipe, or one matching row among two to four compatible live recipes. Prove valid storage and presence before duplication. |
| Module | Exact admitted module, current dictionary keys version, in-range entry, present value. |
| Hint | Current managed dictionary and a live in-range entry whose key is the immutable name. |
| ValueSlot / PointerSlot | Current admitted member descriptor and type version, correct slot representation, present value or pointer. |
| Class | Current class version and live cached payload. Check metaclass version or exact builtin metaclass where nomination requires it. |
| NonDescriptorValues / NonDescriptorNoDict | Admitted class value is a non-descriptor. The instance cannot shadow it under the validated values layout or has no dictionary. |
| GetSet | Current protected descriptor and admitted getset reader. The action can call or raise. |
| Property / Overridden | Current versioned Python getter or `__getattribute__`, correct call shape, default evaluator and admissible frame entry. Save the attribute word’s destination and successor. |
| MethodValues / NoDict / LazyDict | Admitted method descriptor, matching live type version and respectively a valid unshadowed values layout, no dictionary, or still-unmaterialized dictionary. Acquire callable and self independently. |
| ClassMethod | Admitted class receiver/metaclass, live classmethod descriptor and underlying function. Publish the class as self. |
| PolyMethod | Two to four compatible method recipes. Match a current nonzero receiver version and discharge the matching row’s shadowing guard. |
| Generic | Full Python lookup. `GetMethod` publishes either callable/self or value/absent-self. |

### Data, bindings, constructors, and matching

| Family and operands | Arms and admission | Exits | Components | Batch |
|---|---|---|---|---|
| `GetItem(d,o,k,f)` | Exact list/tuple with admitted in-range inline index. Exact list and slice. Compact ASCII with bounded index. Exact Unicode with bounded index yielding cached ASCII. Dict-compatible mapping slot. Versioned Python `__getitem__` with eligible frame. Generic | N,E,R,* | `IR_DUP_CANONICAL_ELEMENT` as typed read, `_PyList_SliceSubscript`, Unicode length/character rows, dictionary first probe, `value_subscript`, child binding | 1C |
| `SetItem(o,k,v,f)` | Exact list with admitted index and stable storage, dict assignment-slot identity, generic protocol | N,R,* | `_PyValueSlot_ExchangeFilled`, `_PyDict_SetValue`, `_PyValue_SetItem`, lock/unlock | 1C |
| `DelItem(o,k)` | Generic deletion | N,R | `_PyValue_DelItem` | 1C |
| `SliceRead(d,o,start,stop)` | Two-bound slice, with any inline bounds optimization inside the typed slice routine | N,R | `_PyEval_BinarySlice` | 1C |
| `SliceWrite(o,start,stop,v)` | Two-bound slice assignment | N,R | `_PyEval_StoreSlice` | 1C |
| `BuildSlice(d,ss)` | Two or three evaluated components | N,R | `_PySlice_FromValues` | 1C |
| `Build(d,kind,ss)` | Tuple, list, set, or map by literal kind. Map inputs are ordered key/value pairs | N,R | Typed vector versions of `IR_BUILD_TUPLE/LIST/SET/MAP`, staged-owner cleanup | 1E |
| `AddElement(container,kind,item[,v])` | Proven list append, set add, map key/value insertion. Retain the container on success | N,R | `_PyList_AppendTakeValueInline`, `_PySet_AddValueTakeRef`, `_PyDict_SetValue_Take2` | 1E |
| `UpdateContainer(container,kind,s)` | List extend, set update, or dictionary update by display operation | N,R | `_PyList_Extend`, `_PySet_Update`, `PyDict_Update`, corresponding error conversion/chaining | 1E |
| `MergeKeywords(dict,s,fn)` | `**` merge for a call. Duplicate keys and invalid mappings use callable-specific errors | N,R | `value_dict_merge_uniq`, `_PyEval_FormatKwargsError` | 1E |
| `Unpack(dd,s,before,after?,f)` | Tuple/list for exact fixed length and stable contents. Generic iterable for fixed length. Star for a starred recipient and trailing count | N,R,* | Length/item rows, `_PyEval_UnpackIterableValue` behind a rooted output collector, staged publication | 1E |
| `Cell(mode,c,s?,d?,span?)` | Make around prior binding, get, mapping-then-cell get, set, delete, copy free-variable bindings. The mode selects the corresponding argument shape | N,R | `PyCell_NewValue`, `_PyCell_GetValue`, `_PyCell_SetValue`, `_PyCell_SwapValue`, `value_load_from_dict`, unbound formatting, closure reads | 1F |
| `NamespaceRead(d,scope,n?,mapping?)` | Current locals, ordinary name, supplied mapping then globals/builtins, or `__build_class__` lookup | N,R | `_PyEval_LoadName`, `value_load_from_dict`, `value_load_global`, `_PyValue_GetOptionalItem`, frame namespace rows | 1F |
| `NamespaceWrite(scope,n,s?,mode)` | Name or global, set or delete. Delete preserves each namespace’s missing-name behavior | N,R | `_PyEval_StoreName`, `PyDict_SetValue`, `PyDict_PopValue`, name-error rows | 1F |
| `GlobalRead(d,n,f)` | Module/global entry with matching keys version and present slot; builtin entry with both appropriate version guards; generic | N,R,* | `value_load_global`, checked dictionary keys, entry and presence rows | 1F |
| `SetupAnnotations()` | Materialize the annotations mapping when the namespace requires it | N,R | `_PyValue_GetOptionalItem`, `PyDict_New`, `_PyValue_SetItem` | 1F |
| `Import(d,mode,n,level?,fromlist?,module?,flags)` | Name/from and ordinary/lazy import as declared by source flags and receiver | N,R | `_PyEval_ImportName`, `_PyEval_LazyImportName`, `_PyEval_ImportFromValue`, `_PyEval_LazyImportFrom` | 1F |
| `BuildText(d,kind,ss,flags)` | Join, interpolation, template. Preserve evaluated components and source conversion flags | N,R | String-vector builder, `_PyInterpolation_BuildValue`, `_PyTemplate_Build` | 1G |
| `Convert(d,s,kind)` | `str`, `repr`, or `ascii` by conversion flag | N,R | `_PyValue_Str`, `_PyValue_Repr`, `_PyValue_ASCII` | 1G |
| `Format(d,s,spec?)` | Simple exact-string identity where admitted, simple protocol, or explicit format specification | N,R | `_PyValue_Str`, `_PyValue_Format`, exact-string test | 1G |
| `Length(d,s)` | Generic size and canonical integer result | N,R | `_PyValue_Size`, `PyValue_FromInt64` | 1H |
| `Pattern(d?,subject,kind,cls?,names?,count?,mismatch?)` | Mapping/sequence eligibility flags. Keys/class capture on success, mismatch on ordinary failure, propagate protocol errors | N,B,R | Type feature flags, `_PyEval_MatchKeys`, `_PyEval_MatchClass`, checked class/isinstance helper | 1H |
| `Function(mode,d?,codeOrFn,s?,field?)` | Make from compiler-proved code or install the designated defaults/annotations/closure field with its ownership convention | N,R | `PyFunction_New`, `_PyFunction_SetVersion`, `_PyFunction_StoreAttribute` | 1I |
| `Intrinsic(d,id,ss)` | One typed signature per admitted unary/binary intrinsic ID, including typing, import-star, exception-group preparation, and async-gen wrap. Known wide-name and comparison escape encodings can lower to the appropriate semantic family | N,R | `_PyEval_CallIntrinsic1/2` with per-ID contracts, explicit GETATTR/SETATTR and numeric special cases | 1I |

### Calls, frames, iteration, suspension, and observation

| Family and operands | Arms and admission | Exits | Components | Batch |
|---|---|---|---|---|
| `Call(d,fn,self?,a,f)` | Exact Python, exact bound method, general Python/bound method, generator construction, allocation plus `__init__`, admitted native convention, generic callable. Native admissions below | N,E,R,D,* | Version/PEP 523/recursion/space rows, `call_push_exact`, general binder, `call_new_generator`, `_PyEval_CreateInitFrame`, vectorcall and native rows, end poll, saved continuation | P4 |
| `InitResult(s)` | Synthetic `__init__` cleanup continuation. None succeeds; any other result raises `TypeError` | N,R | `PyValue_IsNone`, formatted type error, retirement | P4 |
| `Return(s)` | Ordinary frame, Empty with proven vacant scratch, generator completion with saved consumer, or External evaluator exit | T,X,R,D | Pin, heap-safe acquire, unlink/leave recursion, frame retirement with rooted result, `complete_return`/`complete_send` | P4 |
| `CallKw(d,fn,self?,a,names,f)` | Current Python or bound-method binding plan, ordinary Python binder, eligible native keyword convention, generic | N,E,R,D,* | Keyword-names proof, bound-method preparation, typed binding-plan rows, native keyword call, continuation and end poll | 2A |
| `CallEx(d,fn,star,kwargs?,f)` | Standard Python vectorcall after argument normalization; other callable through generic vectorcall; generic error route | N,E,R,D,* | Normalize positional tuple once, validate kwargs, general binding/vectorcall, continuation and end poll | 2A |
| `MaterializeKwargs(l)` | Pending lazy kwargs binding for this local; already materialized is a no-op | N,R | `_PyFrame_MaterializeKwargs` | 2A |
| `IterOpen(p,s,mode,f)` | Self iterator for `tp_iter == PyObject_SelfIter`; Indexed for admitted index protocol; GenMethod for a current versioned generator `__iter__`; generic. Mode distinguishes ordinary and yield-from setup | N,R,* | `_PyEval_GetIter`, `_PyType_IteratesByIndex`, cursor initialization, `_PyGen_NewCall`, prologue/discard rows | 2B |
| `ForIter(item,p,body,done,f[,pairDsts])` | Exact list/tuple with producer-proven cursor, exact eligible range iterator, virtual next/index protocol, claimable generator, guarded dict-items pair delivery, generic | N,B,J,E,R,D,* | Length/item and cursor rows, `_PyForIter_VirtualIteratorNextInline`, `_PyDictIterItem_Next`, claim/enter, `complete_for` | 2B |
| `AsyncIter(d,s,mode)` | Open async iterator or obtain next awaitable by the corresponding protocol | N,R | `_PyEval_GetAIter`, `_PyEval_GetANext` | 2C |
| `Awaitable(d,s,originKind)` | Already transferable coroutine or checked await protocol. Origin distinguishes diagnostics for ordinary await, async enter, and async exit | N,R | `coro_awaitable_can_transfer`, `_PyEval_GetAwaitable` | 2C |
| `Send(yielded,completed,p,sent,done,f)` | Claimable exact generator/coroutine with admissible initial send; virtual cursor and `None`; exact async-gen-asend; generic send/next | N,B,E,R,D,*; folded consumer can T,Y | `gen_may_claim`, `gen_claim`, `_PyValue_Send`, typed next/index slots, child exception chain, `complete_send` | 2C |
| `Yield(s,resumeDst,r,mode)` | Ordinary suspension or delegated consumer with verified resume route | Y,R,D | Heap-safe acquire, generator state/exception swap, pin/switch, checked delivery and observation | 2C |
| `Raise(exc?,cause?)` | Bare raise, explicit exception, or exception with cause | R | `do_raise`, exception conversion, reraising monitor | 2D |
| `Reraise(exc,lasti?)` | Restore exception and optional public origin | R | `_PyErr_SetRaisedException`, checked PC map, reraising monitor | 2D |
| `ExceptState(mode,d?,exc)` | Enter handler by saving previous handled exception, or leave by restoring it | N | `_PyErr_StackItemTakeValue`, handled-exception install/clear, close | 2D |
| `ExceptMatch(d[,rest],exc,pattern,mode)` | Ordinary validated exception target, or exception-group split with handled-state update | N,R | `_PyEval_CheckExceptTypeValidValue`, `_PyEval_CheckExceptStarTypeValidValue`, `PyErr_GivenExceptionMatches`, `value_exception_group_match` | 2D |
| `CleanupThrow(exc,p,completed,done,r)` | Delegated `StopIteration` delivers value to consumer; other exceptions propagate | B,T,R,D | `_PyStopIteration_GetValue`, exception restoration, `complete_send` | 2E |
| `EndAsyncFor(exc,t,sendOrigin)` | Consume `StopAsyncIteration` and continue; restore and propagate any other exception | N,J,R,D | `PyErr_GivenExceptionMatches`, `_PyErr_SetRaisedException`, branch observer with original SEND origin | 2E |
| `WithExit(d,exitFn,self?,exc,context)` | Invoke saved sync or async exit callable for exceptional completion. Async exit’s await is a distinct `Awaitable`/`Send` sequence | N,R | `with_exit_call`, exception class/value/traceback access, checked callable/self pair | 2E |
| `Resume(kind,site)` | Fresh entry, resumed frame, instrumentation refresh, or eligible compiled reentry. Poll according to resume location | N,J,R,D,X | Instrumentation version, `_Py_Instrument`, `check_periodics`, warmup/event, G3 `RESUME_CHECK` | 2F |
| `Poll(kind,site)` | Pending-event check at a declared boundary, optional compiled-entry event | N,R,D,X | `_Py_EvalBreakerHasEvents`, `_Py_HandlePending`, QSBR/signal rows, `IR_JIT_EVENT` successor contract | 2F |

For `Call.Exact`, admission includes the current callable/code, ordinary entry shape, exact positional binding, frame space, recursion budget, and default eval hook. `BoundExact` also requires an admissible bound method, absent explicit self, and safe method release. A weakref callback on the method is sufficient to disqualify early release. `Call.Generator` creates and returns a suspended frame. `Call.Init` requires a current admitted allocator/init combination and room for the init cleanup frame.

| Native Call admission | Action rows |
|---|---|
| Exact `type`, `str`, `tuple`, or `len` builtin, correct arity and absent self | Own type result; `_PyValue_Str`; `_PyValue_SequenceTuple`; `_PyValue_Size` plus integer construction, respectively |
| Exact `isinstance`, two explicit arguments | `_PyValue_IsInstance` with the checked fast identity case |
| Exact list-append callable and admitted list receiver | `_PyList_AppendTakeValue`, correct mutation/retirement order |
| Admitted builtin class with its current vectorcall | `call_class_values` |
| Exact builtin with `METH_O`, FASTCALL, or FASTCALL plus keywords, matching actual argument shape | `native_call_builtin_o` or `call_builtin_values` |
| Method descriptor with matching receiver and `O`, NOARGS, FASTCALL, or FASTCALL plus keywords | `_PyEval_CallMethodDescriptorO`, `_PyEval_CallMethodDescriptorNoArgs`, or `call_descriptor_values`, with balanced recursion |
| Other native callables | Generic `call_vector`/vectorcall protocol, including its exceptions |

Line, instruction, call, return, yield, resume, branch, stop-iteration, and super-call observations are **wrappers**, not additional language families. The family author specifies origin, PRE/POST phase, event operands, normal successor, and callback-error/redirect recipe. The wrapper generator binds the underlying family and arm. This prevents a second implementation of branch truth, a second call binder, or a second generator completion. `Wide` and compact argument payloads are encoding machinery. Neither defines a Python operation.

### Batch ownership

Each ID is one Sonnet unit. **Review** means an Opus review is on the critical integration checklist for that unit.

| Batch | Scope | Review |
|---|---|---|
| P1 | Constant and local binding operations, saved locals, residual home moves and source-only origins | Debugger-visible ownership |
| P2 | Addition semantics, number/text admission, overflow, generic dispatch and the first template | Pilot language/backend |
| P3 | Attribute and method lookup, including mono/poly recipes, descriptor fallback and getter frames | Transfers and callback phases |
| P4 | Positional calls, init cleanup, return, compiled frame switch and external exit | Transfers |
| P5 | Truth, direct jumps, conditional retention and None branches | Observed edges |
| 1A | Remaining numeric and in-place operations and unary protocols | Normal batch review |
| 1B | Value tests and their conditionally fused branch consumers | Fused observer origins |
| 1C | Item lookup/mutation/deletion and slice operations | Python `__getitem__` transfer |
| 1D | Attribute writes, super and special-method resolution | Calls and observation |
| 1E | Displays, updates, comprehension insertion, keyword merge and staged unpack | Normal batch review |
| 1F | Namespace and cell binding, global lookup, annotation setup and import | Normal batch review |
| 1G | Text building, conversion and formatting | Normal batch review |
| 1H | Length and pattern eligibility, captures, mismatch and error | Normal batch review |
| 1I | Function construction/fields and the finite intrinsic catalog | Normal batch review |
| 2A | Keyword and expanded calls, argument materialization | Transfers |
| 2B | Iterator setup, stepping, exhaustion and generator iteration | Transfers and observers |
| 2C | Async iteration, await, send, yield and delegated resume | Suspension and transfers |
| 2D | Exception creation, propagation, handled state and group matching | Unwind/observer phases |
| 2E | Delegated throw completion, async-loop termination and exceptional context exit | Suspension and observers |
| 2F | Resume, periodic work, instrumentation routing and wrapper integration | Transfers and observers |

## Appendix B. Coverage of the old declarations

The key is the analyzer’s uop name, including the leading `_` it supplies for `inst(...)`. I checked the union of parts referenced by source and IR instructions at `dd7e484be64f`. It contains **373 distinct names**. Executable handler records reference **367**. The other six are the prefix, fallthrough marker, and four local superinstructions. Separately, `_POP_TOP_OPARG` and `_GUARD_NOS_TUPLE` exist in the file but have **no instruction users**. Delete both in the final source cleanup. They are outside the 373-row denominator. Use **440** for the header’s row-line inventory. The existing parser returns 438 named components. In particular, three row-looking `SLOT(...)` lines are field result types, and the parser also recognizes `STATE`. [Analyzer construction][analysis] · [Source and executing handler inventories][inventory] · [Component parser][rowparser]

`A` means family arm or outcome, `M` shared typed macro or its named operation inside an arm, `C` component row, `D` decoder rename, `W` observer wrapper, `U` unreachable as a semantic head, and `–` no semantic obligation. Every `A/M/C` mapping includes the old declared argument/result ownership, effects, facts, feedback accesses, and exits **on the paths that use it**. The typed graph may discharge an obligation by proof, such as a close of an inline number. A former `deopt` is a restartable arm miss with the proper adaptive accounting. Every observer and exceptional edge is checked against its own checkpoint. The port gate compares these projected contracts per old **use**, so satisfying one call site of a shared declaration cannot mark all its other uses covered.

| Old uop | Destination of its obligations |
|---|---|
| `_NOP` | D origin, or `Nop` at a required entry |
| `_MATERIALIZE_KWARGS` | A `MaterializeKwargs`, pending binding |
| `_CHECK_PERIODIC` | M `Poll` at interruptible backedge |
| `_CHECK_PERIODIC_AT_END` | M end poll after completed call |
| `_CHECK_PERIODIC_IF_NOT_YIELD_FROM` | A `Resume`, location-dependent poll |
| `_MAYBE_INSTRUMENT` | W `Resume`, refresh instrumentation and redispatch |
| `_MONITOR_RESUME` | W `Resume`, error or redirected frame |
| `_LOAD_FAST_CHECK` | A `LocalRead.Checked`, unbound error |
| `_LOAD_FAST` | A `LocalRead.Bound`, stable read or snapshot |
| `_LOAD_FAST_BORROW` | D stable source, otherwise an owning `LocalRead` |
| `_LOAD_FAST_AND_CLEAR` | A `SaveLocal`, move binding including unbound |
| `_LOAD_CONST` | A `Const.Pool`, owned result |
| `_LOAD_CONST_INLINE_INT` | A `Const.Pool`, inline-int fact |
| `_LOAD_CONST_INLINE_FLOAT` | A `Const.Pool`, inline-float fact |
| `_LOAD_SMALL_INT` | A `Const.Small`, immediate |
| `_LOAD_SMALL_INT_INLINE` | A `Const.Small`, inline-int result |
| `_SWAP_FAST` | A `LocalWrite`, install before retiring displaced binding |
| `_POP_TOP` | M `publish/spent` or edge retirement; stack pop disappears |
| `_POP_TOP_INT` | M exact-long retirement or inline erasure |
| `_POP_TOP_FLOAT` | M inline-float proof discharges the close |
| `_POP_TOP_NUMBER` | M inline-number proof discharges the close |
| `_POP_TOP_UNICODE` | M retirement with exact Unicode destructor |
| `_PUSH_NULL` | D absent self/kwargs, deletion marker, or null cursor by consumer |
| `_END_FOR` | M `complete_for`, retire synthetic completion value |
| `_POP_ITER` | M `complete_for`, retire receiver and cursor |
| `_INSTRUMENTED_END_FOR` | W `ForIter` completion, report generator StopIteration |
| `_INSTRUMENTED_POP_ITER` | W committed exit, defer close through observation |
| `_END_SEND` | M `complete_send`, transfer result and retire receiver |
| `_INSTRUMENTED_END_SEND` | W send completion, stop/branch/error/redirect |
| `_UNARY_NEGATIVE` | A `Unary.Negate` |
| `_UNARY_NOT` | A `Truth.Invert` |
| `_SPECIALIZE_TO_BOOL` | M `Adapt(Truth)` |
| `_TO_BOOL` | A `Truth.Generic`, fallible truth |
| `_GUARD_TOS_BOOL` | M `TruthTest.Bool`, canonical bool |
| `_TO_BOOL_INT` | M `TruthTest.Int`, inline or boxed zero |
| `_GUARD_NOS_LIST` | M exact-list receiver proof, including native append |
| `_GUARD_TOS_LIST` | M exact-list value proof for truth and unpack |
| `_GUARD_TOS_SLICE` | A `GetItem.ListSlice`, slice guard |
| `_TO_BOOL_LIST` | M `TruthTest.List`, current length |
| `_TO_BOOL_NONE` | M `TruthTest.None`, false |
| `_GUARD_NOS_COMPACT_ASCII` | A `GetItem.Ascii`, compact ASCII guard |
| `_GUARD_NOS_UNICODE` | M exact-string left operand for concat/index/compare |
| `_GUARD_TOS_UNICODE` | M exact-string operand for concat/truth/compare |
| `_TO_BOOL_STR` | M `TruthTest.Str`, current length |
| `_GUARD_NONE_OR_TYPE_VERSION` | M `TruthTest.Plain`, None or nominated version |
| `_TRUTH_NONE_OR_PLAIN` | M `TruthTest.Plain`, None false otherwise true |
| `_UNARY_INVERT` | A `Unary.Invert` |
| `_GUARD_NOS_INT` | C inline-int predicate on named left operand |
| `_GUARD_TOS_INT` | C inline-int predicate on named right/value operand |
| `_GUARD_TOS_EXACT_INT` | M `TruthTest.Int`, exact includes boxed ints |
| `_BINARY_OP_MULTIPLY_INT` | A `Binary.Int`, checked product/box |
| `_BINARY_OP_ADD_INT` | A `Add.Int`, overflow box and error phase |
| `_BINARY_OP_SUBTRACT_INT` | A `Binary.Int`, subtraction |
| `_BINARY_OP_FLOOR_DIVIDE_INT` | A `Binary.Int`, nonzero divisor and floor rounding |
| `_BINARY_OP_REMAINDER_INT` | A `Binary.Int`, divisor’s sign |
| `_BINARY_OP_AND_INT` | A `Binary.Int`, bitwise AND |
| `_BINARY_OP_OR_INT` | A `Binary.Int`, bitwise OR |
| `_BINARY_OP_XOR_INT` | A `Binary.Int`, bitwise XOR |
| `_BINARY_OP_LSHIFT_INT` | A `Binary.Int`, count guard and large result |
| `_BINARY_OP_RSHIFT_INT` | A `Binary.Int`, count guard and sign extension |
| `_GUARD_NOS_FLOAT` | M `CompareValues.Float`, inline left |
| `_GUARD_TOS_FLOAT` | M `CompareValues.Float`, inline right |
| `_BINARY_OP_MULTIPLY_FLOAT` | A `Binary.Float`, mixed numeric multiply and NaN |
| `_BINARY_OP_ADD_FLOAT` | A `Add.Float`, mixed numeric add and NaN |
| `_BINARY_OP_SUBTRACT_FLOAT` | A `Binary.Float`, subtraction |
| `_BINARY_OP_TRUEDIV_FLOAT` | A `Binary.Float`, division and zero guard |
| `_BINARY_OP_ADD_UNICODE` | A `Add.Text`, exact concatenation |
| `_BINARY_OP_INPLACE_ADD_UNICODE` | A `Binary.TextLocal`, target/value identity and ownership |
| `_BINARY_OP_EXTEND` | M `ExtendedBinary`, guarded descriptor and typed action |
| `_SPECIALIZE_BINARY_SLICE` | – stats-only placeholder; no slice nomination to port |
| `_BINARY_SLICE` | A `SliceRead` |
| `_SPECIALIZE_STORE_SLICE` | – stats-only placeholder; no slice nomination to port |
| `_STORE_SLICE` | A `SliceWrite` |
| `_GUARD_ELEMENT_INT` | M indexed read, inline integer index |
| `_GUARD_ELEMENT_LIST` | M indexed read, exact list receiver |
| `_GUARD_ELEMENT_TUPLE` | M indexed read, exact tuple receiver |
| `_BINARY_OP_SUBSCR_LIST_INT` | A `GetItem.ListIndex`, bounds then owned read |
| `_BINARY_OP_SUBSCR_LIST_SLICE` | A `GetItem.ListSlice` |
| `_BINARY_OP_SUBSCR_STR_INT` | A `GetItem.Ascii`, bounded index |
| `_BINARY_OP_SUBSCR_USTR_INT` | A `GetItem.UnicodeAsciiChar`, bounded index and character |
| `_GUARD_TOS_TUPLE` | M exact tuple, unpack admission |
| `_BINARY_OP_SUBSCR_TUPLE_INT` | A `GetItem.TupleIndex` |
| `_GUARD_NOS_DICT_SUBSCRIPT` | A `GetItem.Dict`, mapping subscript slot identity |
| `_GUARD_NOS_DICT_STORE_SUBSCRIPT` | A `SetItem.Dict`, mapping assignment slot identity |
| `_GUARD_TOS_ANY_DICT` | M `ContainsTest.Dict`, admitted dict representation |
| `_BINARY_OP_SUBSCR_DICT` | A `GetItem.Dict`, first probe or full mapping call |
| `_BINARY_OP_SUBSCR_CHECK_FUNC` | A `GetItem.PyMethod`, version/code/space guards |
| `_BINARY_OP_SUBSCR_INIT_CALL` | M bind and enter with subscript continuation |
| `_LIST_APPEND` | A `AddElement.List`, take item |
| `_SET_ADD` | A `AddElement.Set`, take item and handle hash error |
| `_SPECIALIZE_STORE_SUBSCR` | M `Adapt(SetItem)` |
| `_STORE_SUBSCR` | A `SetItem.Generic` |
| `_STORE_SUBSCR_LIST_INT` | A `SetItem.ListIndex`, bounds/lock/exchange |
| `_STORE_SUBSCR_DICT` | A `SetItem.Dict`, guarded assignment |
| `_DELETE_SUBSCR` | A `DelItem` |
| `_CALL_INTRINSIC_1` | A `Intrinsic.Unary` or decoded `Unary.Positive` |
| `_CALL_INTRINSIC_2` | A `Intrinsic.Binary` or decoded semantic operation, anchor both |
| `_RAISE_VARARGS` | A `Raise`, bare/exception/cause |
| `_INTERPRETER_EXIT` | A `Return.External`, transfer result to evaluator |
| `_MAKE_HEAP_SAFE` | M `acquire_heap_safe` for Return/Yield |
| `_RETURN_VALUE` | A `Return`, unlink/retire/complete |
| `_RETURN_VALUE_EVENT` | W `Return`, report before departure |
| `_GET_AITER` | A `AsyncIter.Open` |
| `_GET_ANEXT` | A `AsyncIter.NextAwaitable` |
| `_GET_AWAITABLE` | A `Awaitable`, transferable or protocol result |
| `_SPECIALIZE_SEND` | M `Adapt(Send)` |
| `_SEND` | A `Send.Generic`, next/finished/enter/error |
| `_SEND_GEN_FRAME` | A `Send.Generator`, claim and terminal entry |
| `_GUARD_TOS_IS_NONE` | A `Send.Virtual`, sent value is None |
| `_GUARD_NOS_NOT_NULL` | A `Send.Virtual`, cursor present |
| `_SEND_VIRTUAL` | A `Send.Virtual`, advance/exhaust/error |
| `_GUARD_3OS_ASYNC_GEN_ASEND` | A `Send.AsyncGen`, exact asend receiver |
| `_SEND_ASYNC_GEN` | A `Send.AsyncGen`, discriminated result |
| `_YIELD_VALUE` | A `Yield`, suspend and route |
| `_YIELD_VALUE_EVENT` | W `Yield`, inspect/raise/redirect |
| `_POP_EXCEPT` | A `ExceptState.Leave`, restore previous handled exception |
| `_RERAISE` | A `Reraise`, exception owner and optional lasti |
| `_END_ASYNC_FOR` | A `EndAsyncFor`, filter StopAsyncIteration |
| `_MONITOR_END_ASYNC_FOR` | W `EndAsyncFor`, use semantic SEND origin |
| `_CLEANUP_THROW` | A `CleanupThrow`, extract completion or propagate |
| `_LOAD_COMMON_CONSTANT` | A `Const.Common`, interpreter root |
| `_LOAD_BUILD_CLASS` | A `NamespaceRead.BuildClass`, optional builtins lookup |
| `_STORE_NAME` | A `NamespaceWrite.Name`, mapping set/delete |
| `_SPECIALIZE_UNPACK_SEQUENCE` | M `Adapt(Unpack)` |
| `_UNPACK_SEQUENCE` | A `Unpack.Generic`, staged outputs and partial cleanup |
| `_UNPACK_SEQUENCE_TUPLE` | A `Unpack.Tuple`, exact length |
| `_UNPACK_SEQUENCE_LIST` | A `Unpack.List`, stable contents and exact length |
| `_UNPACK_EX` | A `Unpack.Star`, before/after counts |
| `_SPECIALIZE_STORE_ATTR` | M `Adapt(AttrWrite)`, skip deletion |
| `_STORE_ATTR` | A `AttrWrite.Generic`, set or delete |
| `_STORE_GLOBAL` | A `NamespaceWrite.Global`, set/delete/missing error |
| `_LOAD_LOCALS` | A `NamespaceRead.Locals`, current namespace |
| `_LOAD_FROM_DICT_OR_GLOBALS` | A `NamespaceRead.DictGlobal`, mapping then global/builtins |
| `_LOAD_NAME` | A `NamespaceRead.Name` |
| `_SPECIALIZE_LOAD_GLOBAL` | M `Adapt(GlobalRead)` |
| `_LOAD_GLOBAL` | A `GlobalRead.Generic`, explicit absent self if needed |
| `_NULL_SLOT_CONDITIONAL` | D optional self is a named absent role; no extra stack slot |
| `_GUARD_GLOBALS_VERSION` | A `GlobalRead.Builtin`, globals keys-version guard |
| `_LOAD_GLOBAL_MODULE` | A `GlobalRead.Module`, guarded current slot |
| `_LOAD_GLOBAL_BUILTINS` | A `GlobalRead.Builtin`, builtin keys/version/index/presence |
| `_DELETE_FAST` | A `LocalDelete`, unbound error |
| `_MAKE_CELL` | A `Cell.Make`, capture prior binding |
| `_DELETE_DEREF` | A `Cell.Delete`, unbound error |
| `_LOAD_FROM_DICT_OR_DEREF` | A `Cell.DictGet`, mapping then closure |
| `_LOAD_DEREF` | A `Cell.Get`, check unbound |
| `_STORE_DEREF` | A `Cell.Set`, replace owner |
| `_COPY_FREE_VARS` | A `Cell.CopyFree`, acquire closure owners |
| `_BUILD_STRING` | A `BuildText.Join`, ordered vector |
| `_BUILD_INTERPOLATION` | A `BuildText.Interpolation` |
| `_BUILD_TEMPLATE` | A `BuildText.Template` |
| `_BUILD_TUPLE` | A `Build.Tuple` |
| `_BUILD_LIST` | A `Build.List` |
| `_LIST_EXTEND` | A `UpdateContainer.ListExtend`, iterator errors |
| `_SET_UPDATE` | A `UpdateContainer.SetUpdate`, hash/iterator errors |
| `_BUILD_SET` | A `Build.Set` |
| `_BUILD_MAP` | A `Build.Map`, ordered key/value pairs |
| `_SETUP_ANNOTATIONS` | A `SetupAnnotations`, namespace hooks |
| `_DICT_UPDATE` | A `UpdateContainer.DictUpdate`, mapping errors |
| `_DICT_MERGE` | A `MergeKeywords`, duplicate and callable-specific errors |
| `_MAP_ADD` | A `AddElement.Map`, take key and value |
| `_SPECIALIZE_LOAD_SUPER_ATTR` | M `Adapt(SuperLookup)` |
| `_LOAD_SUPER_ATTR` | A `SuperLookup.Generic`, preserve observable super call |
| `_LOAD_SUPER_ATTR_ATTR` | A `SuperLookup.BuiltinAttr`, checked super and class |
| `_GUARD_LOAD_SUPER_ATTR_METHOD` | A `SuperLookup.BuiltinMethod`, binding guard |
| `_LOAD_SUPER_ATTR_METHOD` | A `SuperLookup.BuiltinMethod`, paired result |
| `_SPECIALIZE_LOAD_ATTR` | M `Adapt(GetAttr/GetMethod)` |
| `_LOAD_ATTR` | A `GetAttr/GetMethod.Generic`, immutable name |
| `_GUARD_TYPE_VERSION` | M version match on the named receiver |
| `_GUARD_TYPE_VERSION_LOCKED` | M locked version match, unlock on miss |
| `_CHECK_MANAGED_OBJECT_HAS_VALUES` | C validated inline-values read |
| `_LOAD_ATTR_INSTANCE_VALUE` | A `GetAttr.Instance`, nullable slot and owned hit |
| `_GUARD_SHAPE` | M current-shape check on write |
| `_GUARD_SHAPE_READ` | M correlated version/shape check on read |
| `_LOAD_ATTR_SHAPE` | A `GetAttr.Shape`, duplicate protected value |
| `_GUARD_MODULE_EXACT` | A `GetAttr.Module`, exact module |
| `_LOAD_ATTR_MODULE` | A `GetAttr.Module`, validated dictionary slot |
| `_LOAD_ATTR_WITH_HINT` | A `GetAttr.Hint`, key identity/index/presence |
| `_LOAD_ATTR_SLOT` | A `GetAttr.ValueSlot`, checked slot and presence |
| `_CHECK_ATTR_CLASS` | M class-version check on type receiver |
| `_LOAD_ATTR_CLASS` | A `GetAttr.Class`, protected class payload |
| `_GUARD_BUILTIN_METACLASS` | M exact metaclass/build eligibility |
| `_LOAD_ATTR_PROPERTY_FRAME` | A `GetAttr.Property`, child and continuation |
| `_LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN_FRAME` | A `GetAttr.Overridden`, child and continuation |
| `_GUARD_DORV_NO_DICT` | A `AttrWrite.Instance`, no managed dict and valid values |
| `_STORE_ATTR_INSTANCE_VALUE` | A `AttrWrite.Instance`, insertion order and transition |
| `_LOCK_OBJECT` | M scoped write admission, balanced unlock |
| `_STORE_ATTR_SHAPE` | A `AttrWrite.Shape`, exchange filled slot |
| `_STORE_ATTR_SHAPE_ADD` | A `AttrWrite.ShapeAdd`, insertion order and shape change |
| `_STORE_ATTR_WITH_HINT` | A `AttrWrite.Hint`, guarded update or slow completion |
| `_STORE_ATTR_SLOT` | A `AttrWrite.Slot`, exchange and retire prior owner |
| `_SPECIALIZE_COMPARE_OP` | M `Adapt(Compare)` |
| `_COMPARE_OP` | M `CompareValues.Generic`, rich result/optional truth |
| `_COMPARE_OP_FLOAT` | M `CompareValues.Float`, including unordered comparisons |
| `_COMPARE_OP_INT` | M `CompareValues.Int`, signed inline comparison |
| `_COMPARE_OP_STR` | M `CompareValues.Str`, supported equality sense |
| `_IS_OP` | M `IdentityTest`, PyValue identity and inversion |
| `_CONTAINS_OP` | M `ContainsTest.Generic`, optional inversion |
| `_SPECIALIZE_CONTAINS_OP` | M `Adapt(Contains)` |
| `_GUARD_TOS_ANY_SET` | M `ContainsTest.Set`, admitted set/frozenset |
| `_CONTAINS_OP_SET` | M `ContainsTest.Set`, fallible membership |
| `_CONTAINS_OP_DICT` | M `ContainsTest.Dict`, fallible membership |
| `_GUARD_TOS_STR_TUPLE_LIST` | M `ContainsTest.Sequence`, exact supported receiver |
| `_CONTAINS_OP_SEQUENCE` | C checked sequence contains slot |
| `_CHECK_EG_MATCH` | A `ExceptMatch.Group`, split and update handled exception |
| `_CHECK_EXC_MATCH` | A `ExceptMatch.Single`, validate target then match |
| `_IMPORT_NAME` | A `Import.Name`, level/fromlist/lazy flags |
| `_IMPORT_FROM` | A `Import.From`, lazy or ordinary |
| `_JUMP_FORWARD` | A `Jump`, static forward target |
| `_POP_JUMP_IF_FALSE` | A `BranchTruth`, false edge and retirement |
| `_POP_JUMP_IF_TRUE` | A `BranchTruth`, true edge and retirement |
| `_IS_NONE` | A `BranchNone`, identity and edge owners |
| `_SPECIALIZE_POP_JUMP_IF` | M `Adapt(BranchTruth)` |
| `_POP_JUMP_IF_FALSE_NONE_OR_PLAIN` | A `BranchTruth.Plain`, inverted sense |
| `_POP_JUMP_IF_TRUE_NONE_OR_PLAIN` | A `BranchTruth.Plain`, ordinary sense |
| `_JUMP_IF_FALSE_OR_POP` | A `BranchTruth.Keep`, preserve owner on false edge |
| `_JUMP_IF_TRUE_OR_POP` | A `BranchTruth.Keep`, preserve owner on true edge |
| `_JUMP_BACKWARD_NO_INTERRUPT` | A `Jump`, backedge without interrupt poll |
| `_GET_LEN` | A `Length`, size and possible boxed integer |
| `_MATCH_CLASS` | A `Pattern.Class`, captures/mismatch/error |
| `_MATCH_MAPPING` | A `Pattern.Mapping`, eligibility flag |
| `_MATCH_SEQUENCE` | A `Pattern.Sequence`, eligibility flag |
| `_MATCH_KEYS` | A `Pattern.Keys`, captures/mismatch/error |
| `_SPECIALIZE_GET_ITER` | M `Adapt(IterOpen)` |
| `_GET_ITER` | A `IterOpen.Generic`, iterator/cursor outputs |
| `_GUARD_ITERATOR` | A `IterOpen.Self`, self-iterator slot identity |
| `_GUARD_ITER_VIRTUAL` | A `IterOpen.Indexed`, admitted index iterator |
| `_PUSH_TAGGED_ZERO` | A `IterOpen.Indexed`, initialize tagged cursor |
| `_GUARD_ITER_PY_GENERATOR` | A `IterOpen.GenMethod`, cached `__iter__` and version |
| `_SPECIALIZE_FOR_ITER` | M `Adapt(ForIter)` |
| `_FOR_ITER` | A `ForIter.Generic`, classify step |
| `_FOR_ITER_VIRTUAL` | A `ForIter.Virtual`, next-value/index protocol |
| `_INSTRUMENTED_FOR_ITER` | W `ForIter`, committed body edge and exhaustion |
| `_ITER_CHECK_LIST` | A `ForIter.List`, type and thread eligibility |
| `_ITER_JUMP_LIST` | A `ForIter.List`, live length/exhaustion |
| `_ITER_NEXT_LIST` | A `ForIter.List`, anchor item and advance cursor |
| `_ITER_CHECK_TUPLE` | A `ForIter.Tuple`, type and producer cursor proof |
| `_ITER_JUMP_TUPLE` | A `ForIter.Tuple`, bounds/exhaustion |
| `_ITER_NEXT_TUPLE` | A `ForIter.Tuple`, acquire item and advance cursor |
| `_ITER_CHECK_RANGE` | A `ForIter.Range`, type and exclusive access where required |
| `_ITER_JUMP_RANGE` | A `ForIter.Range`, remaining count/exhaustion |
| `_ITER_NEXT_RANGE` | A `ForIter.Range`, mutate iterator and construct owned int |
| `_FOR_ITER_GEN_FRAME` | A `ForIter.Generator`, claim/enter with consumer continuation |
| `_INSERT_NULL` | D `SpecialLookup` paired operands; staged window write disappears |
| `_LOAD_SPECIAL` | A `SpecialLookup`, lookup and error suggestions |
| `_WITH_EXCEPT_START` | A `WithExit`, invoke with active exception triple |
| `_PUSH_EXC_INFO` | A `ExceptState.Enter`, move previous/current state |
| `_LOAD_ATTR_METHOD_WITH_VALUES` | A `GetMethod.MethodValues`, descriptor and unshadowed receiver |
| `_LOAD_ATTR_METHOD_NO_DICT` | A `GetMethod.NoDict`, descriptor and receiver |
| `_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES` | A `GetAttr.NonDescriptorValues`, guarded class value |
| `_LOAD_ATTR_NONDESCRIPTOR_NO_DICT` | A `GetAttr.NonDescriptorNoDict`, guarded class value |
| `_CHECK_ATTR_METHOD_LAZY_DICT` | A `GetMethod.LazyDict`, absent instance dictionary |
| `_LOAD_ATTR_METHOD_LAZY_DICT` | A `GetMethod.LazyDict`, descriptor/self result |
| `_SPECIALIZE_CALL` | M `Adapt(Call)`, preserve evaluated inputs across qualifying inspection |
| `_MAYBE_EXPAND_METHOD` | M bound-method preparation, protect function and self |
| `_DO_CALL` | A `Call.Generic`, Python transfer/native completion/error |
| `_MONITOR_CALL` | W `Call`, event on prepared operands |
| `_PY_FRAME_GENERAL` | A `Call.Python`, general binding |
| `_PY_FRAME_GENERAL_GUARDED` | A `Call.Python`, validated binding plan |
| `_CHECK_FUNCTION_VERSION` | M function version and callable/code identity |
| `_CHECK_METHOD_VERSION` | M bound method’s function version |
| `_EXPAND_METHOD` | M bound-method preparation, receiver ownership through commit |
| `_CHECK_IS_NOT_PY_CALLABLE` | A `Call.Native`, exclude Python function/method |
| `_CALL_NON_PY_GENERAL` | A `Call.Native`, vectorcall result and end poll |
| `_CHECK_CALL_BOUND_METHOD_EXACT_ARGS` | A `Call.BoundExact`, absent self and no weakref callback |
| `_CHECK_METHOD_EXACT_ARGS` | A `Call.BoundExact`, exact binding shape |
| `_CHECK_METHOD_STACK_SPACE` | A `Call.BoundExact`, space for underlying code |
| `_INIT_CALL_BOUND_METHOD_EXACT_ARGS` | M stage callable/self for exact bound entry |
| `_CHECK_PEP_523` | M default-evaluator guard on every direct entry |
| `_CHECK_FUNCTION_EXACT_ARGS` | M exact argument shape for call and generator factory |
| `_CHECK_STACK_SPACE` | M checked frame capacity |
| `_CHECK_RECURSION_REMAINING` | M recursion budget before committing frame |
| `_INIT_CALL_PY_EXACT_ARGS` | M `bind_exact`, positional and expanded-bound entries |
| `_PUSH_FRAME` | M link/start/enter on live frame builders; tail after an already-terminal enter disappears |
| `_GUARD_NOS_NULL` | A one-argument builtin calls, optional self absent |
| `_GUARD_CALL_SELF_PRESENT` | A `Call.ListAppend`, receiver present |
| `_GUARD_THIRD_NULL` | A `Call.IsInstance`, absent self for two explicit arguments |
| `_GUARD_CALLABLE_TYPE_1` | A `Call.Type1`, callable identity/arity |
| `_CALL_TYPE_1` | A `Call.Type1`, own type result |
| `_GUARD_CALLABLE_STR_1` | A `Call.Str1`, callable identity/arity |
| `_CALL_STR_1` | A `Call.Str1`, Python string conversion |
| `_GUARD_CALLABLE_TUPLE_1` | A `Call.Tuple1`, callable identity/arity |
| `_CALL_TUPLE_1` | A `Call.Tuple1`, sequence conversion |
| `_CHECK_OBJECT` | A `Call.Init`, admitted class/allocation behavior |
| `_CHECK_INIT_FUNCTION` | A `Call.Init`, current init and room for cleanup |
| `_CREATE_INIT_FRAME` | A `Call.Init`, allocate instance and install init frames |
| `_EXIT_INIT_CHECK` | A `InitResult`, enforce None or raise `TypeError` |
| `_GUARD_CALLABLE_BUILTIN_CLASS` | A `Call.BuiltinClass`, class vectorcall admission |
| `_CALL_BUILTIN_CLASS` | A `Call.BuiltinClass`, invoke and retire args |
| `_GUARD_CALLABLE_BUILTIN_O` | A `Call.BuiltinO`, exact convention/arity |
| `_CALL_BUILTIN_O` | A `Call.BuiltinO`, checked native invocation |
| `_GUARD_CALLABLE_BUILTIN_FAST` | A `Call.BuiltinFast`, exact convention/args |
| `_CALL_BUILTIN_FAST` | A `Call.BuiltinFast`, fast positional invocation |
| `_GUARD_CALLABLE_BUILTIN_FAST_WITH_KEYWORDS` | A `Call.BuiltinFastKw`, exact convention |
| `_CALL_BUILTIN_FAST_WITH_KEYWORDS` | A `Call.BuiltinFastKw`, keyword-capable invocation |
| `_GUARD_CALLABLE_LEN` | A `Call.Len`, builtin identity and arity |
| `_CALL_LEN` | A `Call.Len`, size/error and canonical integer result |
| `_GUARD_CALLABLE_ISINSTANCE` | A `Call.IsInstance`, builtin identity and arity |
| `_CALL_ISINSTANCE` | A `Call.IsInstance`, exact/type-protocol result |
| `_GUARD_CALLABLE_LIST_APPEND` | A `Call.ListAppend`, descriptor and exact receiver |
| `_CALL_LIST_APPEND` | A `Call.ListAppend`, mutation and error ordering |
| `_GUARD_CALLABLE_METHOD_DESCRIPTOR_O` | A `Call.DescrO`, descriptor/receiver/arity |
| `_CALL_METHOD_DESCRIPTOR_O` | A `Call.DescrO`, checked invoke and leave recursion |
| `_CHECK_RECURSION_LIMIT` | M native recursion admission and balancing |
| `_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS` | A `Call.DescrFastKw`, descriptor convention/receiver |
| `_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS` | A `Call.DescrFastKw`, invoke keyword-capable slot |
| `_GUARD_CALLABLE_METHOD_DESCRIPTOR_NOARGS` | A `Call.DescrNoArgs`, descriptor/receiver/zero args |
| `_CALL_METHOD_DESCRIPTOR_NOARGS` | A `Call.DescrNoArgs`, checked invoke and leave recursion |
| `_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST` | A `Call.DescrFast`, descriptor/receiver/positional shape |
| `_CALL_METHOD_DESCRIPTOR_FAST` | A `Call.DescrFast`, checked fast invocation |
| `_MONITOR_CALL_KW` | W `CallKw`, prepared arguments and keyword names |
| `_MAYBE_EXPAND_METHOD_KW` | M bound-method preparation, preserve keyword bindings |
| `_DO_CALL_KW` | A `CallKw.Generic`, invoke with keyword names |
| `_PY_FRAME_KW` | A `CallKw.Python`, ordinary keyword binder |
| `_PY_FRAME_KW_GUARDED` | A `CallKw.Python`, validated keyword plan |
| `_CHECK_FUNCTION_VERSION_KW` | M function version, keyword entry |
| `_CHECK_METHOD_VERSION_KW` | M method version, keyword entry |
| `_EXPAND_METHOD_KW` | M bound-method preparation, keyword receiver ownership |
| `_SPECIALIZE_CALL_KW` | M `Adapt(CallKw)`, refresh after qualifying inspection |
| `_CHECK_IS_NOT_PY_CALLABLE_KW` | A `CallKw.Native`, exclude Python function/method |
| `_CALL_KW_NON_PY` | A `CallKw.Native`, checked invocation/completion or protocol transfer |
| `_MAKE_CALLARGS_A_TUPLE` | M `PrepareExArgs`, normalize iterable to tuple once |
| `_DO_CALL_FUNCTION_EX` | A `CallEx.Generic`, bind/invoke |
| `_SPECIALIZE_CALL_FUNCTION_EX` | M `Adapt(CallEx)`, preserve evaluated arguments |
| `_CHECK_IS_PY_CALLABLE_EX` | A `CallEx.Python`, standard Python vectorcall |
| `_PY_FRAME_EX` | A `CallEx.Python`, star/dict binding |
| `_CHECK_IS_NOT_PY_CALLABLE_EX` | A `CallEx.Native`, reject standard Python vectorcall |
| `_CALL_FUNCTION_EX_NON_PY_GENERAL` | A `CallEx.Native`, expanded completion/entry/error |
| `_MAKE_FUNCTION` | A `Function.Make`, code/function ownership and version |
| `_SET_FUNCTION_ATTRIBUTE` | A `Function.SetField`, transfer appropriate attribute |
| `_RETURN_GENERATOR` | U creation marker; generator factory owns creation, resume/source map retains PC |
| `_BUILD_SLICE` | A `BuildSlice`, two or three evaluated bounds |
| `_CONVERT_VALUE` | A `Convert`, str/repr/ascii |
| `_FORMAT_SIMPLE` | A `Format.Simple`, identity or formatting protocol |
| `_FORMAT_WITH_SPEC` | A `Format.Spec`, both operands rooted |
| `_COPY` | D duplicate value identity, acquire another owner if independently live |
| `_SPECIALIZE_BINARY_OP` | M `Adapt(Add/Binary/GetItem)`, immutable selector |
| `_BINARY_OP` | A `Add/Binary.Generic` or `GetItem.Generic` by numeric selector |
| `_SWAP` | D permute virtual names, resolve residual edge assignment |
| `_INSTRUMENTED_LINE` | W line, source identity, error/redirect |
| `_INSTRUMENTED_INSTRUCTION` | W instruction, preserve underlying selection/PC |
| `_INSTRUMENTED_JUMP_FORWARD` | W `Jump`, committed forward edge |
| `_MONITOR_JUMP_BACKWARD` | W `Jump`, committed backward edge |
| `_INSTRUMENTED_POP_JUMP_IF_TRUE` | W `BranchTruth`, true sense |
| `_INSTRUMENTED_POP_JUMP_IF_FALSE` | W `BranchTruth`, false sense |
| `_INSTRUMENTED_POP_JUMP_IF_NONE` | W `BranchNone`, None edge |
| `_INSTRUMENTED_JUMP_IF_FALSE_OR_POP` | W `BranchTruth.Keep`, false-edge owner |
| `_INSTRUMENTED_JUMP_IF_TRUE_OR_POP` | W `BranchTruth.Keep`, true-edge owner |
| `_INSTRUMENTED_POP_JUMP_IF_NOT_NONE` | W `BranchNone`, non-None edge |
| `_CACHE` | U public inline-cache data, invalid to enter |
| `_RESERVED` | U reserved encoding, invalid to enter |
| `_EXTENDED_ARG` | U decoder consumes prefix, no semantic entry |
| `_NOT_TAKEN` | U decoder consumes fallthrough marker, retains source edge |
| `_LOAD_FAST_LOAD_FAST` | U decoder splits two reads |
| `_LOAD_FAST_BORROW_LOAD_FAST_BORROW` | U decoder splits two reads and proves borrow stability |
| `_STORE_FAST_LOAD_FAST` | U decoder splits store then read |
| `_STORE_FAST_STORE_FAST` | U decoder splits two ordered stores |
| `_SAVE_RETURN_SLOT` | M `save_continuation`, caller recipient and successor |
| `_IR_JUMP` | A `Jump`, compiler-inserted control edge |
| `_IR_OPERANDS` | U payload data, verifier rejects incoming PC |
| `_JIT_EVENT` | A `Poll/Resume` JIT event, redirect/exit/error; nested execution grants no direct child entry |
| `_RETURN_VALUE_EMPTY` | A `Return.Empty`, proven vacant scratch homes |
| `_YIELD_DELEGATED` | A `Yield.Delegated`, consumer resume/completion |
| `_LOAD_ATTR_SLOT_POINTER` | A `GetAttr.PointerSlot`, guarded pointer and owned result |
| `_LOAD_ATTR_CLASSMETHOD` | A `GetMethod.ClassMethod`, function and class receiver |
| `_JUMP_IF_COND` | A `TestBranch`, predicate/sense and chosen successor |
| `_INSTRUMENTED_JUMP_IF_COND` | W `TestBranch`, selected committed edge |
| `_TO_BOOL_SLOT` | M `TruthTest.Slot`, checked slot and fallible truth |
| `_GET_ITER_PY_GENERATOR` | A `IterOpen.GenMethod`, construct/bind/prologue and failure cleanup |
| `_FOR_ITER_DICT_ITEMS` | A `ForIter.DictItems`, guarded pair destinations and fused successor |
| `_GUARD_TYPE_HAS_VERSION` | C nonzero-version evidence for PIC admission |
| `_LOAD_ATTR_METHOD_POLY` | A `GetMethod.PolyMethod`, typed row search and paired ownership |
| `_LOAD_ATTR_GETSET` | A `GetAttr.GetSet`, guarded descriptor invocation |
| `_CALL_PY_GENERATOR` | A `Call.Generator`, construct suspended frame and return its owner |

The deletion check must also enumerate products that add **no uop name**: local-to-temp shape variants, shape-poly variants, monitored forms, and generator `_XFER_` boundary variants. Require a family graph, binding refinement, observer wrapper, or continuation recipe for every one of the 343 old handler records. That second check catches the case a 373-name set comparison cannot catch: all the old ingredients are present, but an executing entry or a particular consumer edge has been lost. [Execution variants and generator boundaries][variants] · [Record builder][records]

[lexer]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/lexer.py:300
[contract]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_contract.py:4505
[instance]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:3246
[slotrows]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir_components.h:829
[oldadd]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/generated_ir_cases.c.h:17922
[box]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_ir_components.h:323
[home]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_interpframe.h:171
[detach]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ceval_macros.h:734
[template]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/jit_templates.c.h:45
[builder]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/jit_template_generator.py:2309
[lower]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/jit_lower.c:314
[addtest]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Lib/test/test_jit_template.py:1095
[spec]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/specialize.c:724
[fb]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Include/internal/pycore_feedback.h:79
[callkw]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:6379
[abi]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/g1a/abi.md:21
[decode]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/flowgraph.c:4527
[split]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/flowgraph.c:4695
[handoff]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/flowgraph.c:4877
[convert]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg.c:549
[liveness]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg.c:744
[physical]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg.c:1412
[swap]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg_passes.c:76
[unpackpass]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg_passes.c:510
[passes]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg.c:1765
[roles]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/regcfg_passes.c:1470
[choose]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir_assemble.c:397
[completions]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir_assemble.c:712
[payload]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir_assemble.c:364
[layout]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir_assemble.c:1027
[srcmap]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/ir_assemble.c:234
[unpackold]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Python/bytecodes.c:2351
[dis]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Lib/test/support/ir_dis.py:125
[g3]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/pyjit-link-wave1/register-semantics59/regions89/g3/design.md:93
[validation]: /home/power/code/cpython/.worktrees/python-jit/pyjit/design/validation.md
[analysis]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/analyzer.py:1114
[inventory]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_generator.py:1284
[rowparser]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_contract.py:957
[variants]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_generator.py:200
[records]: /home/power/code/cpython/.worktrees/python-boil-the-ocean/build/worktrees/regions89-g0d1/Tools/cases_generator/ir_generator.py:6131