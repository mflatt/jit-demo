#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "struct.h"
#include "lookup.h"
#include "eval.h"
#include "print.h"
#include "fail.h"
#include "gc.h"

#define RUN_FIB     0
#define RUN_FOREVER 0

#if RUN_FIB
# define HEAP_SIZE 100000
#else
# define HEAP_SIZE 1000
#endif

void check_ptr(void* a, void* b)
{
  if (a != b) {
    fail("failed ptr");
  }
}

void check_num_val(tagged* r, int n)
{
  if (TAGGED_TYPE(r) != num_type) {
    printf("not a number: ");
  } else {
    if (n != NUM_VAL(r))
      printf("not expected number %d: ", n);
    else
      return;
  }
  print_val(r);
  fail("failed num_val");
}

void check_func_val(tagged* r)
{
  if (TAGGED_TYPE(r) != func_type) {
    printf("not a function: ");
    print_val(r);
    fail("failed func_val");
  }
}

int main()
{
  tagged* one = make_num(1);
  tagged* seven = make_num(7);
  tagged* eight = make_plus(one, seven);
  tagged* six = make_minus(seven, one);
  tagged* fortynine = make_times(seven, seven);
  symbol* f = make_symbol("f");
  symbol* x = make_symbol("x");
  symbol* y = make_symbol("x");
  symbol* z = make_symbol("z");
  tagged* f_var = (tagged*)f;
  tagged* x_var = (tagged*)x;
  tagged* z_var = (tagged*)z;
  tagged *id_lam = make_lambda(x, x_var);
  tagged *id_func = make_func(x, x_var, NULL);
  tagged *fofz = make_app(f_var, z_var);

  hash_table* empty_d = make_dict();
  hash_table* fz_d = make_dict();

#if RUN_FOREVER
  symbol* forever = make_symbol("forever");
  tagged* forever_var = (tagged*)forever;
  tagged *forever_func 
    = make_func(x,
                make_app(forever_var, x_var),
                NULL);

  hash_table* forever_d = make_dict();
  tagged *app_forever = make_app(forever_var, make_num(0));
#endif
  
  env* y_env = make_env(y, one, NULL);

#if RUN_FIB
  hash_table* fib_d = make_dict();
  tagged* two = make_num(2);
  symbol* fib = make_symbol("fib");
  tagged* fib_var = (tagged*)fib;
  tagged* fib_func
    = make_func(x,
                make_if0(x_var,
                         one,
                         make_if0(make_minus(x_var, one),
                                  one,
                                  make_plus(make_app(fib_var,
                                                     make_minus(x_var, one)),
                                            make_app(fib_var,
                                                     make_minus(x_var, two))))),
                NULL);
  tagged* thirty = make_num(30);
  tagged* app_fib = make_app(fib_var, thirty);
  int fib_result = 1346269;
#endif
  
  gc_init(HEAP_SIZE);

  hash_set(fz_d, f, id_func);
  hash_set(fz_d, z, seven);
  check_ptr(env_lookup(f, NULL, fz_d), id_func);
  check_ptr(env_lookup(z, NULL, fz_d), seven);
  check_ptr(env_lookup(y, y_env, fz_d), one);

  check_num_val(eval(one, NULL, empty_d), 1);

  check_num_val(eval(seven, NULL, fz_d), 7);
  check_num_val(eval(eight, NULL, fz_d), 8);
  check_num_val(eval(six, NULL, fz_d), 6);
  check_num_val(eval(fortynine, NULL, fz_d), 49);
  check_num_val(eval(z_var, NULL, fz_d), 7);
  check_func_val(eval(id_lam, NULL, fz_d));

  check_num_val(eval(fofz, NULL, fz_d), 7);

#if RUN_FIB
  hash_set(fib_d, fib, fib_func);
  check_num_val(eval(app_fib, NULL, fib_d), fib_result);
#endif

#if RUN_FOREVER
  /* runs -- and GCs -- forever in constant space: */
  hash_set(forever_d, forever, forever_func);
  eval(app_forever, NULL, forever_d);
#endif

  printf("all tests passed\n");

  return 0;
}
