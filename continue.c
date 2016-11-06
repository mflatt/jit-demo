#include <stdlib.h>
#include <string.h>
#include "continue.h"
#include "gc.h"

static void init_cont(cont *c, int tag) {
  c->type = tag;
}

cont *make_done() {
  cont *c = (cont*)gc_malloc0(sizeof(cont));
  init_cont(c, done_type);
  return c;
}

cont *make_right_of_bin(int op, tagged *right, env *env, cont *rest) {
  right_of_bin *c =gc_malloc3(sizeof(right_of_bin),
                              &right,
                              &env,
                              &rest);
  init_cont(&c->c, right_of_bin_type);
  c->op = op;
  c->right = right;
  c->env = env;
  c->rest = rest;
  return (cont*)c;
}

cont *make_finish_bin(int op, tagged *left_val, cont *rest) {
  finish_bin *c = gc_malloc2(sizeof(finish_bin),
                             &left_val,
                             &rest);
  init_cont(&c->c, finish_bin_type);
  c->op = op;
  c->left_val = left_val;
  c->rest = rest;
  return (cont*)c;
}

cont *make_right_of_app(tagged *right, env *env, cont *rest) {
  right_of_app *c = gc_malloc3(sizeof(right_of_app),
                               &right,
                               &env,
                               &rest);
  init_cont(&c->c, right_of_app_type);
  c->right = right;
  c->env = env;
  c->rest = rest;
  return (cont*)c;
}

cont *make_finish_app(tagged *left_val, cont *rest) {
  finish_app *c = gc_malloc2(sizeof(finish_app),
                             &left_val,
                             &rest);
  init_cont(&c->c, finish_app_type);
  c->left_val = left_val;
  c->rest = rest;
  return (cont*)c;
}

cont *make_finish_if0(tagged *thn, tagged *els, env *env, cont *rest) {
  finish_if0 *c = gc_malloc4(sizeof(finish_if0),
                             &thn,
                             &els,
                             &env,
                             &rest);
  init_cont(&c->c, finish_if0_type);
  c->thn = thn;
  c->els = els;
  c->env = env;
  c->rest = rest;

  return (cont*)c;
}

#if USE_JIT
static void init_jitted(jitted *j, jitted_proc code, jitted_proc tail_code, cont *rest) {
  j->code = code;
  j->tail_code = tail_code;
  j->rest = rest;
}

cont *make_right_jitted(jitted_proc code, jitted_proc tail_code, cont *rest, env *env) {
  right_jitted *c = gc_malloc2(sizeof(right_jitted),
                               &env,
                               &rest);
  init_cont(&c->j.c, right_jitted_type);
  init_jitted(&c->j, code, tail_code, rest);
  c->env = env;
  
  return (cont*)c;
}

cont *make_finish_jitted(jitted_proc code, jitted_proc tail_code, cont *rest, tagged* val) {
  finish_jitted *c = gc_malloc2(sizeof(finish_jitted),
                                &rest,
                                &val);
  init_cont(&c->j.c, finish_jitted_type);
  init_jitted(&c->j, code, tail_code, rest);
  c->val = val;
  
  return (cont*)c;
}
#endif
