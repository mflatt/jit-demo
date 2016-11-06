#include <stdlib.h>
#include <string.h>
#include "hash.h"

const double load_factor = 0.5;

typedef struct entry {
  void* key;
  void* val;
} entry;

struct hash_table {
  int size;
  int count;
  entry *a;
  compare_proc compare_key;
  hash_proc hash_key;
};

void resize_hash_table(hash_table *ht);

void reset_hash_table(hash_table *ht, int size)
{
  int i;

  ht->size = size;
  ht->count = 0;

  ht->a = (entry*)malloc(ht->size * sizeof(entry));
  for (i = 0; i < size; i++)
    ht->a[i].key = NULL;
}

hash_table* make_hash_table(compare_proc compare_key,
                            hash_proc hash_key)
{
  hash_table *ht;

  ht = (hash_table *)malloc(sizeof(hash_table));
  ht->compare_key = compare_key;
  ht-> hash_key = hash_key;

  reset_hash_table(ht, 8);
  
  return ht;
}

void* hash_get(hash_table *ht, void* k)
{
  int n = ht->hash_key(k) % ht->size;

  while (1) {
    if (!ht->a[n].key)
      return NULL;
    if (!ht->compare_key(ht->a[n].key, k))
      return ht->a[n].val;
    n = (n + 1) % ht->size;
  }
}

void hash_set(hash_table *ht, void* k, void* val)
{
  int n = ht->hash_key(k) % ht->size;

  while (1) {
    if (!ht->a[n].key)
      break;
    if (!ht->compare_key(ht->a[n].key, k)) {
      ht->a[n].val = val;
      return;
    }
    n = (n + 1) % ht->size;
  }

  ht->a[n].key = k;
  ht->a[n].val = val;

  ht->count++;

  if (ht->count > ht->size * load_factor)
    resize_hash_table(ht);
}

void resize_hash_table(hash_table *ht) {
  int i, size = ht->size;
  entry *a = ht->a;
  
  reset_hash_table(ht, size * 2);
  for (i = 0; i < size; i++) {
    if (a[i].key)
      hash_set(ht, a[i].key, a[i].val);
  }
}

int hash_count(hash_table *ht)
{
  return ht->count;
}

struct hash_iter {
  int pos;
};

hash_iter* hash_iterate_new(hash_table *ht) {
  hash_iter* i = (hash_iter*)malloc(sizeof(hash_iter));
  i->pos = 0;
  return i;
}

hash_iter* hash_iterate_next(hash_table *ht, hash_iter* i) {
  i->pos++;
  if (i->pos >= ht->size)
    return NULL;
  else
    return i;
}

void* hash_iterate_key(hash_table *ht, hash_iter* i) {
  return ht->a[i->pos].key;
}
