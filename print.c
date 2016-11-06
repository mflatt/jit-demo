#include <stdio.h>
#include <stdint.h>
#include "print.h"

void print_val(tagged *t) {
  switch(TAGGED_TYPE(t)) {
  case num_type:
    printf("%ld", (long)NUM_VAL(t));
    break;
  case func_type:
    printf("#<function>");
    break;
  }
  printf("\n");
}
