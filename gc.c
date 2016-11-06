#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gc.h"
#include "eval.h"
#include "struct.h"
#include "continue.h"
#include "fail.h"

static char *to_start, *to_pos, *to_end;
static char *from_start, *from_end;

void gc_init(int heap_size) {
  to_start = malloc(heap_size);
  to_pos = to_start;
  to_end = to_start + heap_size;

  from_start = malloc(heap_size);
  from_end = from_start + heap_size;
}

static void collect_garbage(void *p1, void *p2, void *p3, void *p4);

/************************************************************/

static int enabled = 0;

void enable_gc()
{
  enabled++;
}

void disable_gc()
{
  enabled--;
}

/************************************************************/

typedef struct gcable {
  int tag;
  void *forward;
} gcable;

static int align_size(int sz)
{
  /* make sure there's enough room for a warding pointer: */
  if (sz < sizeof(gcable))
    sz = sizeof(gcable);
  
  /* 16-byte alignment: */
  if ((sz % 16) > 0) {
    sz += (16 - (sz % 16));
  }

  return sz;
}

/************************************************************/

void *gc_malloc4(int sz, void *p1, void *p2, void *p3, void *p4)
{
  if (enabled) {
    sz = align_size(sz);
    while (1) {
      if (to_pos + sz < to_end) {
        void *p = to_pos;
        to_pos += sz;
        return p;
      } else {
        collect_garbage(p1, p2, p3, p4);
        if (to_pos + sz >= to_end)
          return fail("out of memory");
      }
    }
  } else {
    return malloc(sz);
  }
}

void *gc_malloc0(int sz)
{
  return gc_malloc4(sz, NULL, NULL, NULL, NULL);
}

void *gc_malloc1(int sz, void *p1)
{
  return gc_malloc4(sz, p1, NULL, NULL, NULL);
}

void *gc_malloc2(int sz, void *p1, void *p2)
{
  return gc_malloc4(sz, p1, p2, NULL, NULL);
}

void *gc_malloc3(int sz, void *p1, void *p2, void *p3)
{
  return gc_malloc4(sz, p1, p2, p3, NULL);
}

/************************************************************/

static int gcable_size(int tag) 
{
  int sz;

  switch (tag) {
  case num_type:
    sz = sizeof(num_val);
    break;
  case func_type:
    sz = sizeof(func_val);
    break;
  case sym_type:
    sz = sizeof(symbol);
    break;
  case plus_type:
  case minus_type:
  case times_type:
  case app_type:
    sz = sizeof(bin_op_expr);
    break;
  case lambda_type:
    sz = sizeof(lambda_expr);
    break;
  case env_type:
    sz = sizeof(env);
    break;
  case done_type:
    sz = sizeof(cont);
    break;
  case right_of_bin_type:
    sz = sizeof(right_of_bin);
    break;
  case finish_bin_type:
    sz = sizeof(finish_bin);
    break;
  case right_of_app_type:
    sz = sizeof(right_of_app);
    break;
  case finish_app_type:
    sz = sizeof(finish_app);
    break;
  case finish_if0_type:
    sz = sizeof(finish_if0);
    break;
  default:
    fail("bad tag for sizeof");
    return 0;
  }

  return align_size(sz);
}

static void paint_gray(void *_pp) 
/* Paint the object at *pp gray and update *pp, or
   if the object is already gray/black, just update
   *pp using the forwarding pointer. */
{
  void **pp = (void **)_pp;

  /* First, check whether the referenced object is gcable,
     because it might instead have been allocated with
     gc disabled: */
  if (((char*)*pp >= from_start) && ((char*)*pp < from_end)) {
    /* It's a reference to a gcable object */
    gcable *p = (gcable *)*pp;

    if (p->tag == -1) {
      /* already gray or black, so use forwarding pointer */
      *pp = p->forward;
    } else {
      /* paint it gray: */
      void *dest = to_pos;
      int sz;

      sz = gcable_size(p->tag);

      to_pos += sz;
      memcpy(dest, p, sz);

      /* install forwarding pointer: */
      p->tag = -1;
      p->forward = dest;

      *pp = dest;
    }
  }
}

static void follow_one_gray_pointers(void *p)
{
  switch (((gcable *)p)->tag) {
  case num_type:
    break;
  case func_type:
    {
      func_val *fv = (func_val *)p;
      paint_gray(&fv->arg_name);
      paint_gray(&fv->body);
      paint_gray(&fv->e);
    }
    break;
  case sym_type:
    break;
  case plus_type:
  case minus_type:
  case times_type:
  case app_type:
    {
      bin_op_expr *bn = (bin_op_expr *)p;
      paint_gray(&bn->left);
      paint_gray(&bn->right);
    }
    break;
  case lambda_type:
    {
      lambda_expr *lam = (lambda_expr *)p;
      paint_gray(&lam->arg_name);
      paint_gray(&lam->body);
    }
    break;
  case env_type:
    {
      env *e = (env *)p;
      paint_gray(&e->id);
      paint_gray(&e->val);
      paint_gray(&e->rest);
    }
    break;
  case done_type:
    break;
  case right_of_bin_type:
    {
      right_of_bin *gl = (right_of_bin *)p;
      paint_gray(&gl->right);
      paint_gray(&gl->env);
      paint_gray(&gl->rest);
    }
    break;
  case finish_bin_type:
    {
      finish_bin *gr = (finish_bin *)p;
      paint_gray(&gr->left_val);
      paint_gray(&gr->rest);
    }
    break;
  case right_of_app_type:
    {
      right_of_app *gl = (right_of_app *)p;
      paint_gray(&gl->right);
      paint_gray(&gl->env);
      paint_gray(&gl->rest);
    }
    break;
  case finish_app_type:
    {
      finish_app *gr = (finish_app *)p;
      paint_gray(&gr->left_val);
      paint_gray(&gr->rest);
    }
    break;
  case finish_if0_type:
    {
      finish_if0 *gi = (finish_if0 *)p;
      paint_gray(&gi->thn);
      paint_gray(&gi->els);
      paint_gray(&gi->env);
      paint_gray(&gi->rest);
    }
    break;
  default:
    fail("bad tag for paint_gray content");
    break;
  }
}

static void collect_garbage(void *p1, void *p2, void *p3, void *p4)
{
  char *old_start, *old_end, *gray_pos;
  int old_size = to_pos - to_start;

  old_start = to_start;
  old_end = to_end;
  
  to_start = from_start;
  to_end = from_end;
  to_pos = to_start;

  from_start = old_start;
  from_end = old_end;

  paint_gray(&expr);
  paint_gray(&e);
  paint_gray(&todos);
  paint_gray(&val);
  if (p1) paint_gray(p1);
  if (p2) paint_gray(p2);
  if (p3) paint_gray(p3);
  if (p4) paint_gray(p4);

  gray_pos = to_start;
  while (gray_pos < to_pos) {
    follow_one_gray_pointers(gray_pos);
    gray_pos += gcable_size(((gcable *)gray_pos)->tag);
  }

  printf("[collected %d]\n", (int)(old_size - (to_pos - to_start)));
}
