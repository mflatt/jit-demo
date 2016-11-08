#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "struct.h"
#include "lookup.h"
#include "fail.h"

static int compare_sym(void* _s1, void* _s2) {
  return strcmp(((symbol*)_s1)->s, ((symbol*)_s2)->s);
}

static int hash_sym(void* _s) {
  char* s = ((symbol *)_s)->s;
  int i, r = 0;

  for (i = 0; s[i] != 0; i++) {
    r *= 10;
    r += s[i];
  }

  if (r < 0) r = -r;

  return r;
}

hash_table* make_dict() {
  return make_hash_table(compare_sym, hash_sym);
}

tagged* lookup(hash_table* d, symbol* id) {
  tagged *val;

  val = hash_get(d, id);
  if (!val)
    fail("undefined");

  return val;
}

/*****************************************/

tagged* env_lookup(symbol *id, env* e, hash_table* d) {
  while (e) {
    if (same_symbol(id, e->id))
      return e->val;
    e = e->rest;
  }

  return lookup(d, id);
}

int env_lookup_pos(symbol *id, env* e) {
  int pos = 0;
  while (e) {
    if (same_symbol(id, e->id))
      return pos;
    pos++;
    e = e->rest;
  }

  return -1;
}

tagged* env_lookup_by_pos(int pos, env* e) {
  while (e && pos--) {
    e = e->rest;
  }

  if (!e)
    fail("bad position");
  return e->val;
}
