#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "struct.h"
#include "lookup.h"
#include "eval.h"
#include "compile.h"
#include "check.h"
#include "gc.h"
#include "test.h"

#define RUN_FIB     1
#define RUN_FOREVER 0

#if RUN_FIB
# define HEAP_SIZE 100000
#else
# define HEAP_SIZE 1000
#endif

int main()
{
  init_test();
  init_forever_test();
  init_fib_test();

  gc_init(HEAP_SIZE, FALSE);

  test();
  
# if RUN_FIB
  fib_alt_test();
# endif

# if RUN_FOREVER
  /* runs -- and GCs -- forever in constant space: */
  forever_test();
# endif

  printf("all tests passed\n");

  return 0;
}
