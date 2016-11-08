
#ifndef __GC_H__
#define __GC_H__

void gc_init(int heap_size);

void enable_gc();
void disable_gc();

void *gc_malloc0(int sz);
void *gc_malloc1(int sz, void *p1);
void *gc_malloc2(int sz, void *p1, void *p2);
void *gc_malloc3(int sz, void *p1, void *p2, void *p3);
void *gc_malloc4(int sz, void *p1, void *p2, void *p3, void *p4);

int gc_is_collectable(void *p);

#endif
