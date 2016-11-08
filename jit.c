#include <stddef.h>
#include "struct.h"


#if !USE_JIT

int jit(func_val* fv, hash_table* d) {
  return FALSE;
}

#else

#include <lightning.h>
#include "gc.h"
#include "eval.h"
#include "continue.h"
#include "lookup.h"
#include "fail.h"

#define jit_movi_p(dest, addr) jit_movi(dest, (jit_word_t)(addr))
#define jit_pushargi_p(addr) jit_pushargi((jit_word_t)(addr))

static int inited = 0;

typedef struct context {
  hash_table* d;
  symbol* arg_name;
  int stack_pos;
  env *env;
  int env_start;
  int specialize;

  tagged* inline_arg;
  struct context *inline_from;
} context;

static jit_state_t *_jit;

static void jit_expr(tagged* expr, context *ctx);
static void jit_variable(int pos, context *ctx);
static void jit_cont_expr(tagged* expr, int cont_type, context *ctx);
static jitted_proc jit_continue_by_interp(tagged* body);

static void jit_cont_make(tagged* expr, context *ctx, int cont_type, int jit_cont_type);

static void do_jit_next();
static void continue_or_return();

static void jit_check_type(int need_type, int reg, char *complain);
static void jit_set_stack(int stack_pos, int reg);
static void jit_get_stack(int reg, int stack_pos);
static void jit_prep_possible_gc(int stack_pos, int tmp_reg);
static void jit_movi_maybe_gc(int reg, tagged* v);

static void jit_maybe_specialize(int stack_pos, context *ctx);
static func_val *realloc_lam(func_val *fv);
static tagged* extract_known(int pos, context *ctx);

static int no_calls(tagged* expr, context *ctx);
static int want_to_jit(lambda_expr* lam);
static int definitely_number(tagged* expr);
static int definitely_function(tagged* expr);
static int can_inline(bin_op_expr* bn, context *ctx, int without_introducing_calls, func_val **_fv);

#define STACK_SIZE 64
static tagged* stack[STACK_SIZE];
static int stack_gc_depth = 0;

#define MAX_CODE_ROOTS 128
static tagged* code_roots[MAX_CODE_ROOTS];
static int code_roots_count = 0;

#define FRAME_SIZE 64

void push_jit_roots(void (*paint_gray)(void *))
{
  int i;

  for (i = 0; i < code_roots_count; i++)
    paint_gray(&code_roots[i]);

  for (i = 0; i < stack_gc_depth; i++)
    paint_gray(&stack[i]);
}

#if !FIXNUM_ENCODING
# error "JIT requires fixnum encoding"
#endif

static void init_context(context *ctx, hash_table* d, symbol* arg_name, env* e)
{
  ctx->d = d;
  ctx->arg_name = arg_name;
  ctx->stack_pos = 0;
  ctx->env = e;
  ctx->env_start = 1;
  ctx->specialize = 0;

  ctx->inline_arg = NULL;
  ctx->inline_from = NULL;
}

int jit(func_val *fv, hash_table* d)
{
  jit_state_t *old_jit;
  jit_node_t *after_prolog;
  lambda_expr *lam = fv->lam;
  context ctx;

  init_context(&ctx, d, lam->arg_name, fv->e);

  if ((fv->specialize_counter == 1) && want_to_jit(lam)) {
    fv->specialize_counter = 0;
    if (fv->e) {
      fv = realloc_lam(fv);
      lam = fv->lam;
      ctx.env = fv->e;
      ctx.arg_name = lam->arg_name;
      ctx.specialize = 1;
    }
  }

  if (lam->code)
    return TRUE; /* already JIT-compiled */

  if (lam->tail_code)
    return FALSE; /* tail code continues via interp */

  if (!want_to_jit(lam)) {
    lam->tail_code = jit_continue_by_interp(lam->body);
    return FALSE;
  }

  if (!inited) {
    inited = 1;
    init_jit(NULL);
  }

  old_jit = _jit;
  _jit = jit_new_state();

  jit_prolog();
  jit_frame(FRAME_SIZE);
  after_prolog = jit_indirect();

  jit_expr(lam->body, &ctx);

  continue_or_return();
  
  lam->code = jit_emit();
  lam->tail_code = jit_address(after_prolog);

  jit_clear_state();
  _jit = old_jit;
  
  return TRUE;
}

jitted_proc jit_cont(tagged* expr, context *ctx, int cont_type, int jit_cont_type, jitted_proc *_tail_code)
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
 
  jit_cont_expr(expr, cont_type, ctx);

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

jitted_proc jit_continue_by_interp(tagged* body)
{
  jit_state_t *old_jit;
  jitted_proc code;
  jit_node_t *after_prolog;

  if (!inited) {
    inited = 1;
    init_jit(NULL);
  }

  old_jit = _jit;
  _jit = jit_new_state();

  jit_prolog();
  jit_frame(FRAME_SIZE);

  after_prolog = jit_indirect();

  jit_movi_maybe_gc(JIT_R0, body);
  jit_ldi(JIT_R1, &todos);

  jit_prepare();
  jit_pushargr(JIT_R0);
  jit_pushargr(JIT_R1);
  jit_finishi(make_interp);

  jit_retval(JIT_R0);
  jit_sti(&todos, JIT_R0);

  jit_prep_possible_gc(0, JIT_R2);

  jit_movi_p(JIT_R0, make_num(0));
  jit_retr(JIT_R0);

  (void)jit_emit();
  code = jit_address(after_prolog);

  jit_clear_state();
  _jit = old_jit;
  
  return code;
}

static void jit_expr_push(tagged* expr, context *ctx)
{
  ctx->stack_pos++;
  jit_expr(expr, ctx);
  --ctx->stack_pos;
}

static void jit_expr(tagged* expr, context *ctx)
{
  switch (TAGGED_TYPE(expr)) {
  case num_type:
  case func_type:
    jit_movi_maybe_gc(JIT_R0, expr);
    break;
  case sym_type:
    {
      int pos;
      if (same_symbol((symbol*)expr, ctx->arg_name))
        jit_variable(0, ctx);
      else {
        pos = env_lookup_pos((symbol *)expr, ctx->env);
        if (pos != -1)
          jit_variable(pos+1, ctx);
        else
          jit_movi_maybe_gc(JIT_R0, lookup(ctx->d, (symbol *)expr));
      }
    }
    break;
  case debruijn_type:
    {
      int pos = ((debruijn_expr*)expr)->pos;
      jit_variable(pos, ctx);
    }
    break;
  case plus_type:
  case minus_type:
  case times_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      if (no_calls(bn->left, ctx) && no_calls(bn->right, ctx)) {
        jit_expr(bn->left, ctx);

        jit_set_stack(ctx->stack_pos, JIT_R0);
        
        jit_expr_push(bn->right, ctx);

        jit_get_stack(JIT_R1, ctx->stack_pos);

        jit_cont_expr(expr, finish_bin_type, ctx);
      } else {
        jit_cont_make(expr, ctx, right_of_bin_type, right_jitted_type);
        jit_expr(bn->left, ctx);
        do_jit_next();
      }
    }
    break;
  case if0_type:
    {
      if0_expr* if0 = (if0_expr*)expr;

      if (no_calls(if0->tst, ctx)) {
        jit_expr(if0->tst, ctx);

        jit_cont_expr(expr, finish_if0_type, ctx);
      } else {
        jit_cont_make(expr, ctx, finish_if0_type, right_jitted_type);
        jit_expr(if0->tst, ctx);
        do_jit_next();
      }
    }
    break;
  case lambda_type:
    {
      lambda_expr *lam = (lambda_expr *)expr;

      jit_ldi(JIT_R0, &e);      

      jit_prep_possible_gc(ctx->stack_pos, JIT_R2);

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
      func_val *fv;

      if (can_inline(bn, ctx, FALSE, &fv)) {
        /* inline a call */
        context in_ctx;
        init_context(&in_ctx, ctx->d, fv->lam->arg_name, fv->e);
        in_ctx.inline_arg = bn->right;
        in_ctx.inline_from = ctx;
        jit_expr(fv->lam->body, &in_ctx);
      } else {
        if (no_calls(bn->left, ctx) && no_calls(bn->right, ctx)) {
          jit_expr(bn->left, ctx);

          jit_set_stack(ctx->stack_pos, JIT_R0);

          jit_expr_push(bn->right, ctx);

          jit_get_stack(JIT_R1, ctx->stack_pos);

          jit_cont_expr(expr, finish_app_type, ctx);
        } else  {
          jit_cont_make(expr, ctx, right_of_app_type, right_jitted_type);
          jit_expr(bn->left, ctx);
          do_jit_next();
        }
      }
    }
    break;
  default:
    fail("unrecognized expression in JIT compile");
  }
}

static void jit_variable(int pos, context *ctx)
{
  tagged* val;
  val = extract_known(pos, ctx);
  if (val) {
    if (TAGGED_TYPE(val) == debruijn_type) {
      /* due to inlining */
      jit_variable(pos + ((debruijn_expr*)val)->pos + 1, ctx);
    } else if (TAGGED_TYPE(val) == sym_type) {
      /* due to inlining */
      
    } else
      jit_expr(val, ctx);
  } else if ((pos == 0) && ctx->inline_arg) {
    jit_expr(ctx->inline_arg, ctx->inline_from);
  } else {
    jit_ldi(JIT_R2, &e);
    while (pos--)
      jit_ldxi(JIT_R2, JIT_R2, offsetof(env, rest));
    jit_ldxi(JIT_R0, JIT_R2, offsetof(env, val));
  }
}

static void jit_cont_expr(tagged* expr, int cont_type, context *ctx)
{
  switch (cont_type) {
  case right_of_bin_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      if (no_calls(bn->right, ctx)) {
        jit_set_stack(ctx->stack_pos, JIT_R0);
        jit_expr_push(bn->right, ctx);
        jit_get_stack(JIT_R1, ctx->stack_pos);
        jit_cont_expr(expr, finish_bin_type, ctx);
      } else {
        jit_cont_make(expr, ctx, finish_bin_type, finish_jitted_type);
        jit_expr(bn->right, ctx);
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
      
      jit_expr(if0->thn, ctx);
      done = jit_jmpi();
      
      jit_patch(branch);
      jit_expr(if0->els, ctx);
      
      jit_patch(done);
    }
    break;
  case right_of_app_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      if (no_calls(bn->right, ctx)) {
        jit_set_stack(ctx->stack_pos, JIT_R0);
        jit_expr_push(bn->right, ctx);
        jit_get_stack(JIT_R1, ctx->stack_pos);
        jit_cont_expr(expr, finish_app_type, ctx);
      } else {
        jit_cont_make(expr, ctx, finish_app_type, finish_jitted_type);
        jit_expr(bn->right, ctx);
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
      jit_set_stack(ctx->stack_pos, JIT_R1);
      jit_set_stack(ctx->stack_pos+1, JIT_R0);
      
      jit_prep_possible_gc(ctx->stack_pos + 2, JIT_R2);

      jit_maybe_specialize(ctx->stack_pos, ctx);
      
      jit_ldxi(JIT_R2, JIT_R1, offsetof(func_val, lam));
      jit_ldxi(JIT_R0, JIT_R2, offsetof(lambda_expr, tail_code));
      branch = jit_bnei(JIT_R0, (jit_word_t)NULL);

      {
        /* Not yet JIT-compiled, so compile it now */
        jit_movi_p(JIT_R0, ctx->d);
        jit_prepare();
        jit_pushargr(JIT_R1);
        jit_pushargr(JIT_R0);
        jit_finishi(jit);
      }
      
      jit_patch(branch);

      /* Extend the environment */
      jit_get_stack(JIT_R1, ctx->stack_pos);
      jit_get_stack(JIT_R0, ctx->stack_pos+1);
      jit_ldxi(JIT_R2, JIT_R1, offsetof(func_val, e));
      jit_ldxi(JIT_R1, JIT_R1, offsetof(func_val, lam));
      jit_ldxi(JIT_R1, JIT_R1, offsetof(lambda_expr, arg_name));
      
      jit_prepare();
      jit_pushargr(JIT_R1); /* argument name */
      jit_pushargr(JIT_R0); /* argument value */
      jit_pushargr(JIT_R2); /* env */
      jit_finishi(make_env);
      jit_retval(JIT_R0);

      jit_sti(&e, JIT_R0);

      /* Jump to the called function's body */
      jit_get_stack(JIT_R1, ctx->stack_pos);
      jit_ldxi(JIT_R0, JIT_R1, offsetof(func_val, lam));
      jit_ldxi(JIT_R0, JIT_R0, offsetof(lambda_expr, tail_code));
      jit_jmpr(JIT_R0);
    }
    break;
  default:
    fail("unrecognized continuation type in JIT compile");
  }
}

static void jit_cont_make(tagged* expr, context *ctx, int cont_type, int jit_cont_type)
{
  jitted_proc cont_code, cont_tail_code;
  
  cont_code = jit_cont(expr, ctx, cont_type, jit_cont_type, &cont_tail_code);
  
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

static void jit_movi_maybe_gc(int reg, tagged* v)
{
  if (gc_is_collectable(v)) {
    if (code_roots_count >= MAX_CODE_ROOTS)
      fail("too many GCable values in JIT-generated code");
    code_roots[code_roots_count] = v;
    jit_ldi(reg, &code_roots[code_roots_count]);
    code_roots_count++;
  } else
    jit_movi_p(reg, v);
}

static void jit_maybe_specialize(int stack_pos, context *ctx)
{
  /* JIT_R1 holds a function, and the function is also
     on the local stack at position 0 */
  jit_node_t *branch, *branch2;
  
  /* Check specialization counter */
  jit_ldxi_i(JIT_R2, JIT_R1, offsetof(func_val, specialize_counter));
  branch = jit_beqi(JIT_R2, 0);
  
  jit_subi(JIT_R2, JIT_R2, 1);
  jit_stxi_i(offsetof(func_val, specialize_counter), JIT_R1, JIT_R2);
  
  /* If specialize_counter goes to 1, then specialize */
  branch2 = jit_bnei(JIT_R2, 1);
  jit_movi_p(JIT_R0, ctx->d);
  jit_prepare();
  jit_pushargr(JIT_R1);
  jit_pushargr(JIT_R0);
  jit_finishi(jit);

  /* Reload function into JIT_R1 */
  jit_get_stack(JIT_R1, stack_pos);

  jit_patch(branch);
  jit_patch(branch2);
}

static func_val *realloc_lam(func_val *fv)
{
  lambda_expr* lam = fv->lam;
  
  if (gc_is_collectable(fv)) {
    code_roots[code_roots_count++] = (tagged*)fv;
    lam = (lambda_expr*)make_lambda(lam->arg_name,
                                    lam->body);
    fv = (func_val*)code_roots[--code_roots_count];
  } else {
    disable_gc();
    lam = (lambda_expr*)make_lambda(lam->arg_name,
                                    lam->body);
    enable_gc();
  }
  fv->lam = lam;

  return fv;
}

static tagged* extract_known(int pos, context *ctx)
{
  if (ctx->specialize && (pos >= ctx->env_start)) {
    /* specializing; use known value */
    env *env = ctx->env;
    pos -= ctx->env_start;
    while (pos--)
      env = env->rest;
    return env->val;
  } else
    return NULL;
}

static int no_calls(tagged* expr, context *ctx)
{
  switch (TAGGED_TYPE(expr)) {
  case app_type:
    return ctx && can_inline((bin_op_expr*)expr, ctx, TRUE, NULL);
  case plus_type:
  case minus_type:
  case times_type:
    {
      bin_op_expr* bn = (bin_op_expr*)expr;

      return no_calls(bn->left, ctx) && no_calls(bn->right, ctx);
    }
  case if0_type:
    {
      if0_expr* if0 = (if0_expr*)expr;

      return (no_calls(if0->tst, ctx)
              && no_calls(if0->thn, ctx)
              && no_calls(if0->els, ctx));
    }
  }

  return TRUE;
}

static int want_to_jit(lambda_expr* lam)
{
  return TRUE;
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

static int can_inline(bin_op_expr* bn, context *ctx, int without_introducing_calls, func_val **_fv)
{
  if (TAGGED_TYPE(bn->left) == debruijn_type) {
    tagged* val = extract_known(((debruijn_expr*)bn->left)->pos, ctx);
    if (val && TAGGED_TYPE(val) == func_type) {
      /* Simpe enough argument? */
      switch (TAGGED_TYPE(bn->right)) {
      case num_type:
      case func_type:
      case debruijn_type:
      case sym_type:
        {
          func_val* fv = (func_val*)val;
          if (_fv)
            *_fv = fv;
          if (without_introducing_calls)
            return no_calls(fv->lam->body, NULL);
          return TRUE;
        }
        break;
      }
    }
  }

  return FALSE;
}

#endif /* USE_JIT */
