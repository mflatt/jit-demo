#include <stdio.h>
#include "print.h"

void print_val(tagged *t) {
  switch(t->type) {
  case num_type:
    printf("%d", ((num_val*)t)->n);
    break;
  case func_type:
    printf("#<function>");
    break;
  }
  printf("\n");
}
