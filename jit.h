
#ifndef __JIT_H__
#define __JIT_H__

int jit(func_val *fv, hash_table* d);

#if USE_JIT
void push_jit_roots(void (*paint_gray)(void *));
#endif

#endif
