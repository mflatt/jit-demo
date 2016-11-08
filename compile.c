#include <stdlib.h>
#include "compile.h"
#include "lookup.h"
#include "fail.h"

tagged* compile(tagged* expr, env* e, hash_table* d)
{
  switch (TAGGED_TYPE(expr)) {
  case num_type:
  case func_type:
    return expr;
  case sym_type:
    {
      int pos;

      pos = env_lookup_pos((symbol *)expr, e);
      if (pos == -1)
        return lookup(d, (symbol *)expr);
      else
        return make_debruijn(pos);
    }
  case plus_type:
  case minus_type:
  case times_type:
  case app_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      return make_bin_op(bn->t.type,
                         compile(bn->left, e, d),
                         compile(bn->right, e, d));
    }
  case lambda_type:
    {
      lambda_expr *lam = (lambda_expr *)expr;

      return make_lambda(lam->arg_name,
                         compile(lam->body,
                                 make_env(lam->arg_name,
                                          make_num(0),
                                          e),
                                 d));
    }
  case if0_type:
    {
      if0_expr* if0 = (if0_expr*)expr;

      return make_if0(compile(if0->tst, e, d),
                      compile(if0->thn, e, d),
                      compile(if0->els, e, d));
    }
  default:
    fail("unrecognized expression in compile");
  }

  return expr;
}

void compile_function(symbol* name, hash_table *d) {
  /* Compiles a (potentially recursive) function that's in d */
  tagged* fn = lookup(d, name);
  tagged* new_lam;

  if (TAGGED_TYPE(fn) != func_type)
    fail("not defined as a function");

  new_lam = make_lambda(((func_val*)fn)->lam->arg_name,
                        compile(((func_val*)fn)->lam->body,
                                make_env(((func_val*)fn)->lam->arg_name,
                                         make_num(0),
                                         ((func_val *)fn)->e),
                                d));
    
  ((func_val*)fn)->lam = (lambda_expr*)new_lam;
}
