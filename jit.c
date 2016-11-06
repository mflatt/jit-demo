#include <stddef.h>
#include "struct.h"
#include "jit.h"

#if !USE_JIT

int jit(lambda_expr *lam) {
  return FALSE;
}

#else

#include <lightning.h>
#include "eval.h"
#include "continue.h"
#include "fail.h"

#define jit_movi_p(dest, addr) jit_movi(dest, (jit_word_t)(addr))
#define jit_pushargi_p(addr) jit_pushargi((jit_word_t)(addr))

static int inited = 0;

static jit_state_t *_jit;

static void jit_expr(tagged* expr, int stack_pos);
static void jit_cont_expr(tagged* expr, int cont_type, int stack_pos);

static void jit_cont_make(tagged* expr, int cont_type, int jit_cont_type);

static void do_jit_next();
static void continue_or_return();

static void jit_check_type(int need_type, int reg, char *complain);
static void jit_set_stack(int stack_pos, int reg);
static void jit_get_stack(int reg, int stack_pos);
static void jit_prep_possible_gc(int stack_pos, int tmp_reg);

static int no_calls(tagged* expr);
static int has_sym(tagged* expr);
static int definitely_number(tagged* expr);
static int definitely_function(tagged* expr);

#define STACK_SIZE 64
static tagged* stack[STACK_SIZE];
static int stack_gc_depth;

#define FRAME_SIZE 64

void push_jit_stack(void (*paint_gray)(void *))
{
  int i;
  for (i = 0; i < stack_gc_depth; i++) {
    paint_gray(&stack[i]);
  }
}

#if !FIXNUM_ENCODING
# error "JIT requires fixnum encoding"
#endif

int jit(lambda_expr *lam)
{
  jit_state_t *old_jit;
  jit_node_t *after_prolog;

  if (lam->code)
    return TRUE; /* already JIT-compiled */

  if (has_sym(lam->body))
    return FALSE;

  if (!inited) {
    inited = 1;
    init_jit(NULL);
  }

  old_jit = _jit;
  _jit = jit_new_state();

  jit_prolog();
  jit_frame(FRAME_SIZE);
  after_prolog = jit_indirect();

  jit_expr(lam->body, 0);

  continue_or_return();
  
  lam->code = jit_emit();
  lam->tail_code = jit_address(after_prolog);

  jit_clear_state();
  _jit = old_jit;
  
  return TRUE;
}

jitted_proc jit_cont(tagged* expr, int cont_type, int jit_cont_type, jitted_proc *_tail_code)
{
  jitted_proc code;
  jit_state_t *old_jit;
  jit_node_t *after_prolog;
   
  old_jit = _jit;
  _jit = jit_new_state();

  jit_prolog();
  jit_frame(FRAME_SIZE);
  after_prolog = jit_indirect();

  jit_ldi(JIT_R2, &todos);
  
  jit_ldxi(JIT_R1, JIT_R2, offsetof(jitted, rest));
  jit_sti(&todos, JIT_R1);

  switch (jit_cont_type) {
  case right_jitted_type:
    jit_ldxi(JIT_R1, JIT_R2, offsetof(right_jitted, env));
    jit_sti(&e, JIT_R1);
    break;
  case finish_jitted_type:
    jit_ldxi(JIT_R1, JIT_R2, offsetof(finish_jitted, val));
    break;
  }

  jit_ldi(JIT_R0, &val);
 
  jit_cont_expr(expr, cont_type, 0);

  continue_or_return();
  
  code = jit_emit();
  *_tail_code = jit_address(after_prolog);

  jit_clear_state();
  _jit = old_jit;
  
  return code;
}

void continue_or_return()
{
  /* Stay in the JITted world if the continuation is one of ours */
  jit_node_t *test, *this_one;
  
  jit_ldi(JIT_R2, &todos);
  jit_ldxi_i(JIT_R1, JIT_R2, offsetof(cont, type));

  test = jit_bnei(JIT_R1, right_jitted_type);
  this_one = jit_label();
  do_jit_next();

  jit_patch(test);
  test = jit_beqi(JIT_R1, finish_jitted_type);
  jit_patch_at(test, this_one);

  jit_prep_possible_gc(0, JIT_R2);
  jit_retr(JIT_R0);
} 

static void jit_expr(tagged* expr, int stack_pos)
{
  switch (TAGGED_TYPE(expr)) {
  case num_type:
  case func_type:
    jit_movi_p(JIT_R0, expr);
    break;
  case sym_type:
    fail("cannot JIT symbol lookup");
    break;
  case debruijn_type:
    {
      int pos = ((debruijn_expr*)expr)->pos;
      jit_ldi(JIT_R2, &e);
      while (pos--) {
        jit_ldxi(JIT_R2, JIT_R2, offsetof(env, rest));
      }
      jit_ldxi(JIT_R0, JIT_R2, offsetof(env, val));
    }
    break;
  case plus_type:
  case minus_type:
  case times_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      if (no_calls(bn->left) && no_calls(bn->right)) {
        jit_expr(bn->left, stack_pos);

        jit_set_stack(stack_pos, JIT_R0);
  
        jit_expr(bn->right, stack_pos + 1);

        jit_get_stack(JIT_R1, stack_pos);

        jit_cont_expr(expr, finish_bin_type, stack_pos);
      } else {
        jit_cont_make(expr, right_of_bin_type, right_jitted_type);
        jit_expr(bn->left, stack_pos);
        do_jit_next();
      }
    }
    break;
  case if0_type:
    {
      if0_expr* if0 = (if0_expr*)expr;

      if (no_calls(if0->tst)) {
        jit_expr(if0->tst, stack_pos);

        jit_cont_expr(expr, finish_if0_type, stack_pos);
      } else {
        jit_cont_make(expr, finish_if0_type, right_jitted_type);
        jit_expr(if0->tst, stack_pos);
        do_jit_next();
      }
    }
    break;
  case lambda_type:
    {
      lambda_expr *lam = (lambda_expr *)expr;

      jit_prep_possible_gc(stack_pos, JIT_R2);
      
      jit_prepare();
      jit_pushargi_p(lam);
      jit_pushargr(JIT_R0);
      jit_finishi(make_func);
      jit_retval(JIT_R0);
    }
    break;
  case app_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      if (no_calls(bn->left) && no_calls(bn->right)) {
        jit_expr(bn->left, stack_pos);

        jit_set_stack(stack_pos, JIT_R0);

        jit_expr(bn->right, stack_pos + 1);

        jit_get_stack(JIT_R1, stack_pos);

        jit_cont_expr(expr, finish_app_type, stack_pos);
      } else  {
        jit_cont_make(expr, right_of_app_type, right_jitted_type);
        jit_expr(bn->left, stack_pos);
        do_jit_next();
      }
    }
    break;
  default:
    fail("unrecognized expression in JIT compile");
  }
}

static void jit_cont_expr(tagged* expr, int cont_type, int stack_pos)
{
  switch (cont_type) {
  case right_of_bin_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      if (no_calls(bn->right)) {
        jit_set_stack(stack_pos, JIT_R0);
        jit_expr(bn->right, stack_pos+1);
        jit_get_stack(JIT_R1, stack_pos);
        jit_cont_expr(expr, finish_bin_type, stack_pos);
      } else {
        jit_cont_make(expr, finish_bin_type, finish_jitted_type);
        jit_expr(bn->right, stack_pos);
        do_jit_next();
      }
    }
    break;
  case finish_bin_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      /* JIT_R1 has left arg, JIT_R0 has right arg */

      if (!definitely_number(bn->left))
        jit_check_type(num_type, JIT_R1, "not a number");
      if (!definitely_number(bn->right))
        jit_check_type(num_type, JIT_R0, "not a number");
      
      jit_rshi(JIT_R1, JIT_R1, 1);
      jit_rshi(JIT_R0, JIT_R0, 1);
      
      switch (TAGGED_TYPE(expr)) {
      case plus_type:
        jit_addr(JIT_R0, JIT_R1, JIT_R0);
        break;
      case minus_type:
        jit_subr(JIT_R0, JIT_R1, JIT_R0);
        break;
      case times_type:
        jit_mulr(JIT_R0, JIT_R1, JIT_R0);
        break;
      default:
        fail("unknown arithmetic");
      }

      jit_lshi(JIT_R0, JIT_R0, 1);
      jit_ori(JIT_R0, JIT_R0, 0x1);
    }
    break;
  case finish_if0_type:
    {
      if0_expr* if0 = (if0_expr*)expr;
      jit_node_t *branch, *done;

      if (!definitely_number(if0->tst))
        jit_check_type(num_type, JIT_R0, "not a number");

      jit_rshi(JIT_R0, JIT_R0, 1);
      branch = jit_bnei(JIT_R0, 0);
      
      jit_expr(if0->thn, stack_pos);
      done = jit_jmpi();
      
      jit_patch(branch);
      jit_expr(if0->els, stack_pos);
      
      jit_patch(done);
    }
    break;
  case right_of_app_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      if (no_calls(bn->right)) {
        jit_set_stack(stack_pos, JIT_R0);
        jit_expr(bn->right, stack_pos+1);
        jit_get_stack(JIT_R1, stack_pos);
        jit_cont_expr(expr, finish_app_type, stack_pos);
      } else {
        jit_cont_make(expr, finish_app_type, finish_jitted_type);
        jit_expr(bn->right, stack_pos);
        do_jit_next();
      }
    }
    break;
  case finish_app_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;
      jit_node_t *branch;

      /* JIT_R1 has function, JIT_R0 has argument */
      if (!definitely_function(bn->left))
        jit_check_type(func_type, JIT_R1, "not a function");

      /* Save function & arg on stack in case of GC */
      jit_set_stack(stack_pos, JIT_R1);
      jit_set_stack(stack_pos+1, JIT_R0);

      jit_prep_possible_gc(stack_pos+2, JIT_R2);
      
      jit_ldxi(JIT_R2, JIT_R1, offsetof(func_val, lam));
      jit_ldxi(JIT_R0, JIT_R2, offsetof(lambda_expr, code));
      branch = jit_bnei(JIT_R0, (jit_word_t)NULL);

      {
        /* Not yet JIT-compiled, so compile it now */
        jit_prepare();
        jit_pushargr(JIT_R2);
        jit_finishi(jit);
      }
      
      jit_patch(branch);

      /* Extend the environment */
      jit_get_stack(JIT_R1, stack_pos);
      jit_get_stack(JIT_R0, stack_pos+1);
      jit_ldxi(JIT_R1, JIT_R1, offsetof(func_val, lam));
      jit_ldxi(JIT_R1, JIT_R1, offsetof(lambda_expr, arg_name));
      jit_ldi(JIT_R2, &e);

      
      jit_prepare();
      jit_pushargr(JIT_R1); /* argument name */
      jit_pushargr(JIT_R0); /* argument value */
      jit_pushargr(JIT_R2); /* env */
      jit_finishi(make_env);
      jit_retval(JIT_R0);

      jit_sti(&e, JIT_R0);

      /* Jump to the called function's body */
      jit_get_stack(JIT_R1, stack_pos);
      jit_ldxi(JIT_R0, JIT_R1, offsetof(func_val, lam));
      jit_ldxi(JIT_R0, JIT_R0, offsetof(lambda_expr, tail_code));
      jit_jmpr(JIT_R0);
    }
    break;
  default:
    fail("unrecognized continuation type in JIT compile");
  }
}

static void jit_cont_make(tagged* expr, int cont_type, int jit_cont_type)
{
  jitted_proc cont_code, cont_tail_code;
  
  cont_code = jit_cont(expr, cont_type, jit_cont_type, &cont_tail_code);
  
  jit_prep_possible_gc(0, JIT_R2);

  jit_ldi(JIT_R2, &todos);

  jit_prepare();
  jit_movi_p(JIT_R1, cont_code);
  jit_pushargr(JIT_R1); /* code pointer */
  jit_movi_p(JIT_R1, cont_tail_code);
  jit_pushargr(JIT_R1);
  jit_pushargr(JIT_R2); /* continuation */
  switch (jit_cont_type) {
  case right_jitted_type:
    jit_ldi(JIT_R0, &e);
    jit_pushargr(JIT_R0); /* environment */
    jit_finishi(make_right_jitted);
    break;
  case finish_jitted_type:
    jit_pushargr(JIT_R0); /* value to save */
    jit_finishi(make_finish_jitted);
    break;
  }
  jit_retval(JIT_R0);

  jit_sti(&todos, JIT_R0);
}

static void jit_check_type(int need_type, int reg, char *complain)
{
  jit_node_t *ok, *not_ok;
  
  if (need_type == num_type)
    ok = jit_bmsi(reg, 0x1);
  else {
    not_ok = jit_bmsi(reg, 0x1);
    jit_ldxi_i(JIT_R2, reg, offsetof(tagged, type));
    ok = jit_beqi(JIT_R2, need_type);
    jit_patch(not_ok);
  }

  jit_prepare();
  jit_pushargi_p(complain);
  jit_finishi(fail);

  jit_patch(ok);
}

static void do_jit_next()
{
  /* We know that the current continuation has type finish_jitted[_with_val] */
  jit_sti(&val, JIT_R0);
  jit_ldi(JIT_R2, &todos);
  jit_ldxi(JIT_R2, JIT_R2, offsetof(jitted, tail_code));
  jit_jmpr(JIT_R2);
}

static void jit_set_stack(int stack_pos, int reg)
{
  if (stack_pos >= STACK_SIZE)
    fail("stack too small for JIT-generated code");

  jit_sti(&stack[stack_pos], reg);
}
  
static void jit_get_stack(int reg, int stack_pos)
{
  jit_ldi(reg, &stack[stack_pos]);
}

static void jit_prep_possible_gc(int stack_pos, int tmp_reg)
{
  jit_movi(tmp_reg, stack_pos);
  jit_sti_i(&stack_gc_depth, tmp_reg);
}

static int no_calls(tagged* expr)
{
  switch (TAGGED_TYPE(expr)) {
  case app_type:
    return FALSE;
  case plus_type:
  case minus_type:
  case times_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      return no_calls(bn->left) && no_calls(bn->right);
    }
  case if0_type:
    {
      if0_expr* if0 = (if0_expr*)expr;

      return no_calls(if0->tst) && no_calls(if0->thn) && no_calls(if0->els);
    }
  }

  return TRUE;
}

static int has_sym(tagged* expr)
{
  switch (TAGGED_TYPE(expr)) {
  case sym_type:
    return TRUE;
  case plus_type:
  case minus_type:
  case times_type:
  case app_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      return has_sym(bn->left) || has_sym(bn->right);
    }
  case if0_type:
    {
      if0_expr* if0 = (if0_expr*)expr;

      return has_sym(if0->tst) || has_sym(if0->thn) || has_sym(if0->els);
    }
  }

  return FALSE;
}

static int definitely_number(tagged* expr)
{
  switch (TAGGED_TYPE(expr)) {
  case num_type:
  case plus_type:
  case minus_type:
  case times_type:
    return TRUE;
  }

  return FALSE;
}

static int definitely_function(tagged* expr)
{
  switch (TAGGED_TYPE(expr)) {
  case lambda_type:
  case func_type:
    return TRUE;
  }

  return FALSE;
}

#endif /* USE_JIT */
