
#ifndef __LOOKUP_H__
#define __LOOKUP_H__

#include "hash.h"
#include "struct.h"

hash_table* make_dict();
tagged* lookup(hash_table* d, symbol* id);
tagged* env_lookup(symbol *id, env* e, hash_table* d);

#endif
