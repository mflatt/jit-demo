
#ifndef __JIT_H__
#define __JIT_H__

int jit(lambda_expr *lam);

#if USE_JIT
void push_jit_stack(void (*paint_gray)(void *));
#endif

#endif
