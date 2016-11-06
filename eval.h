
#ifndef __EVAL_H__
#define __EVAL_H__

#include "struct.h"
#include "hash.h"

tagged* eval(tagged* expr, env* e, hash_table* d);

extern struct tagged* expr;
extern struct env* e;
extern struct cont* todos;
extern struct tagged *val;

#endif
