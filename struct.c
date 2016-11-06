#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "struct.h"
#include "gc.h"

static void init_tagged(tagged *t, int tag) {
  t->type = tag;
}

#if !FIXNUM_ENCODING
tagged* make_num(int n) {
  num_val *nv = (num_val *)gc_malloc0(sizeof(num_val));
  init_tagged(&nv->t, num_type);
  nv->n = n;
  return (tagged*)nv;
}
#endif

tagged* make_func(symbol *arg_name, tagged *body, env *e) {
  func_val *fv = (func_val *)gc_malloc3(sizeof(func_val),
                                        &arg_name,
                                        &body,
                                        &e);
  init_tagged(&fv->t, func_type);
  fv->arg_name = arg_name;
  fv->body = body;
  fv->e = e;
  return (tagged*)fv;
}

symbol* make_symbol(char *s) {
  symbol *sym = (symbol *)gc_malloc0(sizeof(symbol));
  init_tagged(&sym->t, sym_type);
  sym->s = strdup(s);
  return sym;
}

tagged* make_debruijn(int pos) {
  debruijn_expr *db = (debruijn_expr *)gc_malloc0(sizeof(debruijn_expr));
  init_tagged(&db->t, debruijn_type);
  db->pos = pos;
  return (tagged*)db;
}

tagged* make_bin_op(int type, tagged *left, tagged *right) {
  bin_op_expr *bin = (bin_op_expr *)gc_malloc2(sizeof(bin_op_expr),
                                               &left,
                                               &right);
  init_tagged(&bin->t, type);
  bin->left = left;
  bin->right = right;
  return (tagged*)bin;  
}

tagged* make_plus(tagged *left, tagged *right) {
  return make_bin_op(plus_type, left, right);
}

tagged* make_minus(tagged *left, tagged *right) {
  return make_bin_op(minus_type, left, right);
}

tagged* make_times(tagged *left, tagged *right) {
  return make_bin_op(times_type, left, right);
}

tagged* make_app(tagged *left, tagged *right) {
  return make_bin_op(app_type, left, right);
}

tagged* make_lambda(symbol *arg_name, tagged *body) {
  lambda_expr *lam = (lambda_expr *)gc_malloc2(sizeof(lambda_expr),
                                               &arg_name,
                                               &body);
  init_tagged(&lam->t, lambda_type);
  lam->arg_name = arg_name;
  lam->body = body;
  return (tagged*)lam;
}

tagged* make_if0(tagged *tst, tagged *thn, tagged *els) {
  if0_expr *if0 = (if0_expr *)gc_malloc3(sizeof(if0_expr),
                                         &tst,
                                         &thn,
                                         &els);
  init_tagged(&if0->t, if0_type);
  if0->tst = tst;
  if0->thn = thn;
  if0->els = els;
  return (tagged*)if0;
}

env* make_env(symbol *id, tagged *val, env *rest)
{
  env *e = (env *)gc_malloc3(sizeof(env),
                             &id,
                             &val,
                             &rest);
  e->type = env_type;
  e->id = id;
  e->val = val;
  e->rest = rest;
  return e;  
}
