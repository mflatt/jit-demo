#include <stddef.h>
#include "hash.h"
#include "lookup.h"
#include "struct.h"
#include "eval.h"
#include "compile.h"
#include "check.h"
#include "test.h"

static tagged* zero;
static tagged* one;
static tagged* seven;
static tagged* eight;
static tagged* six;
static tagged* fortynine;
static tagged* branch0;
static tagged* branch1;
static symbol* f;
static symbol* x;
static symbol* y;
static symbol* z;
static tagged* f_var;
static tagged* x_var;
static tagged* z_var;
static tagged* id_lam;
static tagged* id_func;
static tagged* fofz;
static tagged* idofone;
static tagged* eightofone;
static tagged* fiftysix;
static tagged* eightbybranch0;
static tagged* ninebybranch1;
static tagged* app_lam;

static hash_table* empty_d;
static hash_table* fz_d;

static env* y_env;

static tagged* c_one;
static tagged* c_seven;
static tagged* c_eight;
static tagged* c_six;
static tagged* c_fortynine;
static tagged* c_branch0;
static tagged* c_branch1;
static tagged* c_z_var;
static tagged* c_id_lam;
static tagged* c_fofz;
static tagged* c_idofone;
static tagged* c_eightofone;
static tagged* c_fiftysix;
static tagged* c_eightbybranch0;
static tagged* c_ninebybranch1;

static tagged* partial_c_one;


void init_test()
{
  zero = make_num(0);
  one = make_num(1);
  seven = make_num(7);
  eight = make_plus(one, seven);
  six = make_minus(seven, one);
  fortynine = make_times(seven, seven);
  branch0 = make_if0(zero, seven, eight);
  branch1 = make_if0(one, seven, eight);
  f = make_symbol("f");
  x = make_symbol("x");
  y = make_symbol("x");
  z = make_symbol("z");
  f_var = (tagged*)f;
  x_var = (tagged*)x;
  z_var = (tagged*)z;
  id_lam = make_lambda(x, x_var);
  id_func = make_func(make_lambda(x, x_var), NULL);
  fofz = make_app(f_var, z_var);
  idofone = make_app(id_lam, one);
  eightofone = make_app(make_lambda(x, eight), one);
  fiftysix = make_app(make_lambda(x, make_times(eightofone, seven)), one);
  eightbybranch0 = make_app(make_lambda(x, make_plus(branch0, one)), one);
  ninebybranch1 = make_app(make_lambda(x, make_plus(branch1, one)), one);
  app_lam = make_lambda(x, make_app(x_var, one));

  empty_d = make_dict();
  fz_d = make_dict();

  hash_set(fz_d, f, id_func);
  hash_set(fz_d, z, seven);

  c_one = compile(one, NULL, empty_d);
  c_seven = compile(seven, NULL, fz_d);
  c_eight = compile(eight, NULL, fz_d);
  c_six = compile(six, NULL, fz_d);
  c_fortynine = compile(fortynine, NULL, fz_d);
  c_branch0 = compile(branch0, NULL, fz_d);
  c_branch1 = compile(branch1, NULL, fz_d);
  c_z_var = compile(z_var, NULL, fz_d);
  c_id_lam = compile(id_lam, NULL, fz_d);
  c_fofz = compile(fofz, NULL, fz_d);
  c_idofone = compile(idofone, NULL, fz_d);
  c_eightofone = compile(eightofone, NULL, fz_d);
  c_fiftysix = compile(fiftysix, NULL, fz_d);
  c_eightbybranch0 = compile(eightbybranch0, NULL, fz_d);
  c_ninebybranch1 = compile(ninebybranch1, NULL, fz_d);

  partial_c_one = make_app(compile(app_lam, NULL, fz_d),
                           id_lam);

  y_env = make_env(y, one, NULL);
}

void test()
{
  check_ptr(env_lookup(f, NULL, fz_d), id_func);
  check_ptr(env_lookup(z, NULL, fz_d), seven);
  check_ptr(env_lookup(y, y_env, fz_d), one);

  check_num_val(eval(one, NULL, empty_d), 1);

  check_num_val(eval(seven, NULL, fz_d), 7);
  check_num_val(eval(eight, NULL, fz_d), 8);
  check_num_val(eval(six, NULL, fz_d), 6);
  check_num_val(eval(fortynine, NULL, fz_d), 49);
  check_num_val(eval(branch0, NULL, fz_d), 7);
  check_num_val(eval(branch1, NULL, fz_d), 8);
  check_num_val(eval(z_var, NULL, fz_d), 7);
  check_func_val(eval(id_lam, NULL, fz_d));

  check_num_val(eval(fofz, NULL, fz_d), 7);
  check_num_val(eval(idofone, NULL, fz_d), 1);
  check_num_val(eval(eightofone, NULL, fz_d), 8);
  check_num_val(eval(fiftysix, NULL, fz_d), 56);
  check_num_val(eval(eightbybranch0, NULL, fz_d), 8);
  check_num_val(eval(ninebybranch1, NULL, fz_d), 9);

  check_num_val(eval(c_one, NULL, empty_d), 1);

  check_num_val(eval(c_seven, NULL, fz_d), 7);
  check_num_val(eval(c_eight, NULL, fz_d), 8);
  check_num_val(eval(c_six, NULL, fz_d), 6);
  check_num_val(eval(c_fortynine, NULL, fz_d), 49);
  check_num_val(eval(c_branch0, NULL, fz_d), 7);
  check_num_val(eval(c_branch1, NULL, fz_d), 8);
  check_num_val(eval(c_z_var, NULL, fz_d), 7);
  check_func_val(eval(c_id_lam, NULL, fz_d));

  check_num_val(eval(c_fofz, NULL, fz_d), 7);
  check_num_val(eval(c_idofone, NULL, fz_d), 1);
  check_num_val(eval(c_eightofone, NULL, fz_d), 8);
  check_num_val(eval(c_fiftysix, NULL, fz_d), 56);
  check_num_val(eval(c_eightbybranch0, NULL, fz_d), 8);
  check_num_val(eval(c_ninebybranch1, NULL, fz_d), 9);

  check_num_val(eval(partial_c_one, NULL, fz_d), 1);
}

/* ************************************************************ */
/* infinite-loop test */

static symbol* forever;
static tagged* forever_var;
static tagged *forever_func;
static hash_table* forever_d;
static tagged *app_forever;

void init_forever_test()
{
  forever = make_symbol("forever");
  forever_var = (tagged*)forever;
  forever_func = make_func(make_lambda(x,
                                       make_app(forever_var, x_var)),
                           NULL);

  forever_d = make_dict();
  app_forever = make_app(forever_var, make_num(0));

  hash_set(forever_d, forever, forever_func);
}

void forever_test()
{
  eval(app_forever, NULL, forever_d);
}

/* ************************************************************ */
/* a Y combinator */

static tagged* Y;

static tagged* make_let(symbol* lhs, tagged* rhs, tagged* body)
{
  return make_app(make_lambda(lhs, body), rhs);
}

static void init_Y()
{
  symbol* proc;
  symbol* fX;
  tagged* proc_var;
  tagged* fX_var;

  if (Y) return;

  proc = make_symbol("proc");
  proc_var = (tagged*)proc;
  fX = make_symbol("fX");
  fX_var = (tagged*)fX;
  
  Y = make_lambda(proc,
                  make_let(fX,
                           make_lambda(fX,
                                       make_let(f,
                                                make_lambda(x,
                                                            make_app(make_app(fX_var, fX_var),
                                                                     x_var)),
                                                make_app(proc_var, f_var))),
                           make_app(fX_var, fX_var)));
}

/* ************************************************************ */
/* fib test */

static hash_table* fib_d;
static tagged* two;
static symbol* fib;
static tagged* fib_var;
static tagged* fib_lam;
static tagged* fib_func;
static tagged* fib_Y;

static symbol* alt_fib;
static symbol* make_fib;
static symbol* get_self;
static symbol* check1;
static symbol* check2;
static symbol* base;
static tagged* alt_fib_var;
static tagged* make_fib_var;
static tagged* get_self_var;
static tagged* check1_var;
static tagged* check2_var;
static tagged* base_var;
static tagged* make_fib_body;
static tagged* alt_fib_func;
static tagged* app_alt_fib;

static tagged* thirty;
static tagged* app_fib;
static tagged* app_Y_fib;
static int fib_result;

static tagged* c_app_Y_fib;

static tagged* make_fib_lam(tagged* self, tagged* check1_x, tagged* check2_x, tagged* base_val)
{
  return make_lambda(x,
                     make_if0(check1_x,
                              base_val,
                              make_if0(check2_x,
                                       base_val,
                                       make_plus(make_app(self,
                                                          make_minus(x_var, one)),
                                                 make_app(self,
                                                          make_minus(x_var, two))))));
}

void init_fib_test()
{
  env *env;
  
  init_Y();
  
  fib_d = make_dict();
  two = make_num(2);
  fib = make_symbol("fib");
  fib_var = (tagged*)fib;
  fib_lam = make_fib_lam(fib_var, x_var, make_minus(x_var, one), one);
  fib_func = make_func(fib_lam, NULL);

  fib_Y = make_app(Y, make_lambda(fib, fib_lam));
  
  thirty = make_num(30);

  app_fib = make_app(fib_var, thirty);
  app_Y_fib = make_app(fib_Y, thirty);
  
  fib_result = 1346269;

  hash_set(fib_d, fib, fib_func);
  compile_function(fib, fib_d);

  c_app_Y_fib = compile(app_Y_fib, NULL, empty_d);

  /* abstracts fib over tests for two base cases and the base-case result */
  alt_fib = make_symbol("alt-fib");
  make_fib = make_symbol("make-fib");
  get_self = make_symbol("get-self");
  check1 = make_symbol("check1");
  check2 = make_symbol("check2");
  base = make_symbol("base");
  alt_fib_var = (tagged*)alt_fib;
  make_fib_var = (tagged*)make_fib;
  get_self_var = (tagged*)get_self;
  check1_var = (tagged*)check1;
  check2_var = (tagged*)check2;
  base_var = (tagged*)base;
  make_fib_body = make_fib_lam(make_app(get_self_var, one),
                               make_app(check1_var, x_var),
                               make_app(check2_var, x_var),
                               base_var);

  env = make_env(get_self, make_func(make_lambda(x, alt_fib_var), NULL), NULL);
  env = make_env(check1, make_func(make_lambda(x, x_var), NULL), env);
  env = make_env(check2, make_func(make_lambda(x, make_minus(x_var, one)), NULL), env);
  env = make_env(base, one, env);
  alt_fib_func = make_func(make_fib_body, env);

  hash_set(fib_d, alt_fib, alt_fib_func);
  compile_function(alt_fib, fib_d);

  app_alt_fib = make_app(alt_fib_var, thirty);
}

void fib_test()
{
  check_num_val(eval(app_fib, NULL, fib_d), fib_result);
}

void fib_Y_test()
{
  check_num_val(eval(c_app_Y_fib, NULL, empty_d), fib_result);
}

void fib_alt_test()
{
  check_num_val(eval(app_alt_fib, NULL, fib_d), fib_result);
}
