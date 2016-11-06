#include <stdio.h>
#include <stdlib.h>
#include "fail.h"

void* fail(char *s) {
  printf("%s\n", s);
  exit(1);
  return NULL; /* but not really */
}
