
#ifndef __COMPILE_H__
#define __COMPILE_H__

#include "struct.h"
#include "hash.h"

tagged* compile(tagged* expr, env* e, hash_table* d);
void compile_function(symbol* fib, hash_table *d);

#endif
