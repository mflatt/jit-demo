#include "eval.h"
#include "continue.h"
#include "fail.h"
#include "lookup.h"
#include "jit.h"
#include "gc.h"

#define PERFORM_TODOS(_val, _todos) val = _val; todos = _todos; goto todo
#define EVAL(_expr, _e, _todos) expr = _expr; e = _e; todos = _todos; goto eval

tagged* expr;
env* e;
cont* todos;
tagged *val;

void eval_star(hash_table* d)
{

 eval:
  switch(TAGGED_TYPE(expr)) {
  case num_type:
    PERFORM_TODOS(expr, todos);
  case func_type: /* literal function can appear as a result of compilation */
    PERFORM_TODOS(expr, todos);
  case sym_type:
    PERFORM_TODOS(env_lookup((symbol*)expr, e, d), todos);
  case debruijn_type:
    {
#     define db ((debruijn_expr*)expr)
      PERFORM_TODOS(env_lookup_by_pos(db->pos, e), todos);
#     undef db
    }
  case plus_type:
  case minus_type:
  case times_type:
    {
#     define bn ((bin_op_expr*)expr)
       
      todos = make_right_of_bin(bn->t.type,
                                bn->right,
                                e,
                                todos);
      EVAL(bn->left, e, todos);

#     undef bn
    }
  case app_type:
    {
#     define bn ((bin_op_expr*)expr)
      
      todos = make_right_of_app(bn->right,
                                e,
                                todos);
      EVAL(bn->left, e, todos);

#     undef bn
    }
  case lambda_type:
    {
#     define lam ((lambda_expr *)expr)

      val = make_func(expr,
                      e);
      PERFORM_TODOS(val, todos);

#     undef lam
    }
  case if0_type:
    {
#     define if0 ((if0_expr *)expr)

      todos = make_finish_if0(if0->thn,
                              if0->els,
                              e,
                              todos);

      EVAL(if0->tst, e, todos);

#     undef if0
    }
  }

  fail("unrecognized expression");

 todo:
  switch (TAGGED_TYPE(todos)) {
  case done_type:
    return;
  case right_of_bin_type:
    {
#     define gl ((right_of_bin *)todos)
      cont* new_todos;

      new_todos = make_finish_bin(gl->op, val, gl->rest);

      EVAL(gl->right, gl->env, new_todos);

#     undef gl
    }
  case finish_bin_type:
    {
#     define gr ((finish_bin *)todos)

      tagged *l = gr->left_val;
      tagged *r = val;
      int ln, rn, vn;
       
      if (TAGGED_TYPE(l) != num_type)
        fail("not a number");
      if (TAGGED_TYPE(r) != num_type)
        fail("not a number");
       
      ln = NUM_VAL(l);
      rn = NUM_VAL(r);

      switch(gr->op) {
      case plus_type:
        vn = ln + rn;
        break;
      case minus_type:
        vn = ln - rn;
        break;
      case times_type:
        vn = ln * rn;
        break;
      }

      val = make_num(vn);
      PERFORM_TODOS(val, gr->rest);

#      undef gr
    }
  case right_of_app_type:
    {
#     define gl ((right_of_app *)todos)
      cont* new_todos;

      new_todos = make_finish_app(val, gl->rest);
      EVAL(gl->right, gl->env, new_todos);

#     undef gl
    }
  case finish_app_type:
    {
#     define gr ((finish_app *)todos)
#     define fn (gr->left_val)

      if (TAGGED_TYPE(fn)) {
#       define fv ((func_val *)fn)

        e = make_env(fv->lam->arg_name, val, fv->e);

        if (jit(fv->lam)) {
#         if USE_JIT
          jitted_proc code = fv->lam->code;
          todos = gr->rest;
          /* `code` may update `todos` */
          PERFORM_TODOS(code(&todos), todos);
#         endif
        } else {
          EVAL(fv->lam->body, e, gr->rest);
        }
        
#       undef fv
      } else
        fail("not a function");

#     undef fn
#     undef gr
    }  
  case finish_if0_type:
    {
#     define gi ((finish_if0 *)todos)

      if (TAGGED_TYPE(val) == num_type) {
        if (NUM_VAL(val) == 0) {
          EVAL(gi->thn, gi->env, gi->rest);
        } else {
          EVAL(gi->els, gi->env, gi->rest);
        }
      } else
        fail("not a number");

#     undef gi
    }
# if USE_JIT
  case right_jitted_type:
  case finish_jitted_type:
    {
#     define gj ((jitted *)todos)
      /* `gj->code` updates `todos` */
      PERFORM_TODOS(gj->code(), todos);
#     undef gj
    }
# endif
  }
  
  fail("unrecognized continuation");
}

tagged* eval(tagged* _expr, env* _e, hash_table* d)
{
  enable_gc();

  expr = _expr;
  e = _e;
  todos = make_done();
  eval_star(d);

  disable_gc();

  return val;
}
