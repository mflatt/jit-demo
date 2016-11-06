#ifndef __STRUCT_H__
#define __STRUCT_H__

#define TRUE 1
#define FALSE 0

enum {
  num_type,
  func_type,
  sym_type,
  plus_type,
  minus_type,
  times_type,
  app_type,
  lambda_type,
  if0_type
};

enum {
  env_type = 100
};

typedef struct env env;

typedef struct tagged {
  int type;
} tagged;

typedef struct num_val {
  tagged t;
  int n;
} num_val;

typedef struct func_val {
  tagged t;
  struct symbol *arg_name;
  tagged *body;
  env* e;
} func_val;

typedef struct symbol {
  tagged t;
  char *s;
} symbol;

typedef struct bin_op_expr {
  tagged t;
  tagged *left, *right;
} bin_op_expr;

typedef struct lambda_expr {
  tagged t;
  symbol *arg_name;
  tagged *body;
} lambda_expr;

typedef struct if0_expr {
  tagged t;
  tagged *tst;
  tagged *thn;
  tagged *els;
} if0_expr;

struct env {
  int type;
  symbol *id;
  tagged *val;
  struct env *rest;
};

tagged* make_num(int n);
tagged* make_func(symbol *arg_name, tagged *body, env *e);
symbol* make_symbol(char *s);
tagged* make_plus(tagged *left, tagged *right);
tagged* make_minus(tagged *left, tagged *right);
tagged* make_times(tagged *left, tagged *right);
tagged* make_app(tagged *left, tagged *right);
tagged* make_lambda(symbol *arg_name, tagged *body);
tagged* make_if0(tagged *tst, tagged *thn, tagged *els);

env* make_env(symbol *id, tagged *val, env *rest);

#endif
