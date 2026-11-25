"""Tests of the register instruction checker, Tools/cases_generator/rop_verify.py:
each arm and macro proved correct against its own declarations."""

import unittest
from pathlib import Path

from test import test_tools

test_tools.skip_if_missing("cases_generator")
with test_tools.imports_under_tool("cases_generator"):
    import rop_check
    import rop_syntax
    import rop_verify

ROP_DIR = Path(test_tools.basepath) / "Python" / "register_ops"


def verify_source(src: str, filename: str = "base.rop") -> list[tuple[int, int, str, str]]:
    program = rop_check.check([rop_syntax.parse(src, filename)])
    assert not program.diagnostics, [str(d) for d in program.diagnostics]
    return [(d.span.line, d.span.column, d.definition, d.message)
            for d in rop_verify.verify(program)]


# Definitions that pass the checker and use every construct it reasons
# about; each case below changes one piece and lists the diagnostics that
# result (line, column, definition, rule: message)
VERIFY_BASE = """\
enum Op(PLUS, MINUS);
family F(dst:Dst, x:Src, y:Src, fb:Fb<Binary>)
  exits(next, miss, again, raise, redirect) ops(NOP);
point F.Done(current, next_head, clear) keep() spent(y, x) defs(dst);
point F.Kept(current, site, pending) keep(x, y) spent() defs();
point F.Gone(current, site, pending) keep() spent(y, x) defs();
extern primitive both(a:B, b:B) -> Bool effects() test(inline_int(a), inline_int(b)) pending;
extern primitive scale(v:B) -> Scaled51 effects() requires(inline_int(v)) pending;
extern primitive unscale(s:Scaled51) -> O effects() ensures(result.inline_int(result)) pending;
extern primitive add(a:Scaled51, b:Scaled51) cases(fits(s:Scaled51), over(w:I64)) effects() ensures(over.outside_i51(w)) pending;
extern helper box(v:I64) cases(ok(z:O), err(e:Err)) effects(allocates, raises) requires(outside_i51(v)) ensures(ok.exact<int>(z)) pending;
extern helper op(a:B, b:B) cases(ok(z:O), err(e:Err)) effects(runs_python, raises) pending;
extern primitive is_obj(v:B) -> Bool effects() test(object(v)) pending;
extern primitive as_obj(v:B) -> Obj effects() requires(object(v)) pending;
extern helper slot_recipe(f:Fb<Binary>) cases(hit(r:Recipe<Binary>), absent()) effects(reads) feedback(read(entry.kind)) pending;
extern primitive version(o:Obj, r:Recipe<Binary>) -> Bool effects(reads) test(type_version(o, r)) pending;
extern primitive valid(o:Obj) -> Bool effects(reads) test(has_values(o)) pending;
extern primitive slot(o:Obj, r:Recipe<Binary>) -> Maybe<B> effects(reads) requires(type_version(o, r), has_values(o)) pending;
extern primitive present(m:Maybe<B>) -> Bool effects() test(present(m)) pending;
extern helper scribble() -> Unit effects(writes) pending;
extern primitive peek(v:B) -> B effects(reads) pending;
extern helper op3(a:B, b:B) cases(ok(z:O), err(e:Err), redirected(r:Route)) effects(runs_python, raises) pending;
extern helper alloc() -> Unit effects(allocates) pending;
extern helper drop_anchors(a:Anchors) -> Unit effects(releases) pending;
extern helper close_one() -> Unit effects(releases) pending;
extern helper tick<K>(f:Fb<K>) cases(quiet(), due()) effects(reads, writes) feedback(read(counter), write(counter)) pending;
extern helper install<K>(f:Fb<K>, c:Candidate<K>) cases(changed(t:Ticket<K>), same()) effects(reads, writes, allocates) feedback(read(entry), write(entry), write(handler)) pending;
extern helper commit<K>(t:Ticket<K>) -> Unit effects(writes) pending;
extern helper finish(f:Fb<Binary>) cases(done(h:Head), err(e:Err)) effects(reads, raises) pending;
extern helper fill(n:Size) cases(ok(s:Span<O>), err(e:Err, part:Span<O>)) effects(allocates, raises) pending;
extern helper drop_all(s:Span<O>) -> Unit effects(releases) pending;
extern helper new_frame() cases(ok(f:NewFrame), err(e:Err)) effects(allocates, raises) pending;
macro anchor(s:Span<Src>) -> Anchors effects() feedback() pending;
macro call_at(a:Anchors) cases(ok(z:O), err(e:Err), redirected(r:Route)) effects(runs_python, raises) feedback() borrows(a) pending;
macro retire_failed(a:Anchors, s:Span<Src>, e:Err) -> Err effects(writes, releases) feedback() spends(s) pending;
macro retire_gone(a:Anchors, r:Route) -> Route effects(releases) feedback() pending;
macro adapt(f:Fb<Binary>, x:Src, y:Src)
  cases(run(), changed(t:Ticket<Binary>))
  effects(reads, writes, allocates)
  feedback(read(entry), write(entry), write(handler))
{
  entry():
    call tick(f) {
      quiet -> Run();
      due -> Sample();
    }
  Sample():
    branch both(use(x), use(y)) -> Chosen(), Run();
  Chosen():
    let c:Candidate<Binary> = candidate(F.Int);
    call install(f, c) {
      changed -> Changed();
      same -> Run();
    }
  Changed(t:Ticket<Binary>):
    exit changed(t);
  Run():
    exit run();
}
macro take_both(h:InOut<O>, k:InOut<O>) cases(ok(a:O, b:O)) effects(writes) feedback()
{
  entry():
    let a:O = move(h);
    let b:O = move(k);
    exit ok(a, b);
}
macro fill_or_raise(n:Size) cases(ok(s:Span<O>), err(e:Err)) effects(allocates, raises, releases) feedback()
{
  entry():
    call fill(n) {
      ok -> Ok();
      err -> Failed();
    }
  Ok(s:Span<O>):
    exit ok(s);
  Failed(e:Err, part:Span<O>):
    let _:Unit = drop_all(part);
    exit err(e);
}
macro keep_anchor(a:Anchors) cases(done()) effects(releases) feedback() borrows(a)
{
  entry():
    exit done();
}
macro retire_all(a:Anchors, s:Span<Src>, e:Err) -> Err effects(writes, releases) feedback() spends(s)
{
  entry():
    let r:Err = retire_failed(a, s, e);
    exit result(r);
}
macro walk_all(s:Span<Src>) cases(done()) effects() feedback()
{
  entry():
    each s limit length body Body exhaust Done();
  Body(e:B, i:Size):
    advance Body;
  Done():
    exit done();
}
arm F.Int inline
  effects(writes, allocates, raises, releases on (dst))
  feedback()
  exits(next, miss, raise)
  ensures(next.exact<int>(dst))
{
  entry():
    let l:B = use(x);
    let r:B = use(y);
    guard both(l, r) else miss;
    let a:Scaled51 = scale(l);
    let b:Scaled51 = scale(r);
    call add(a, b) {
      fits -> Fits();
      over -> Over();
    }
  Fits(s:Scaled51):
    goto Finish(unscale(s));
  Over(w:I64):
    call box(w) {
      ok -> Finish();
      err -> Failed();
    }
  Failed(e:Err):
    exit raise(F.Kept, e);
  Finish(z:O):
    publish [dst = z] retire [spent(y), spent(x), old(dst)] at F.Done;
    exit next();
}
arm F.Slot inline
  effects(reads, writes, allocates, releases on (dst))
  feedback(read(entry.kind))
  exits(next, miss)
{
  entry():
    let v:B = use(x);
    guard is_obj(v) else miss;
    let o:Obj = as_obj(v);
    call slot_recipe(fb) {
      hit -> Check();
      absent -> Miss();
    }
  Check(r:Recipe<Binary>):
    guard version(o, r) else miss;
    guard valid(o) else miss;
    let item:Maybe<B> = slot(o, r);
    guard present(item) else miss;
    let z:O = dup(unwrap(item));
    publish [dst = z] retire [spent(y), spent(x), old(dst)] at F.Done;
    exit next();
  Miss():
    exit miss();
}
arm F.Generic shared
  effects(runs_python, raises, releases on (dst))
  feedback(read(entry), write(entry), write(handler))
  exits(next, again, raise, redirect)
{
  entry():
    call adapt(fb, x, y) {
      run -> Run();
      changed -> Retry();
    }
  Retry(t:Ticket<Binary>):
    exit again(t);
  Run():
    let a:Anchors = anchor([x, y]);
    call call_at(a) {
      ok -> Result();
      err -> Failed();
      redirected -> Moved();
    }
  Result(z:O):
    publish [dst = z] retire [roots(a), spent(y), spent(x), old(dst)] at F.Done;
    exit next();
  Failed(e:Err):
    let saved:Err = retire_failed(a, [y, x], e);
    exit raise(F.Gone, saved);
  Moved(route:Route):
    let now:Route = retire_gone(a, route);
    exit redirect(now);
}
family H(x:Src, y:Src, fb:Fb<Binary>) exits(branch, raise, enter) ops(NOP);
point H.Kept(current, site, pending) keep(x, y) spent() defs();
point H.Back(caller, next_head, clear) keep() spent(y, x) defs();
arm H.Cont split
  effects(reads, raises, releases)
  feedback()
  exits(branch, raise)
{
  entry():
    call finish(fb) {
      done -> Done();
      err -> Failed();
    }
  Done(h:Head):
    exit branch(h);
  Failed(e:Err):
    exit raise(H.Kept, e);
}
arm H.Enter split
  effects(allocates, raises, writes, releases, switches_frame)
  feedback()
  exits(enter, raise)
{
  entry():
    call new_frame() {
      ok -> Go();
      err -> Failed();
    }
  Failed(e:Err):
    exit raise(H.Kept, e);
  Go(f:NewFrame):
    publish [] retire [spent(y), spent(x)] at H.Back;
    exit enter(f) resume H.Back;
}
family Two(p:Dst, q:Dst, x:Src) exits(next) ops(NOP);
point Two.Done(current, next_head, clear) keep() spent(x) defs(p, q);
arm Two.Both inline
  effects(writes, releases on (p, q))
  feedback()
  exits(next)
{
  entry():
    let v:B = use(x);
    let o:O = dup(v);
    let o2:O = dup(v);
    publish [p = o, q = o2] retire [spent(x), old(p), old(q)] at Two.Done;
    exit next();
}
"""

VERIFY_CASES = [
    ('miss after dup',
     '    guard present(item) else miss;\n    let z:O = dup(unwrap(item));\n',
     '    let z:O = dup(unwrap(item));\n    guard present(item) else miss;\n', [
         (146, 19, 'F.Slot', 'facts: unwrap(item) requires present(item), which no dominating fact proves'),
         (147, 5, 'F.Slot', 'restart: guard present(item) else miss follows dup(unwrap(item)) at base.rop:146: the word cannot restart'),
     ]),
    ('interior across runs_python',
     '    call call_at(a) {',
     '    let w:B = peek(use(x));\n    call op3(w, w) {', [
         (168, 5, 'F.Generic', 'borrow: w (heap interior by peek(...) at base.rop:167) crosses call op3 (runs_python), which can invalidate it; pass an anchor the call cannot invalidate'),
     ]),
    ('home borrow across runs_python',
     '    call call_at(a) {',
     '    let w:B = use(x);\n    call op3(w, w) {', [
         (168, 5, 'F.Generic', 'borrow: w (read from home by use(x) at base.rop:167) crosses call op3 (runs_python), which can invalidate it; pass an anchor the call cannot invalidate'),
     ]),
    ('partial span',
     '    let _:Unit = drop_all(part);\n',
     '', [
         (77, 5, 'fill_or_raise', 'ownership: exit err leaves part (made by case err of fill at base.rop:72) unaccounted'),
     ]),
    ('enter before spending',
     '    publish [] retire [spent(y), spent(x)] at H.Back;\n    exit enter(f) resume H.Back;',
     '    exit enter(f) resume H.Back;', [
         (213, 5, 'H.Enter', 'points: exit enter resume H.Back: H.Back lists x in spent, but x is intact'),
         (213, 5, 'H.Enter', 'points: exit enter resume H.Back: H.Back lists y in spent, but y is intact'),
     ]),
    ('completion target',
     '  Done(h:Head):\n    exit branch(h);',
     '  Done(h:Head):\n    let _:Unit = close_one();\n    exit branch(h);', [
         (197, 17, 'H.Cont', 'control: h (control value by case done of finish at base.rop:192) is used after call close_one (releases) at base.rop:196, which can invalidate it'),
     ]),
    ('join extra owner',
     '  Fits(s:Scaled51):\n    goto Finish(unscale(s));',
     '  Fits(s:Scaled51):\n    let k:Anchors = anchor([x, y]);\n    goto Finish(unscale(s));', [
         (126, 3, 'F.Int', 'ownership: k (made by anchor(...) at base.rop:117) is live on the edge from Fits to Finish, which cannot see it: it is never discharged'),
     ]),
    ('join owners',
     '    guard both(l, r) else miss;\n    let a:Scaled51 = scale(l);\n    let b:Scaled51 = scale(r);\n    call add(a, b) {\n      fits -> Fits();\n      over -> Over();\n    }\n  Fits(s:Scaled51):\n',
     '    guard both(l, r) else miss;\n    let k:Anchors = anchor([x, y]);\n    let a:Scaled51 = scale(l);\n    let b:Scaled51 = scale(r);\n    call add(a, b) {\n      fits -> Fits();\n      over -> Over();\n    }\n  Fits(s:Scaled51):\n    let _:Unit = drop_anchors(k);\n', [
         (126, 5, 'F.Int', 'ownership: exit raise leaves k (made by anchor(...) at base.rop:110) unaccounted'),
         (127, 3, 'F.Int', 'join: at Finish, k is live from Over but consumed by call drop_anchors at base.rop:118 from Fits: incompatible owners'),
     ]),
    ('guard after action',
     '    guard present(item) else miss;',
     '    let _:Unit = alloc();\n    guard present(item) else miss;', [
         (147, 5, 'F.Slot', 'restart: guard present(item) else miss follows call alloc at base.rop:146: the word cannot restart'),
     ]),
    ('stale interior',
     '    guard present(item) else miss;\n',
     '    guard present(item) else miss;\n    let _:Unit = scribble();\n', [
         (148, 26, 'F.Slot', 'borrow: item (heap interior by slot(...) at base.rop:145) is used after call scribble (writes) at base.rop:147, which can invalidate it'),
     ]),
    ('unrelated recipes',
     '  Check(r:Recipe<Binary>):\n    guard version(o, r) else miss;\n    guard valid(o) else miss;\n    let item:Maybe<B> = slot(o, r);',
     '  Check(r:Recipe<Binary>):\n    guard version(o, r) else miss;\n    call slot_recipe(fb) {\n      hit -> Other();\n      absent -> Miss();\n    }\n  Other(r2:Recipe<Binary>):\n    guard valid(o) else miss;\n    let item:Maybe<B> = slot(o, r2);', [
         (150, 25, 'F.Slot', 'facts: call slot requires type_version(o, r2), which no dominating fact proves'),
     ]),
    ('duplicate move',
     '    let b:O = move(k);',
     '    let b:O = move(h);', [
         (64, 20, 'take_both', 'ownership: h is used after the move at base.rop:63 vacated it'),
     ]),
    ('again without ticket',
     '  Retry(t:Ticket<Binary>):\n    exit again(t);',
     '  Retry(t:Ticket<Binary>):\n    let _:Unit = commit(t);\n    exit again(t);', [
         (165, 5, 'F.Generic', 'again: exit again(t) has no fresh ticket: t was consumed by call commit at base.rop:164'),
         (165, 5, 'F.Generic', 'again: exit again follows call commit at base.rop:164: the ticket is not fresh'),
     ]),
    ('token unaccounted',
     '  Failed(e:Err):\n    exit raise(F.Kept, e);',
     '  Failed(e:Err):\n    let extra:O = dup(use(x));\n    exit raise(F.Kept, e);', [
         (125, 5, 'F.Int', 'ownership: exit raise leaves extra (made by dup(use(x)) at base.rop:124) unaccounted'),
     ]),
    ('point keep',
     '    exit raise(F.Kept, e);\n  Finish',
     '    exit raise(F.Gone, e);\n  Finish', [
         (124, 5, 'F.Int', 'points: raise at F.Gone: F.Gone lists x in spent, but x is intact'),
         (124, 5, 'F.Int', 'points: raise at F.Gone: F.Gone lists y in spent, but y is intact'),
     ]),
    ('undeclared effect',
     '  effects(writes, allocates, raises, releases on (dst))\n  feedback()\n  exits(next, miss, raise)',
     '  effects(writes, raises, releases on (dst))\n  feedback()\n  exits(next, miss, raise)', [
         (119, 5, 'F.Int', 'effects: call box may allocates, which effects(writes, raises, releases on (dst)) does not declare'),
     ]),
    ('requires unproven',
     '    let l:B = use(x);\n    let r:B = use(y);\n    guard both(l, r) else miss;\n    let a:Scaled51 = scale(l);',
     '    let l:B = use(x);\n    let r:B = use(y);\n    let a:Scaled51 = scale(l);\n    guard both(l, r) else miss;', [
         (109, 22, 'F.Int', 'facts: call scale requires inline_int(l), which no dominating fact proves'),
     ]),
    ('two outputs one owner',
     '[p = o, q = o2]',
     '[p = o, q = o]', [
         (227, 21, 'Two.Both', 'publish: p and q both take o: two outputs need two owners'),
         (228, 5, 'Two.Both', 'ownership: exit next leaves o2 (made by dup(v) at base.rop:226) unaccounted'),
     ]),
    ('old missing',
     'old(p), old(q)]',
     'old(p)]', [
         (227, 5, 'Two.Both', 'publish: q is bound but its displaced owner is not retired: add old(q)'),
     ]),
    ('old twice',
     'old(p), old(q)]',
     'old(p), old(q), old(q)]', [
         (227, 63, 'Two.Both', 'publish: old(q) is retired twice: one home has one displaced owner'),
     ]),
    ('use after spent',
     'at Two.Done;\n',
     'at Two.Done;\n    let w:B = use(x);\n', [
         (228, 19, 'Two.Both', 'ownership: x is used after it was spent by publish at Two.Done at base.rop:227'),
     ]),
    ('spent twice',
     'retire [spent(x), old(p)',
     'retire [spent(x), spent(x), old(p)', [
         (227, 47, 'Two.Both', 'publish: spent(x) is retired twice: a repeated operand makes one claim on its temporary'),
     ]),
    ('again not fresh',
     '  Retry(t:Ticket<Binary>):\n    exit again(t);',
     '  Retry(t:Ticket<Binary>):\n    let _:Unit = scribble();\n    exit again(t);', [
         (165, 5, 'F.Generic', 'again: exit again follows call scribble at base.rop:164: the ticket is not fresh'),
     ]),
    ('feedback undeclared',
     '  feedback(read(entry.kind))\n  exits(next, miss)',
     '  feedback()\n  exits(next, miss)', [
         (138, 5, 'F.Slot', 'feedback: call slot_recipe may read entry.kind, which feedback() does not declare'),
     ]),
    ('family exits',
     'exits(branch, raise, enter) ops(NOP);',
     'exits(branch, raise, enter) generic(Cont) ops(NOP);', [
         (201, 3, 'H.Enter', 'family: exits enter of H.Enter are not exits of the generic arm H.Cont'),
     ]),
    ('acquire twice',
     '    let o:O = dup(v);\n    let o2:O = dup(v);',
     '    let o:O = acquire(x);\n    let o2:O = acquire(x);', [
         (226, 16, 'Two.Both', 'ownership: acquire(x): x was already acquired at base.rop:225; a dying source has one owner'),
     ]),
    ('borrowed consumed',
     '  entry():\n    exit done();',
     '  entry():\n    let _:Unit = drop_anchors(a);\n    exit done();', [
         (83, 18, 'keep_anchor', 'ownership: call drop_anchors: a is borrowed from the caller (borrows); the macro cannot consume it'),
     ]),
    ('macro ensures',
     'macro take_both(h:InOut<O>, k:InOut<O>) cases(ok(a:O, b:O)) effects(writes) feedback()',
     'macro take_both(h:InOut<O>, k:InOut<O>) cases(ok(a:O, b:O)) effects(writes) feedback() ensures(ok.inline_int(a))', [
         (65, 5, 'take_both', 'facts: exit ok ensures inline_int(a), which no dominating fact proves'),
     ]),
    ('heap kill',
     '    guard valid(o) else miss;\n    let item',
     '    guard valid(o) else miss;\n    let _:Unit = close_one();\n    let item', [
         (146, 25, 'F.Slot', 'facts: call slot requires type_version(o, r), which no dominating fact proves'),
         (146, 25, 'F.Slot', 'facts: call slot requires has_values(o), which no dominating fact proves'),
         (146, 30, 'F.Slot', 'borrow: o (heap interior by as_obj(...) at base.rop:137) is used after call close_one (releases) at base.rop:145, which can invalidate it'),
         (147, 5, 'F.Slot', 'restart: guard present(item) else miss follows call close_one at base.rop:145: the word cannot restart'),
     ]),
    ('join facts',
     'ensures(next.exact<int>(dst))',
     'ensures(next.inline_int(dst))', [
         (127, 5, 'F.Int', 'facts: exit next ensures inline_int(dst), which no dominating fact proves'),
     ]),
    ('acquired at a point',
     '  entry():\n    call finish(fb) {',
     '  entry():\n    let k:O = acquire(x);\n    call finish(fb) {', [
         (191, 15, 'H.Cont', 'effects: acquire(x) may writes, which effects(reads, raises, releases) does not declare'),
         (197, 5, 'H.Cont', 'points: exit branch: x is acquired by acquire(x) at base.rop:191, but no publish names a point for it'),
         (197, 5, 'H.Cont', 'ownership: exit branch leaves k (made by acquire(x) at base.rop:191) unaccounted'),
         (199, 5, 'H.Cont', 'points: raise at H.Kept: x is acquired by acquire(x) at base.rop:191 with k outstanding; a point carries no owner, so discharge it first'),
         (199, 5, 'H.Cont', 'ownership: exit raise leaves k (made by acquire(x) at base.rop:191) unaccounted'),
     ]),
    ('loop token',
     '  Body(e:B, i:Size):\n    advance Body;',
     '  Body(e:B, i:Size):\n    let k:O = dup(e);\n    advance Body;', [
         (97, 5, 'walk_all', 'ownership: advance Body: k (made by dup(e) at base.rop:96) is live at the end of the iteration'),
     ]),
    ('spends undeclared',
     '-> Err effects(writes, releases) feedback() spends(s)\n{',
     '-> Err effects(writes, releases) feedback()\n{', [
         (89, 5, 'retire_all', 'ownership: exit result: s is spent by call retire_failed at base.rop:88, which spends() does not declare'),
     ]),
    ('reentry identity',
     '    guard both(l, r) else miss;\n    let a:Scaled51',
     '    guard both(l, r) else miss;\n    let _:Unit = close_one();\n    let a:Scaled51', [
         (111, 22, 'F.Int', 'facts: call scale requires inline_int(l), which no dominating fact proves'),
         (111, 28, 'F.Int', 'borrow: l (read from home by use(x) at base.rop:107) is used after call close_one (releases) at base.rop:110, which can invalidate it'),
         (112, 22, 'F.Int', 'facts: call scale requires inline_int(r), which no dominating fact proves'),
         (112, 28, 'F.Int', 'borrow: r (read from home by use(y) at base.rop:108) is used after call close_one (releases) at base.rop:110, which can invalidate it'),
     ]),
    ('publish releases',
     'arm Two.Both inline\n  effects(writes, releases on (p, q))',
     'arm Two.Both inline\n  effects(writes)', [
         (227, 5, 'Two.Both', 'effects: publish at Two.Done (spent(x)) may releases, which effects(writes) does not declare'),
         (227, 47, 'Two.Both', "effects: publish at Two.Done: old(p) releases the binding p displaces when it names a local's home, which may run Python: declare releases on (p)"),
         (227, 55, 'Two.Both', "effects: publish at Two.Done: old(q) releases the binding q displaces when it names a local's home, which may run Python: declare releases on (q)"),
     ]),
    ('publish point',
     'point Two.Done(current, next_head, clear) keep() spent(x) defs(p, q)',
     'point Two.Done(current, next_head, clear) keep(x) spent() defs(p)', [
         (227, 5, 'Two.Both', 'points: publish at Two.Done: Two.Done does not list q in defs, but q is defined by publish at Two.Done at base.rop:227'),
         (227, 5, 'Two.Both', 'points: publish at Two.Done: Two.Done lists x in keep, but x is spent by publish at Two.Done at base.rop:227'),
     ]),
    ('arm requires',
     '  exits(next, miss, raise)\n  ensures(next.exact<int>(dst))',
     '  exits(next, miss, raise)\n  requires(nonnull(fb))\n  ensures(next.exact<int>(dst))', [
         (104, 12, 'F.Int', "facts: arm requires nonnull(fb), which nothing before the arm's dispatch establishes"),
     ]),
    ('entry undeclared',
     '  Chosen():\n    let c:Candidate<Binary> = candidate(F.Int);',
     '  Chosen():\n    call slot_recipe(f) {\n      hit -> Hit();\n      absent -> Run();\n    }\n'
     '  Hit(r:Recipe<Binary>):\n    let c:Candidate<Binary> = candidate(F.Slot, r);', [
         (55, 31, 'adapt', 'entry: candidate(F.Slot, r): F.Slot is installed with a recipe but declares no entry(...)'),
     ]),
    ('spends unmet',
     '    let r:Err = retire_failed(a, s, e);\n    exit result(r);',
     '    let _:Unit = drop_anchors(a);\n    exit result(e);', [
         (89, 5, 'retire_all', 'ownership: exit result: spends(s) is declared, but s is intact'),
     ]),

]


# Standalone definitions, each refused by one rule of the language freeze or
# one hole the review unit (RV) confirmed; the name says which.  Each runs
# through rop_check, then rop_verify when rop_check accepts it.
LANGUAGE_CASES = [
    ('h01 block parameter home', """\
extern primitive test_int(v:B) -> Bool effects() test(inline_int(v)) pending;
extern primitive scaled(v:B) -> Scaled51 effects() requires(inline_int(v)) pending;
extern helper reenter() -> Unit effects(runs_python) pending;
family F(x:Src) exits(next, miss) ops(NOP);
arm F.Bad split effects(runs_python) feedback() exits(next, miss) {
  entry():
    goto Body(x);
  Body(h:Src):
    guard test_int(use(h)) else miss;
    let _:Unit = reenter();
    let s:Scaled51 = scaled(use(h));
    exit next();
}
""", [
        (8, 8, 'F.Bad', 'block parameter h: Src is a home; a block takes values'),
    ]),
    ('h02 unwrap moves the token', """\
extern helper is_present(m:Maybe<O>) -> Bool effects() borrows(m) test(present(m)) pending;
extern helper drop(o:O) -> Unit effects(releases) pending;
extern helper drop_maybe(o:Maybe<O>) -> Unit effects(releases) pending;
macro bad(m:Maybe<O>) cases(done()) effects(releases) feedback() {
  entry():
    branch is_present(m) -> Yes(), No();
  Yes():
    let o:O = unwrap(m);
    let _:Unit = drop(o);
    let _:Unit = drop_maybe(m);
    exit done();
  No():
    let _:Unit = drop_maybe(m);
    exit done();
}
""", [
        (10, 18, 'bad', 'ownership: call drop_maybe: m was already consumed by unwrap(m) at t.rop:8'),
    ]),
    ('h03 each over owners', """\
extern helper drop(o:O) -> Unit effects(releases) pending;
extern helper drop_all(s:Span<O>) -> Unit effects(releases) pending;
macro bad(s:Span<O>) cases(done()) effects(releases) feedback() {
  entry():
    each s limit 4 body Body exhaust Done();
  Body(o:O, i:Size):
    let _:Unit = drop(o);
    advance Body;
  Done():
    let _:Unit = drop_all(s);
    exit done();
}
""", [
        (5, 5, 'bad', 'each over s: its items (O) carry obligations; each walks values only'),
    ]),
    ('h04 completion at redirect', """\
extern helper eat(s:Src) -> Unit effects(writes, releases) spends(s) pending;
extern helper redirect_status(f:Frame) cases(running(), redirected(r:Route)) effects(reads) pending;
family F(x:Src) exits(next, redirect) ops(NOP);
arm F.Bad split effects(reads, writes, releases) feedback() exits(next, redirect) {
  entry():
    let _:Unit = eat(x);
    call redirect_status(frame) {
      running -> Next();
      redirected -> Gone();
    }
  Next():
    exit next();
  Gone(r:Route):
    exit redirect(r);
}
""", [
        (12, 5, 'F.Bad', 'points: exit next: x is spent by call eat at t.rop:6, but no publish names a point for it'),
        (14, 5, 'F.Bad', 'points: exit redirect: x is spent by call eat at t.rop:6, but no publish names a point for it'),
    ]),
    ('h05 span of borrows', """\
extern helper call_all(s:Span<B>) -> Unit effects(runs_python) pending;
extern helper call_one(b:B) -> Unit effects(runs_python) pending;
family F(x:Src) exits(next) ops(NOP);
arm F.Bad split effects(runs_python) feedback() exits(next) {
  entry():
    let v:B = use(x);
    let _:Unit = call_all([v]);
    exit next();
}
arm F.Refused split effects(runs_python) feedback() exits(next) {
  entry():
    let v:B = use(x);
    let _:Unit = call_one(v);
    exit next();
}
""", [
        (7, 18, 'F.Bad', 'borrow: v (read from home by use(x) at t.rop:6) crosses call call_all (runs_python), which can invalidate it; pass an anchor the call cannot invalidate'),
        (13, 18, 'F.Refused', 'borrow: v (read from home by use(x) at t.rop:12) crosses call call_one (runs_python), which can invalidate it; pass an anchor the call cannot invalidate'),
    ]),
    ('h06 feedback write writes', """\
extern helper note(f:Fb<Binary>) -> Unit effects(reads) feedback(write(entry)) pending;
extern primitive test_int(v:B) -> Bool effects() test(inline_int(v)) pending;
family F(x:Src, fb:Fb<Binary>) exits(next, miss) ops(NOP);
arm F.Bad split effects(reads) feedback(write(entry)) exits(next, miss) {
  entry():
    let _:Unit = note(fb);
    guard test_int(use(x)) else miss;
    exit next();
}
""", [
        (1, 42, 'note', 'a feedback write writes memory: declare writes'),
        (4, 17, 'F.Bad', 'a feedback write writes memory: declare writes'),
    ]),
    ('h08 acquire writes', """\
family F(x:Src) exits(exit) ops(NOP);
arm F.Bad split effects() feedback() exits(exit) {
  entry():
    let z:O = acquire(x);
    exit exit(z);
}
""", [
        (4, 15, 'F.Bad', 'effects: acquire(x) may writes, which effects() does not declare'),
    ]),
    ('h10 feedback through a block parameter', """\
extern helper write(f:Fb<Binary>) -> Unit effects(writes) feedback(write(entry)) pending;
family F(fb:Fb<Binary>) exits(next) ops(NOP);
arm F.Bad shared effects(writes) feedback() exits(next) {
  entry():
    goto Body(fb);
  Body(f:Fb<Binary>):
    let _:Unit = write(f);
    exit next();
}
""", [
        (7, 18, 'F.Bad', 'feedback: call write may write entry, which feedback() does not declare'),
    ]),
    ('lead1 one owner twice in a span', """\
extern helper drop(s:Span<O>) -> Unit effects(releases) pending;
macro bad(o:O) cases(done()) effects(releases) feedback() {
  entry():
    let _:Unit = drop([o, o]);
    exit done();
}
""", [
        (4, 27, 'bad', 'ownership: [...] at t.rop:4: o was already consumed by [...] at t.rop:4 at t.rop:4'),
    ]),
    ('lead2 feedback through a let', """\
extern helper write(f:Fb<Binary>) -> Unit effects(writes) feedback(write(entry)) pending;
family F(fb:Fb<Binary>) exits(next) ops(NOP);
arm F.Bad shared effects(writes) feedback() exits(next) {
  entry():
    let other:Fb<Binary> = fb;
    let _:Unit = write(other);
    exit next();
}
""", [
        (6, 18, 'F.Bad', 'feedback: call write may write entry, which feedback() does not declare'),
    ]),
    ('lead5 publish binding is a let name', """\
extern primitive int_scaled(v:B) -> Scaled51 effects() pending;
extern primitive from_int_scaled(s:Scaled51) -> O effects() pending;
family F(dst:Dst, x:Src) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep() spent(x) defs(dst);
arm F.Bad inline effects(writes, releases on (dst)) feedback() exits(next) {
  entry():
    publish [dst = from_int_scaled(int_scaled(use(x)))] retire [spent(x), old(dst)] at F.Done;
    exit next();
}
""", [
        (7, 20, 'F.Bad', "publish [dst = from_int_scaled(int_scaled(use(x)))]: a binding's value is a let name or null"),
    ]),
    ('join needs a fresh checkpoint', """\
extern primitive test(v:B) -> Bool effects() pending;
family F(dst:Dst, x:Src) exits(next) ops(NOP);
point F.A(current, next_head, clear) keep() spent(x) defs(dst);
point F.B(current, next_head, clear) keep() spent(x) defs(dst);
arm F.Bad inline effects(writes, releases on (dst)) feedback() exits(next) {
  entry():
    let v:B = use(x);
    let o:O = dup(v);
    branch test(v) -> Left(o), Right(o);
  Left(a:O):
    publish [dst = a] retire [spent(x), old(dst)] at F.A;
    goto Out();
  Right(b:O):
    publish [dst = b] retire [spent(x), old(dst)] at F.B;
    goto Out();
  Out():
    exit next();
}
""", [
        (17, 5, 'F.Bad', 'points: exit next: the paths into it published different points (F.A, F.B); a joined exit needs a fresh checkpoint on each path'),
    ]),
    ('event row needs instrumented', """\
extern helper event(at:Site) cases(ok(), err(e:Err)) effects(runs_python, raises) requires(instrumented(at)) pending;
family F(x:Src) exits(next, raise) ops(NOP);
point F.Failed(current, site, pending) keep(x) spent() defs();
arm F.Bad split effects(runs_python, raises) feedback() exits(next, raise) {
  entry():
    call event(site) {
      ok -> Go();
      err -> Failed();
    }
  Go():
    exit next();
  Failed(e:Err):
    exit raise(F.Failed, e);
}
""", [
        (6, 5, 'F.Bad', 'facts: call event requires instrumented(site), which no dominating fact proves'),
    ]),
    ('row takes no destination', """\
extern helper put(d:Dst, v:O) -> Unit effects(writes) pending;
macro m(d:Span<Dst>) -> Unit effects() feedback() pending;
""", [
        (1, 19, 'put', 'parameter d:Dst: a row cannot write a home; return the value and publish it'),
        (2, 9, 'm', 'parameter d:Span<Dst>: a row cannot write a home; return the value and publish it'),
    ]),
    ('old of a local runs python', """\
extern helper take(fr:Frame, x:Local) cases(ok(old:O), err(e:Err)) effects(reads, writes, raises) spends(x) pending;
family F(local:Local) exits(next, raise) ops(NOP);
point F.Unbound(current, site, pending) keep() spent(local) defs();
point F.Done(current, next_head, clear) keep() spent(local) defs(local);
arm F.Bad shared effects(reads, writes, raises, releases) feedback() exits(next, raise) {
  entry():
    call take(frame, local) {
      ok -> Done();
      err -> Failed();
    }
  Failed(e:Err):
    exit raise(F.Unbound, e);
  Done(old:O):
    publish [local = null] retire [old(local), owned(old)] at F.Done;
    exit next();
}
""", [
        (14, 36, 'F.Bad', "effects: publish at F.Done (old(local) closes a local's binding) may runs_python, which effects(reads, writes, raises, releases) does not declare"),
    ]),
    ('vector binding operands', """\
family F(dsts:Span<Dst>, srcs:Span<Src>, x:Src) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep() spent(srcs) defs(dsts);
arm F.Bad shared effects(writes, releases) feedback() exits(next) {
  entry():
    publish [dsts[] = x[]] retire [old(dsts)] at F.Done;
    exit next();
}
""", [
        (5, 23, 'F.Bad', 'publish [dsts[] = x[]]: x is not a Span<Src> operand'),
        (5, 36, 'F.Bad', 'old(dsts): a vector binding writes vacant destinations; it displaces no owner'),
    ]),
    ('generic clause', """\
family F(x:Src) exits(next, miss) generic(Fast) ops(NOP);
family G(x:Src) exits(next, miss) generic(Absent) ops(NOP);
arm F.Fast inline effects() feedback() exits(next, miss) {
  entry():
    exit next();
}
arm G.Fast inline effects() feedback() exits(next, miss) {
  entry():
    exit next();
}
""", [
        (1, 1, 'F', 'generic(Fast): F.Fast is inline; the generic arm is shared, or split when it leaves the frame'),
        (1, 1, 'F', 'generic(Fast): F.Fast misses; the generic arm is where a miss restarts'),
        (2, 1, 'G', 'generic(Absent): no arm G.Absent'),
        (3, 19, 'F.Fast', 'declared exit miss is not reached'),
        (7, 19, 'G.Fast', 'declared exit miss is not reached'),
    ]),
    ('select gives its fact', """\
extern primitive peek(v:B) -> B effects() requires(nonnull(v)) pending;
family F(dst:Dst, local:Src) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep() spent(local) defs(dst);
arm F.Read inline effects() feedback() exits(next) {
  entry():
    let v:B = use(local);
    let w:B = peek(v);
    exit next();
}
""", [
        (7, 15, 'F.Read', 'facts: call peek requires nonnull(v), which no dominating fact proves'),
    ]),
    ('optional operand', """\
extern helper eat(s:Src) -> Unit effects(writes, releases) spends(s) pending;
extern helper present_src(x:Maybe<Src>) -> Bool effects(reads) test(present(x)) pending;
family F(x:Maybe<Src>) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep() spent(x) defs();
arm F.Bad shared effects(writes, releases) feedback() exits(next) {
  entry():
    let v:B = use(x);
    let o:O = acquire(x);
    let _:Unit = eat(x);
    publish [] retire [owned(o)] at F.Done;
    exit next();
}
""", [
        (7, 15, 'F.Bad', 'facts: use(x) reads the optional operand x: it requires present(x), which no dominating fact proves'),
        (8, 15, 'F.Bad', 'facts: acquire(x) reads the optional operand x: it requires present(x), which no dominating fact proves'),
        (9, 18, 'F.Bad', 'facts: call eat reads the optional operand x: it requires present(x), which no dominating fact proves'),
    ]),
    ('owners binding values', """\
extern helper split(v:B) -> Span<O> effects(allocates) pending;
family F(dsts:Span<Dst>, srcs:Span<Src>, x:Src) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep(x) spent(srcs) defs(dsts);
arm F.Bad shared effects(allocates, writes, releases) feedback() exits(next) {
  entry():
    let o:O = dup(use(x));
    publish [dsts[] = srcs, dsts[] = o] retire [owned(o)] at F.Done;
    exit next();
}
""", [
        (7, 23, 'F.Bad', 'publish [dsts[] = srcs]: an operand span binds as srcs[]'),
        (7, 29, 'F.Bad', 'publish binds dsts twice'),
        (7, 38, 'F.Bad', 'publish [dsts[] = o]: o is O, expected Span<O>'),
    ]),
    ('count of a span', """\
family F(dsts:Span<Dst>, x:Src) exits(next) ops(NOP);
arm F.Bad inline effects() feedback() exits(next) {
  entry():
    let n:Size = count(x);
    let m:Size = count(dsts, x);
    exit next();
}
""", [
        (4, 18, 'F.Bad', 'count(x): Src is not a Span'),
        (5, 18, 'F.Bad', 'count takes one argument'),
    ]),
    ('owners binding retires', """\
extern helper split(v:B) -> Span<O> effects(allocates) pending;
family F(dsts:Span<Dst>, x:Src) exits(next) ops(NOP);
point F.Done(current, next_head, clear) keep(x) spent() defs(dsts);
arm F.Unretired shared effects(allocates, writes) feedback() exits(next) {
  entry():
    let v:Span<O> = split(use(x));
    publish [dsts[] = v] retire [] at F.Done;
    exit next();
}
arm F.Undeclared shared effects(allocates, writes, releases) feedback() exits(next) {
  entry():
    let v:Span<O> = split(use(x));
    let n:Size = count(dsts);
    publish [dsts[] = v] retire [old(dsts)] at F.Done;
    exit next();
}
""", [
        (7, 5, 'F.Unretired', 'publish: dsts is bound but its displaced owner is not retired: add old(dsts)'),
        (14, 34, 'F.Undeclared', 'effects: publish at F.Done: old(dsts) releases the binding dsts displaces when it names a local\'s home, which may run Python: declare releases on (dsts)'),
    ]),
    ('presence of no optional operand', """\
family G(x:Src) exits(next) ops(NOP);
arm G.Bad inline select(present) effects() feedback() exits(next) {
  entry():
    exit next();
}
arm G.Vacant inline select(absent) effects() feedback() exits(next) {
  entry():
    exit next();
}
""", [
        (2, 1, 'G.Bad', "select names an optional operand's presence, but G has no Maybe<Src> operand"),
        (6, 1, 'G.Vacant', "select names an optional operand's presence, but G has no Maybe<Src> operand"),
    ]),

]


def language_diagnostics(src: str) -> list[tuple[int, int, str, str]]:
    program = rop_check.check([rop_syntax.parse(src, "t.rop")])
    found = list(program.diagnostics)
    if not found:
        found = rop_verify.verify(program)
    return [(d.span.line, d.span.column, d.definition, d.message) for d in found]


class LanguageTests(unittest.TestCase):
    def test_language_fixtures(self):
        for name, src, expected in LANGUAGE_CASES:
            with self.subTest(name):
                self.assertTrue(expected)
                self.assertEqual(sorted(language_diagnostics(src)), sorted(expected))

    def test_entry_matches_the_classifier_case(self):
        src = VERIFY_BASE.replace("arm F.Slot inline\n", "arm F.Slot inline entry(hit)\n", 1)
        src = src.replace('  Chosen():\n    let c:Candidate<Binary> = candidate(F.Int);', '  Chosen():\n    call slot_recipe(f) {\n      hit -> Hit();\n      absent -> Run();\n    }\n' '  Hit(r:Recipe<Binary>):\n    let c:Candidate<Binary> = candidate(F.Slot, r);', 1)
        self.assertEqual(verify_source(src), [])
        src = src.replace("entry(hit)", "entry(absent)", 1)
        self.assertEqual(verify_source(src), [
            (55, 31, 'adapt', 'entry: candidate(F.Slot, r): F.Slot reads entry(absent), '
             'but the recipe is the payload of case hit')])

    def test_select_grants_its_fact(self):
        src = dict((n, s) for n, s, _ in LANGUAGE_CASES)["select gives its fact"]
        src = src.replace("arm F.Read inline effects", "arm F.Read inline select(bound) effects")
        src += ("arm F.Checked inline select(checked) effects() feedback() exits(next) {\n"
                "  entry():\n    exit next();\n}\n")
        self.assertEqual(language_diagnostics(src), [])

    def test_owners_binding(self):
        src = dict((n, s) for n, s, _ in LANGUAGE_CASES)["owners binding retires"]
        src = src[:src.index("arm F.Unretired")] + src[src.index("arm F.Undeclared"):]
        src = src.replace("shared effects(allocates, writes, releases)",
                          "shared effects(allocates, writes, releases on (dsts), runs_python)")
        self.assertEqual(language_diagnostics(src), [])

    def test_present_grants_an_optional_operand(self):
        src = dict((n, s) for n, s, _ in LANGUAGE_CASES)["optional operand"]
        selected = src.replace("arm F.Bad shared effects", "arm F.Bad shared select(present) effects")
        selected += ("arm F.Absent inline select(absent) effects() feedback() exits(next) {\n"
                     "  entry():\n    exit next();\n}\n")
        self.assertEqual(language_diagnostics(selected), [])
        tested = src.replace("  entry():\n    let v:B",
                             "  entry():\n    branch present_src(x) -> Yes(), No();\n"
                             "  No():\n    exit next();\n  Yes():\n    let v:B", 1)
        tested = tested.replace("shared effects(writes", "shared effects(reads, writes", 1)
        self.assertEqual(language_diagnostics(tested), [])


# A source acquired from an operand, then a raise: a checkpoint carries no
# owner, so the acquired owner must be discharged before the raise
HELD = """\
family H(x:Src, fb:Fb<Binary>) exits(next, raise) ops(NOP);
point H.Held(current, site, pending) keep() spent() defs();
point H.Done(current, next_head, clear) keep() spent(x) defs();
extern helper keep_o(o:O) cases(ok(), err(e:Err)) effects(raises, releases) pending;
arm H.A inline effects(writes, releases, raises) feedback() exits(next, raise)
{
  entry():
    let k:O = acquire(x);
    call check(fb) {
      ok -> Ok();
      err -> Failed();
    }
  Ok():
    publish [] retire [spent(x), owned(k)] at H.Done;
    exit next();
  Failed(e:Err):
    exit raise(H.Held, e);
}
extern helper check(f:Fb<Binary>) cases(ok(), err(e:Err)) effects(raises) pending;
"""


# A borrow projected from an anchor lives as long as the anchor token
ANCHORED = VERIFY_BASE + """\
extern helper arg(a:Anchors, i:Size) -> B effects(reads) feedback() borrows(a) pending;
macro anchored_across(a:Anchors) cases(done()) effects(writes, reads) feedback() borrows(a)
{
  entry():
    let w:B = arg(a, 0);
    let _:Unit = scribble();
    let v:B = peek(w);
    exit done();
}
macro anchored_after_close(a:Anchors) cases(done()) effects(releases, reads) feedback()
{
  entry():
    let w:B = arg(a, 0);
    let _:Unit = drop_anchors(a);
    let v:B = peek(w);
    exit done();
}
"""


# Frame transfers: a call that enters its child and resumes at a point, and
# a return whose arms are selected statically by the kind of frame
TRANSFERS = """\
family K(dst:Dst, fn:Src, a:Args, fb:Fb<Call>) exits(enter, miss) ops(NOP);
point K.Returned(caller, next_head, clear) keep() spent(fn, a) defs(dst);
point K.Early(current, next_head, clear) keep(fn, a) spent() defs(dst);
extern primitive admits(v:B) -> Bool effects() pending;
extern helper push(t:Thread, fn:Src, a:Args) -> NewFrame effects(writes, releases) spends(fn, a) pending;
arm K.Exact split
  effects(writes, releases, switches_frame)
  feedback()
  exits(enter, miss)
{
  entry():
    guard admits(use(fn)) else miss;
    let c:NewFrame = push(thread, fn, a);
    exit enter(c) resume K.Returned;
}
family R(value:Src) exits(return, exit, raise) ops(NOP);
point R.Delivered(caller, resume_head, clear) keep() spent(value) defs();
point R.ConsumerFailed(consumer, saved_origin, pending) keep() spent(value) defs();
extern helper pin_departing() -> Unit effects(writes) pending;
extern primitive caller_of(f:Frame) -> Frame effects(reads) pending;
extern helper unlink(t:Thread, f:Frame) -> DeadFrame effects(writes) pending;
extern helper retire_frame(t:Thread, d:DeadFrame) -> Unit effects(releases) pending;
extern helper deliver(c:Frame, v:O) -> Head effects(writes) pending;
extern helper gen_return(t:Thread, v:O) cases(return(f:Frame, h:Head), raise(e:Err))
  effects(writes, releases, raises, switches_frame) c GEN_RETURN as transfer
  evidence(review: "r", contract: "c");
arm R.Ordinary split select(thread_frame)
  effects(reads, writes, releases, switches_frame)
  feedback()
  exits(return)
{
  entry():
    let _:Unit = pin_departing();
    let v:O = acquire(value);
    let caller:Frame = caller_of(frame);
    let d:DeadFrame = unlink(thread, frame);
    let _:Unit = retire_frame(thread, d);
    let h:Head = deliver(caller, v);
    publish [] retire [] at R.Delivered;
    exit return(caller, h);
}
arm R.Generator split select(generator_frame)
  effects(writes, releases, raises, switches_frame)
  feedback()
  exits(return, raise)
{
  entry():
    let _:Unit = pin_departing();
    let v:O = acquire(value);
    call gen_return(thread, v) {
      return -> Returned();
      raise -> Failed();
    }
  Returned(f:Frame, h:Head):
    exit return(f, h);
  Failed(e:Err):
    exit raise(R.ConsumerFailed, e);
}
arm R.External split select(entry_frame)
  effects(writes)
  feedback()
  exits(exit)
{
  entry():
    let v:O = acquire(value);
    exit exit(v);
}
"""

TRANSFER_CASES = [
    ('resume point role',
     'point K.Returned(caller, next_head,', 'point K.Returned(current, next_head,', [
         (14, 5, 'K.Exact', 'resume K.Returned: the point is (current, next_head); a resume point is (caller, next_head)'),
     ]),
    ('resume two definitions',
     'family K(dst:Dst, fn:Src, a:Args, fb:Fb<Call>) exits(enter, miss) ops(NOP);\n'
     'point K.Returned(caller, next_head, clear) keep() spent(fn, a) defs(dst)',
     'family K(dst:Dst, other:Dst, fn:Src, a:Args, fb:Fb<Call>) exits(enter, miss) ops(NOP);\n'
     'point K.Returned(caller, next_head, clear) keep() spent(fn, a) defs(dst, other)', [
         (14, 5, 'K.Exact', "resume K.Returned: the child's value is one definition, not 2"),
     ]),
    ('enter without resume',
     '    exit enter(c) resume K.Returned;', '    exit enter(c);', [
         (14, 5, 'K.Exact', "an arm's exit enter or yield names its resume point (exit enter(c) resume P, exit yield(v) resume P), and no other exit does"),
     ]),
    ('resume on another exit',
     '    exit exit(v);', '    exit exit(v) resume K.Returned;', [
         (66, 5, 'R.External', "an arm's exit enter or yield names its resume point (exit enter(c) resume P, exit yield(v) resume P), and no other exit does"),
         (66, 5, 'R.External', 'point K.Returned is not a point of R'),
     ]),
    ('select twice in a dimension',
     'arm R.External split select(entry_frame)',
     'arm R.External split select(entry_frame, thread_frame)', [
         (16, 1, 'R', 'selection (entry_frame) is admitted by 0 arms; exactly one admits it'),
         (59, 1, 'R.External', 'select names two members of dimension frame; one holds'),
     ]),
    ('select gap',
     'arm R.External split select(entry_frame)',
     'arm R.External split select(generator_frame)', [
         (16, 1, 'R', 'selection (entry_frame) is admitted by 0 arms; exactly one admits it'),
         (16, 1, 'R', 'selection (generator_frame) is admitted by 2 arms (R.Generator, R.External); exactly one admits it'),
     ]),
    ('transfer case that is no exit',
     'cases(return(f:Frame, h:Head), raise(e:Err))',
     'cases(back(f:Frame, h:Head), raise(e:Err))', [
         (24, 47, 'gen_return', 'case back of a transfer row names the exit its callee performs (one of: jump, branch, return, redirect, exit, raise)'),
         (50, 5, 'R.Generator', 'call gen_return at t.rop:50 has no edge for back'),
         (51, 7, 'R.Generator', 'call gen_return at t.rop:50: gen_return has no case return (cases: back, raise)'),
     ]),
    ('transfer case payload',
     'cases(return(f:Frame, h:Head), raise(e:Err))',
     'cases(return(f:Frame), raise(e:Err))', [
         (24, 47, 'gen_return', 'case return: exit return carries (Frame, Head)'),
         (51, 17, 'R.Generator', 'edge return -> Returned takes 2 arguments, not 1 (payload 1 + 0 given)'),
     ]),
    ('transfer edge with a statement',
     '  Returned(f:Frame, h:Head):\n    exit return(f, h);',
     '  Returned(f:Frame, h:Head):\n    let _:Unit = pin_departing();\n    exit return(f, h);', [
         (51, 7, 'R.Generator', "call gen_return at t.rop:50: edge return: the callee performs exit return, so Returned is entered by this edge only and is exactly `exit return(...)` of the case's payload"),
     ]),
    ('transfer edge to another exit',
     '      raise -> Failed();\n    }\n  Returned',
     '      raise -> Returned();\n    }\n  Returned', [
         (43, 3, 'R.Generator', 'declared exit raise is not reached'),
         (51, 7, 'R.Generator', "call gen_return at t.rop:50: edge return: the callee performs exit return, so Returned is entered by this edge only and is exactly `exit return(...)` of the case's payload"),
         (52, 7, 'R.Generator', "call gen_return at t.rop:50: edge raise: the callee performs exit raise, so Returned is entered by this edge only and is exactly `exit raise(...)` of the case's payload"),
         (52, 16, 'R.Generator', 'edge raise -> Returned takes 2 arguments, not 1 (payload 1 + 0 given)'),
         (56, 3, 'R.Generator', 'block Failed is unreachable'),
     ]),
    ('defined before the child returns',
     '    let c:NewFrame = push(thread, fn, a);\n',
     '    let o:O = dup(use(fn));\n'
     '    publish [dst = o] retire [old(dst)] at K.Early;\n'
     '    let c:NewFrame = push(thread, fn, a);\n', [
         (14, 31, 'K.Exact', "effects: publish at K.Early: old(dst) releases the binding dst displaces when it names a local's home, which may run Python: declare releases on (dst)"),
         (16, 5, 'K.Exact', 'points: exit enter resume K.Returned: dst is defined by publish at K.Early at t.rop:14 before the frame resumes there; the resumption defines it'),
     ]),
    ('spent source missing',
     ') -> NewFrame effects(writes, releases) spends(fn, a) pending;',
     ') -> NewFrame effects(writes, releases) spends(fn) pending;', [
         (14, 5, 'K.Exact', 'points: exit enter resume K.Returned: K.Returned lists a in spent, but a is intact'),
     ]),
    ('return unpinned',
     '    let _:Unit = pin_departing();\n    let v:O = acquire(value);\n    let caller',
     '    let v:O = acquire(value);\n    let caller', [
         (39, 5, 'R.Ordinary', 'control: exit return releases the departing frame, whose code is not pinned on this path: call pin_departing() first'),
     ]),
    ('transfer unpinned',
     '    let _:Unit = pin_departing();\n    let v:O = acquire(value);\n    call gen_return',
     '    let v:O = acquire(value);\n    call gen_return', [
         (54, 5, 'R.Generator', 'control: exit return releases the departing frame, whose code is not pinned on this path: call pin_departing() first'),
     ]),
    ('transfer leaves an owner',
     '    call gen_return(thread, v) {',
     '    let w:O = dup(use(value));\n    call gen_return(thread, v) {', [
         (56, 5, 'R.Generator', 'ownership: exit return leaves w (made by dup(use(value)) at t.rop:50) unaccounted'),
         (58, 5, 'R.Generator', 'ownership: exit raise leaves w (made by dup(use(value)) at t.rop:50) unaccounted'),
     ]),
]


def transfer_diagnostics(src: str) -> list[tuple[int, int, str, str]]:
    program = rop_check.check([rop_syntax.parse(src, "t.rop")])
    found = program.diagnostics + rop_verify.verify(program)
    return [(d.span.line, d.span.column, d.definition, d.message) for d in found]


class TransferTests(unittest.TestCase):
    def test_transfers_are_clean(self):
        self.assertEqual(transfer_diagnostics(TRANSFERS), [])

    def test_transfer_fixtures(self):
        for name, old, new, expected in TRANSFER_CASES:
            with self.subTest(name):
                self.assertIn(old, TRANSFERS)
                found = transfer_diagnostics(TRANSFERS.replace(old, new, 1))
                self.assertTrue(expected)
                self.assertEqual(sorted(found), sorted(expected))


class VerifyTests(unittest.TestCase):
    def test_anchored_borrow(self):
        found = verify_source(ANCHORED)
        self.assertEqual(len(found), 1, found)
        (line, column, definition, message) = found[0]
        self.assertEqual(definition, "anchored_after_close")
        self.assertTrue(message.startswith("borrow: w (anchored borrow by arg(...)"),
                        message)

    def test_base_is_clean(self):
        self.assertEqual(verify_source(VERIFY_BASE), [])

    def test_negative_fixtures(self):
        for name, old, new, expected in VERIFY_CASES:
            with self.subTest(name):
                self.assertIn(old, VERIFY_BASE)
                found = verify_source(VERIFY_BASE.replace(old, new, 1))
                self.assertTrue(expected)
                self.assertEqual(sorted(found), sorted(expected))

    def test_every_rule_has_a_fixture(self):
        rules = {message.split(":")[0] for _, _, _, diags in VERIFY_CASES
                 for _, _, _, message in diags}
        self.assertEqual(rules, {r.value for r in rop_verify.Rule})

    def test_diagnostic_format(self):
        src = VERIFY_BASE.replace("    let b:O = move(k);", "    let b:O = move(h);", 1)
        program = rop_check.check([rop_syntax.parse(src, "core.rop")])
        self.assertEqual([str(d) for d in rop_verify.verify(program)], [
            "core.rop:64:20: take_both: ownership: h is used after the move at "
            "core.rop:63 vacated it"])

    def test_a_point_carries_no_owner(self):
        self.assertEqual(verify_source(HELD), [
            (17, 5, "H.A", "points: raise at H.Held: x is acquired by acquire(x) at "
             "base.rop:8 with k outstanding; a point carries no owner, so discharge "
             "it first"),
            (17, 5, "H.A", "ownership: exit raise leaves k (made by acquire(x) at "
             "base.rop:8) unaccounted"),
        ])



# W2's transfers (WR): an exit that unwinds an exception the word already
# reported
UNWIND = """\
family U(exc:Src) exits(unwind, raise) ops(NOP);
point U.Raised(current, site, pending) keep(exc) spent() defs();
extern helper restore(t:Thread, e:Src) cases(err(x:Err)) effects(runs_python, raises)
  c RESTORE as raised evidence(review: "r", contract: "c");
arm U.Restore split
  effects(runs_python, raises)
  feedback()
  exits(unwind)
{
  entry():
    call restore(thread, exc) {
      err -> Failed();
    }
  Failed(e:Err):
    exit unwind(U.Raised, e);
}
"""

UNWIND_CASES = [
    ('unwind to a point that is not pending',
     'point U.Raised(current, site, pending)', 'point U.Raised(current, site, clear)', [
         (15, 5, 'U.Restore', "exit unwind(U.Raised, ...): the exception is pending as the frame unwinds, but U.Raised's exception role is clear"),
     ]),
    ('unwind not declared',
     '  exits(unwind)\n', '  exits(raise)\n', [
         (6, 3, 'U.Restore', 'declared exit raise is not reached'),
         (15, 5, 'U.Restore', 'exit unwind: unwind is not a declared exit of U.Restore (raise)'),
     ]),
    ('unwind without raises',
     '  effects(runs_python, raises)\n  feedback()', '  effects(writes)\n  feedback()', [
         (11, 5, 'U.Restore', 'effects: call restore may raises, which effects(writes) does not declare'),
         (11, 5, 'U.Restore', 'effects: call restore may runs_python, which effects(writes) does not declare'),
         (15, 5, 'U.Restore', 'effects: exit unwind may raises, which effects(writes) does not declare'),
     ]),
    ('unwind in a transfer row',
     'cases(err(x:Err)) effects(runs_python, raises)\n  c RESTORE as raised',
     'cases(unwind(x:Err)) effects(runs_python, raises)\n  c RESTORE as transfer', [
         (3, 46, 'restore', 'case unwind of a transfer row names the exit its callee performs (one of: jump, branch, return, redirect, exit, raise)'),
         (11, 5, 'U.Restore', 'call restore at t.rop:11 has no edge for unwind'),
         (12, 7, 'U.Restore', 'call restore at t.rop:11: restore has no case err (cases: unwind)'),
     ]),
]


class UnwindTests(unittest.TestCase):
    def test_unwind_is_clean(self):
        self.assertEqual(transfer_diagnostics(UNWIND), [])

    def test_unwind_fixtures(self):
        for name, old, new, expected in UNWIND_CASES:
            with self.subTest(name):
                self.assertIn(old, UNWIND)
                found = transfer_diagnostics(UNWIND.replace(old, new, 1))
                self.assertTrue(expected)
                self.assertEqual(sorted(found), sorted(expected))


GROUPS = """\
family G(item:Dst, pair:IterPair, done:NearHead) exits(next, branch) ops(NOP);
point G.Stepped(current, next_head, clear) keep(pair.receiver) spent(pair.cursor) defs(item, pair.cursor);
point G.Done(current, next_head, clear) keep() spent(pair) defs();
extern helper next_item(r:B, c:B) cases(hit(v:O), absent()) effects(reads)
  c NEXT_ITEM as bool_out_hit
  evidence(review: "r", contract: "c");
extern helper next_index(c:B) -> O effects(allocates)
  c NEXT_INDEX as value
  evidence(review: "r", contract: "c");
arm G.Step split effects(reads, allocates, writes, releases on (item)) feedback() exits(next, branch)
{
  entry():
    let r:B = use(pair.receiver);
    let c:B = use(pair.cursor);
    call next_item(r, c) { hit -> Hit(); absent -> Exhausted(); }
  Hit(v:O):
    let n:O = next_index(c);
    publish [item = v, pair.cursor = n] retire [old(item), old(pair.cursor)] at G.Stepped;
    exit next();
  Exhausted():
    publish [] retire [spent(pair)] at G.Done;
    exit branch(done);
}
family W(window:SendWindow) exits(next) ops(NOP);
point W.Done(current, next_head, clear) keep(window.receiver) spent(window.cursor, window.value) defs();
arm W.Drop shared effects(writes, releases) feedback() exits(next)
{
  entry():
    publish [] retire [spent(window.value), spent(window.cursor)] at W.Done;
    exit next();
}
"""


GROUPS_CASES = [
    ('publish without old',
     'retire [old(item), old(pair.cursor)]',
     'retire [old(item)]', [
         (18, 5, 'G.Step', 'publish: pair.cursor is bound but its displaced owner is not retired: add old(pair.cursor)'),
         (18, 5, 'G.Step', 'points: publish at G.Stepped: G.Stepped lists pair.cursor in spent, but pair.cursor is intact'),
     ]),
    ('old of a group',
     'old(pair.cursor)]',
     'old(pair)]', [
         (18, 60, 'G.Step', 'old(pair): pair is not a destination operand'),
     ]),
    ('use of a whole group',
     'let c:B = use(pair.cursor);',
     'let c:B = use(pair);', [
         (14, 19, 'G.Step', 'pair is IterPair, expected Src'),
     ]),
    ('unknown projection',
     'let c:B = use(pair.cursor);',
     'let c:B = use(pair.value);', [
         (14, 19, 'G.Step', 'unknown name pair.value'),
     ]),
    ('point keeps the republished cursor',
     'keep(pair.receiver) spent(pair.cursor) defs(item, pair.cursor)',
     'keep(pair) spent() defs(item, pair.cursor)', [
         (18, 5, 'G.Step', 'points: publish at G.Stepped: G.Stepped lists pair.cursor in keep, but pair.cursor is spent by publish at G.Stepped at t.rop:18'),
     ]),
    ('point names an unknown projection',
     'spent(window.cursor, window.value)',
     'spent(window.cursor, window.sent)', [
         (25, 1, 'W.Done', 'spent(window.sent): W has no operand window.sent'),
         (29, 5, 'W.Drop', 'points: publish at W.Done: W.Done does not list window.value, but window.value is spent by publish at W.Done at t.rop:29'),
     ]),
    ('row takes a group',
     'extern helper next_index(c:B)',
     'extern helper next_index(c:B, w:SendWindow)', [
         (7, 31, 'next_index', 'parameter w:SendWindow: a row takes one home of a group, a projection g.p, as a Src'),
         (17, 15, 'G.Step', 'next_index takes 2 arguments, not 1'),
     ]),
    ('spent twice by group',
     'retire [spent(window.value), spent(window.cursor)]',
     'retire [spent(window), spent(window.cursor)]', [
         (29, 5, 'W.Drop', 'points: publish at W.Done: W.Done lists window.receiver in keep, but window.receiver is spent by publish at W.Done at t.rop:29'),
         (29, 39, 'W.Drop', 'publish: spent(window.cursor) is retired twice: a repeated operand makes one claim on its temporary'),
     ]),
    ('cursor release undeclared',
     'arm W.Drop shared effects(writes, releases)',
     'arm W.Drop shared effects(writes)', [
         (29, 5, 'W.Drop', 'effects: publish at W.Done (spent(window.value), spent(window.cursor)) may releases, which effects(writes) does not declare'),
     ]),
]


class GroupTests(unittest.TestCase):
    """Home groups: IterPair and SendWindow operands stand for their
    projections g.p, temporaries the instruction reads and writes"""
    def test_groups_are_clean(self):
        self.assertEqual(transfer_diagnostics(GROUPS), [])

    def test_group_fixtures(self):
        for name, old, new, expected in GROUPS_CASES:
            with self.subTest(name):
                self.assertIn(old, GROUPS)
                found = transfer_diagnostics(GROUPS.replace(old, new, 1))
                self.assertTrue(expected)
                self.assertEqual(sorted(found), sorted(expected))



COMPLETE = """\
family S(completed:Dst, window:SendWindow, done:NearHead) exits(branch, enter, raise) ops(NOP);
point S.Yielded(caller, next_head, clear) keep(window.receiver, window.cursor) spent(window.value) defs(window.value);
point S.Completed(current, next_head, clear) keep() spent(window) defs(completed);
point S.Failed(current, site, pending) keep(window.receiver, window.cursor) spent(window.value) defs();
extern helper enter_generator(t:Thread, f:Frame, g:B, v:O)
  cases(entered(c:NewFrame), err(e:Err))
  effects(reads, writes, raises)
  ensures(entered.generator_frame(c))
  c ENTER_GENERATOR as pointer_or_raised
  evidence(review: "r", contract: "c");
arm S.Generator split effects(reads, writes, raises, switches_frame) feedback() exits(enter, raise)
{
  entry():
    let g:B = use(window.receiver);
    let v:O = acquire(window.value);
    call enter_generator(thread, frame, g, v) { entered -> Enter(); err -> Failed(); }
  Enter(c:NewFrame):
    exit enter(c) resume S.Yielded complete S.Complete;
  Failed(e:Err):
    exit raise(S.Failed, e);
}
arm S.Complete completion(S.Yielded) split effects(writes, releases on (completed)) feedback() exits(branch)
{
  entry(v:O):
    publish [completed = v] retire [spent(window.receiver), spent(window.cursor), old(completed)] at S.Completed;
    exit branch(done);
}
"""


COMPLETE_SECOND = """\
arm S.Twice completion(S.Yielded) split effects(writes, releases on (completed)) feedback() exits(branch)
{
  entry(v:O):
    publish [completed = v] retire [spent(window.receiver), spent(window.cursor), old(completed)] at S.Completed;
    exit branch(done);
}
"""


COMPLETE_CASES = [
    ('a completion that leaves the frame',
     'feedback() exits(branch)\n{\n  entry(v:O)',
     'feedback() exits(branch, enter)\n{\n  entry(v:O)', [
         (22, 44, 'S.Complete', 'a completion runs after its child returned; it cannot leave the frame or restart the word (enter)'),
         (22, 44, 'S.Complete', 'declared exit enter is not reached'),
     ]),
    ('an entry with two parameters',
     'entry(v:O):',
     'entry(v:O, w:O):', [
         (24, 3, 'S.Complete', "a completion's first block is entry(v:O), the child's final value"),
     ]),
    ('an entry with no parameter',
     'entry(v:O):',
     'entry():', [
         (24, 3, 'S.Complete', "a completion's first block is entry(v:O), the child's final value"),
         (25, 26, 'S.Complete', 'unknown name v'),
     ]),
    ('complete names a word arm',
     'complete S.Complete;',
     'complete S.Generator;', [
         (18, 5, 'S.Generator', 'complete S.Generator: not a plain completion arm of S'),
     ]),
    ('complete at another point',
     'arm S.Complete completion(S.Yielded)',
     'point S.Again(caller, next_head, clear) keep(window.receiver, window.cursor) spent(window.value) defs(window.value);\narm S.Complete completion(S.Again)', [
         (18, 5, 'S.Generator', 'complete S.Complete: it completes S.Again, but the enter resumes at S.Yielded'),
     ]),
    ('a completion point that is not a resume point',
     'arm S.Complete completion(S.Yielded)',
     'arm S.Complete completion(S.Completed)', [
         (18, 5, 'S.Generator', 'complete S.Complete: it completes S.Completed, but the enter resumes at S.Yielded'),
         (22, 1, 'S.Complete', 'completion(S.Completed): the point is (current, next_head) with 1 definitions; a resume point is (caller, next_head) with at most one'),
     ]),
    ('two plain completions',
     '    exit branch(done);\n}\n',
     '    exit branch(done);\n}\n' + COMPLETE_SECOND, [
         (28, 1, 'S.Twice', 'S has two plain completions (S.Complete, S.Twice)'),
     ]),
    ('a generator entered without complete',
     ' complete S.Complete;',
     ';', [
         (18, 5, 'S.Generator', "control: exit enter(c): the frame is a generator's, which returns through a completion: name it (resume P complete A)"),
     ]),
    ("complete on a frame not proved a generator's",
     '  ensures(entered.generator_frame(c))\n',
     '', [
         (17, 5, 'S.Generator', 'control: exit enter(c) complete S.Complete: no fact proves generator_frame(c)'),
     ]),
    ('an observed completion without a plain one',
     'arm S.Complete completion(S.Yielded) split',
     'arm S.Complete completion(S.Yielded) split observes', [
         (18, 5, 'S.Generator', 'complete S.Complete: not a plain completion arm of S'),
         (22, 1, 'S.Complete', "an observed completion needs its family's events: S declares no events(...)"),
         (22, 1, 'S.Complete', 'an observed completion needs a plain one: S has none'),
     ]),
]


class CompletionTests(unittest.TestCase):
    """Completion arms: a generator child's return runs the consumer
    family's completion, from the resume point of the enter that names it"""
    def test_completion_is_clean(self):
        self.assertEqual(transfer_diagnostics(COMPLETE), [])

    def test_completion_fixtures(self):
        for name, old, new, expected in COMPLETE_CASES:
            with self.subTest(name):
                self.assertIn(old, COMPLETE)
                found = transfer_diagnostics(COMPLETE.replace(old, new, 1))
                self.assertTrue(expected)
                self.assertEqual(sorted(found), sorted(expected))

    def test_completions_are_not_word_arms(self):
        program = rop_check.check([rop_syntax.parse(COMPLETE, "t.rop")])
        self.assertEqual(set(program.completions), {"S.Complete"})
        self.assertNotIn("S.Complete", program.arms)



YIELD = """\
family Y(value:Src, resume:Dst) exits(yield) ops(NOP);
point Y.Resumed(current, next_head, clear) keep() spent(value) defs(resume);
extern helper pin_departing() -> Unit
  effects()
  c PIN as unit
  evidence(review: "r", contract: "c");
arm Y.Value split effects(writes, switches_frame) feedback() exits(yield)
{
  entry():
    let v:O = acquire(value);
    let _:Unit = pin_departing();
    exit yield(v) resume Y.Resumed;
}
"""


YIELD_CASES = [
    ('a yield without resume',
     ' resume Y.Resumed;',
     ';', [
         (12, 5, 'Y.Value', "an arm's exit enter or yield names its resume point (exit enter(c) resume P, exit yield(v) resume P), and no other exit does"),
     ]),
    ('a resume point of the caller',
     'point Y.Resumed(current,',
     'point Y.Resumed(caller,', [
         (12, 5, 'Y.Value', 'yield resume Y.Resumed: the point is (caller, next_head) with 1 definitions; a generator resumes at (current, next_head) with one, the sent value'),
     ]),
    ('a resume point whose definition is published before',
     '    let v:O = acquire(value);\n',
     '    let w:O = dup(use(value));\n    publish [resume = w] retire [old(resume)] at Y.Resumed;\n    let v:O = acquire(value);\n', [
         (11, 5, 'Y.Value', 'points: publish at Y.Resumed: Y.Resumed lists value in spent, but value is intact'),
         (11, 34, 'Y.Value', "effects: publish at Y.Resumed: old(resume) releases the binding resume displaces when it names a local's home, which may run Python: declare releases on (resume)"),
         (14, 5, 'Y.Value', 'points: exit yield resume Y.Resumed: resume is defined by publish at Y.Resumed at t.rop:11 before the frame resumes there; the resumption defines it'),
     ]),
    ('a yield of a borrowed value',
     'let v:O = acquire(value);',
     'let v:B = use(value);', [
         (12, 16, 'Y.Value', 'v is B, expected O'),
     ]),
    ('a yield on a path without pin_departing',
     '    let _:Unit = pin_departing();\n',
     '', [
         (11, 5, 'Y.Value', 'control: exit yield releases the departing frame, whose code is not pinned on this path: call pin_departing() first'),
     ]),
    ('a yield that names a completion',
     ' resume Y.Resumed;',
     ' resume Y.Resumed complete Y.Value;', [
         (12, 5, 'Y.Value', 'only an enter names a completion'),
         (12, 5, 'Y.Value', 'complete Y.Value: not a plain completion arm of Y'),
     ]),
]


class YieldTests(unittest.TestCase):
    """exit yield(v) resume P: the generator gives v away and resumes at P,
    the sent value P's one definition"""
    def test_yield_is_clean(self):
        self.assertEqual(transfer_diagnostics(YIELD), [])

    def test_yield_fixtures(self):
        for name, old, new, expected in YIELD_CASES:
            with self.subTest(name):
                self.assertIn(old, YIELD)
                found = transfer_diagnostics(YIELD.replace(old, new, 1))
                self.assertTrue(expected)
                self.assertEqual(sorted(found), sorted(expected))


def rop_files() -> list[Path]:
    return sorted(ROP_DIR.glob("*.rop"))


def at(filename: str, text: str, column: int | None = None, nth: int = 0,
       last: bool = False, index: int | None = None) -> str:
    """filename:line, and :column if given, of the nth line (the first by
    default; the last, if last; the index'th of all, if index) of the
    checked-in definition file that contains text"""
    lines = (ROP_DIR / filename).read_text().splitlines()
    found = [i for i, source in enumerate(lines, 1) if text in source]
    line = found[index] if index is not None else found[-1] if last else found[nth]
    return f"{filename}:{line}" if column is None else f"{filename}:{line}:{column}"


class PilotTests(unittest.TestCase):
    """The checked-in definitions, rows and pending macros taken at their
    declared signatures."""

    def test_pilot_is_clean(self):
        program = rop_verify.load(rop_files())
        self.assertEqual([str(d) for d in program.diagnostics], [])
        self.assertEqual(rop_verify.missing_generic(program), ())

    def test_pilot_mutations(self):
        # Changes to the pilot that the checker refuses.  Diagnostics name
        # the lines of the checked-in files, found by their text, so that
        # edits elsewhere in the pilot leave the expectations intact.
        box = at("add.rop", "call box_i64(full)", 5)
        # Add.Extended and Add.Generic end alike; the mutations edit the
        # last block, Add.Generic's
        raise_ = at("add.rop", "exit raise(Add.OpFailed, saved);", 5, last=True)
        retire = at("add.rop", "let saved:Err = retire_failed_call(", last=True)
        raise_ext = at("add.rop", "exit raise(Add.OpFailed, saved);", 5)
        cell_map = at("cells.rop", "exit raise(CellMapRead.Failed, saved);", 5)
        dup = at("getattr.rop", "let result:O = dup(unwrap(item));")
        guard = at("getattr.rop", "guard present(item) else miss;")
        # The GetSet arm raises first; Generic's is the second
        raise_getset = at("getattr.rop", "exit raise(GetAttr.Failed, saved);", 5)
        raise_attr = at("getattr.rop", "exit raise(GetAttr.Failed, saved);", 5, nth=1)
        raise_method = at("getattr.rop", "exit raise(GetMethod.Failed, saved);", 5)
        # Every arm of items.rop that raises: its family's sources, each of
        # which the arm's raise exit lists in spent
        item_spent = {"GetItem": ("key", "obj"), "SetItem": ("key", "obj", "value"),
                      "DelItem": ("key", "obj"), "SliceRead": ("obj", "start", "stop"),
                      "SliceWrite": ("obj", "start", "stop", "value")}
        item_diagnostics = []
        arm = None
        for number, source in enumerate(
                (ROP_DIR / "items.rop").read_text().splitlines(), 1):
            if source.startswith("arm "):
                arm = source.split()[1]
            elif source.lstrip().startswith("exit raise(") and arm.split(".")[0] in item_spent:
                family = arm.split(".")[0]
                item_diagnostics.extend(
                    f"items.rop:{number}:5: {arm}: points: raise at {family}.Failed: "
                    f"{family}.Failed lists {name} in spent, but {name} is intact"
                    for name in item_spent[family])
        mapped = at("namespaces.rop", "exit raise(NamespaceReadFrom.Failed, saved);", 5)
        # The arms of tests.rop that raise, in order of line: the family, the
        # arm and the operands that its point lists in spent
        tests_raises = [
            f"{at('tests.rop', f'exit raise({family}.Failed, saved);', 5, nth=nth)}: "
            f"{arm}: points: raise at {family}.Failed: {family}.Failed lists "
            f"{operand} in spent, but {operand} is intact"
            for family, nth, arm, operands in [
                ("Compare", 0, "Compare.Generic", ("x", "y")),
                ("Contains", 0, "Contains.Set", ("container", "item")),
                ("Contains", 1, "Contains.Dict", ("container", "item")),
                ("Contains", 2, "Contains.Generic", ("container", "item")),
                ("TestBranch", 0, "TestBranch.Set", ("x", "y")),
                ("TestBranch", 1, "TestBranch.Dict", ("x", "y")),
                ("TestBranch", 2, "TestBranch.Sequence", ("x", "y")),
                ("TestBranch", 3, "TestBranch.Generic", ("x", "y")),
                ("TestBranch", 4, "TestBranch.Observed", ("x", "y"))]
            for operand in operands]
        truth_slot = at("truth.rop", "exit raise(Truth.Failed, saved);", 5)
        truth_generic = at("truth.rop", "exit raise(Truth.Failed, saved);", 5, last=True)
        branch_slot = at("truth.rop", "exit raise(BranchTruth.Failed, saved);", 5)
        # The last is the observed wrapper's, which has no arm of its own to
        # adapt: the generic arm's is the one before it
        branch_generic = at("truth.rop", "exit raise(BranchTruth.Failed, saved);", 5, index=-2)
        branch_observed = at("truth.rop", "exit raise(BranchTruth.Failed, saved);", 5, last=True)
        mutations = [
            ("add.rop", "  effects(writes, allocates, raises, releases on (dst))\n",
             "  effects(writes, raises, releases on (dst))\n",
             [f"{box}: Add.Int: effects: call box_i64 may allocates, which "
              "effects(writes, raises, releases on (dst)) does not declare"]),
            ("add.rop", "    exit raise(Add.OpFailed, saved);",
             "    exit raise(Add.BoxFailed, saved);",
             [f"{raise_}: Add.Generic: points: raise at Add.BoxFailed: Add.BoxFailed "
              f"lists x in keep, but x is spent by call retire_failed_call at {retire}",
              f"{raise_}: Add.Generic: points: raise at Add.BoxFailed: Add.BoxFailed "
              f"lists y in keep, but y is spent by call retire_failed_call at {retire}"]),
            # The swap moves dup's line up to the guard's, and the guard's down
            ("getattr.rop", "    guard present(item) else miss;\n"
             "    let result:O = dup(unwrap(item));\n",
             "    let result:O = dup(unwrap(item));\n"
             "    guard present(item) else miss;\n",
             [f"{guard}:24: GetAttr.Instance: facts: unwrap(item) requires "
              "present(item), which no dominating fact proves",
              f"{dup}:5: GetAttr.Instance: restart: guard present(item) else miss "
              f"follows dup(unwrap(item)) at {guard}: the word cannot restart"]),
            ("core.rop", "  spends(s)\n", "",
             # Without its spends(s), the macro leaves the sources the point
             # declares spent
             [f"{raise_ext}: Add.Extended: points: raise at Add.OpFailed: Add.OpFailed "
              "lists x in spent, but x is intact",
              f"{raise_ext}: Add.Extended: points: raise at Add.OpFailed: Add.OpFailed "
              "lists y in spent, but y is intact",
              f"{raise_}: Add.Generic: points: raise at Add.OpFailed: Add.OpFailed "
              "lists x in spent, but x is intact",
              f"{raise_}: Add.Generic: points: raise at Add.OpFailed: Add.OpFailed "
              "lists y in spent, but y is intact",
              f"{cell_map}: CellMapRead.Lookup: points: raise at CellMapRead.Failed: "
              "CellMapRead.Failed lists mapping in spent, but mapping is intact",
              f"{raise_getset}: GetAttr.GetSet: points: raise at GetAttr.Failed: "
              "GetAttr.Failed lists obj in spent, but obj is intact",
              f"{raise_attr}: GetAttr.Generic: points: raise at GetAttr.Failed: "
              "GetAttr.Failed lists obj in spent, but obj is intact",
              f"{raise_method}: GetMethod.Generic: points: raise at GetMethod.Failed: "
              "GetMethod.Failed lists obj in spent, but obj is intact",
              *item_diagnostics,
              f"{mapped}: NamespaceReadFrom.Mapping: points: raise at "
              "NamespaceReadFrom.Failed: NamespaceReadFrom.Failed lists mapping in "
              "spent, but mapping is intact",
              *tests_raises,
              f"{truth_slot}: Truth.Slot: points: raise at Truth.Failed: "
              "Truth.Failed lists value in spent, but value is intact",
              f"{truth_generic}: Truth.Generic: points: raise at Truth.Failed: "
              "Truth.Failed lists value in spent, but value is intact",
              f"{branch_slot}: BranchTruth.Slot: points: raise at BranchTruth.Failed: "
              "BranchTruth.Failed lists value in spent, but value is intact",
              f"{branch_generic}: BranchTruth.Generic: points: raise at BranchTruth.Failed: "
              "BranchTruth.Failed lists value in spent, but value is intact",
              f"{branch_observed}: BranchTruth.Observed: points: raise at BranchTruth.Failed: "
              "BranchTruth.Failed lists value in spent, but value is intact"]),
        ]
        for filename, old, new, expected in mutations:
            with self.subTest(old):
                files = []
                for path in rop_files():
                    text = path.read_text()
                    if path.name == filename:
                        self.assertIn(old, text)
                        # The last occurrence: Add.Generic's block
                        head, _, tail = text.rpartition(old)
                        text = head + new + tail if "exit raise" in old else text.replace(old, new, 1)
                    files.append(rop_syntax.parse(text, path.name))
                program = rop_check.check(files)
                found = [str(d) for d in program.diagnostics + rop_verify.verify(program)]
                self.assertEqual(found, expected)


if __name__ == "__main__":
    unittest.main()
