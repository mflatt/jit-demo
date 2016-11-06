
#ifndef __LOOKUP_H__
#define __LOOKUP_H__

#include "hash.h"
#include "struct.h"

hash_table* make_dict();
tagged* lookup(hash_table* d, symbol* id);
tagged* env_lookup(symbol *id, env* e, hash_table* d);

int env_lookup_pos(symbol *id, env* e);
tagged* env_lookup_by_pos(int pos, env* e);

#endif
