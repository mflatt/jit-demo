#ifndef __CONTINUE_H__
#define __CONTINUE_H__

#include "struct.h"

enum {
  done_type = 200,
  right_of_bin_type,
  finish_bin_type,
  right_of_app_type,
  finish_app_type,
  finish_if0_type
};

typedef struct cont {
  int type;
} cont;

typedef struct right_of_bin {
  cont c;
  int op;
  tagged *right;
  env *env;
  cont *rest;
} right_of_bin;

typedef struct finish_bin {
  cont c;
  int op;
  tagged *left_val;
  cont *rest;
} finish_bin;

typedef struct right_of_app {
  cont c;
  tagged *right;
  env *env;
  cont *rest;
} right_of_app;

typedef struct finish_app {
  cont c;
  tagged *left_val;
  cont *rest;
} finish_app;

typedef struct finish_if0 {
  cont c;
  tagged *thn;
  tagged *els;
  env *env;
    cont *rest;
} finish_if0;

cont *make_done();
cont *make_right_of_bin(int op, tagged *right, env *env, cont *rest);
cont *make_finish_bin(int op, tagged *left_val, cont *rest);
cont *make_right_of_app(tagged *right, env *env, cont *rest);
cont *make_finish_app(tagged *left_val, cont *rest);
cont *make_finish_if0(tagged *thn, tagged *els, env *env, cont *rest);

#endif
