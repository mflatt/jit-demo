
#ifndef __HASH_H__
#define __HASH_H__

typedef int (*compare_proc)(void* a, void* b);
/* compare result: 0 => same, not 0 => different */

typedef int (*hash_proc)(void* a);
/* hash result must be non-negative */

struct hash_table;
typedef struct hash_table hash_table;

hash_table* make_hash_table(compare_proc compare_key,
                            hash_proc hash_key);
void* hash_get(hash_table *ht, void* k);
void hash_set(hash_table *ht, void* k, void* val);
int hash_count(hash_table *ht);

struct hash_iter;
typedef struct hash_iter hash_iter;

hash_iter* hash_iterate_new(hash_table *ht);
hash_iter* hash_iterate_next(hash_table *ht, hash_iter* i);
void* hash_iterate_key(hash_table *ht, hash_iter* i);

#endif
