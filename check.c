#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "struct.h"
#include "print.h"
#include "fail.h"

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

