/*
 *
 * Copyright (c) 2017 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <head.h>
#include "fns.h"

#define L1X(va)		(va >> 20)
#define L2X(va)		((va >> 12) & ((1 << 8) - 1))

#define L1_TYPE 	0b11
#define L1_FAULT	0b00
#define L1_COARSE	0b01
#define L1_SECTION	0b10
#define L1_FINE		0b11

#define L2_TYPE 	0b11
#define L2_FAULT	0b00
#define L2_LARGE	0b01
#define L2_SMALL	0b10
#define L2_TINY		0b11

uint32_t
ttb[4096]__attribute__((__aligned__(16*1024))) = { L1_FAULT };

struct addrspace *space;
uint32_t low = UINT_MAX;
uint32_t high = 0;

void
mmuinit(void)
{
  int i;

  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  mmuloadttb(ttb);
}

void
mmuempty1(void)
{
  uint32_t i;

  for (i = low; i <= high; i++) {
    if ((ttb[i] & L1_TYPE) == L1_COARSE) {
      ttb[i] = L1_FAULT;
    }
  }

  mmuinvalidate();

  low = UINT_MAX;
  high = 0;
}

void
imap(void *start, void *end, int ap, bool cachable)
{
  uint32_t x, mask;

  x = (uint32_t) start & ~((1 << 20) - 1);

  mask = (ap << 10) | L1_SECTION;

  if (cachable) {
    mask |= (7 << 12) | (1 << 3) | (0 << 2);
  } else {
    mask |= (0 << 12) | (0 << 3) | (1 << 2);
  }
  
  while (x < (uint32_t) end) {
    /* Map section so everybody can see it.
     * This wil change. */
    ttb[L1X(x)] = x | mask;
    x += 1 << 20;
  }
}

static void
mmuswitchstack(struct stack *s)
{
  if (s->l2 != 0) {
    ttb[L1X(s->bottom)] = s->l2 | L1_COARSE;
    if (L1X((uint32_t) s->bottom) > high) {
      high = L1X((uint32_t) s->bottom);
    } else if (L1X((uint32_t) s->bottom) < low) {
      low = L1X((uint32_t) s->bottom);
    }
  }
}

static void
mmuswitchl2(struct l2 l2s[], size_t len)
{
  struct l2 *l2;
  size_t i;

  for (i = 0; i < len; i++) {
    l2 = &l2s[i];
    if (l2->va == 0) {
      break;
    }

    ttb[L1X(l2->va)] = l2->pa | L1_COARSE;
    if (L1X((uint32_t) l2->va) > high) {
      high = L1X((uint32_t) l2->va);
    } else if (L1X((uint32_t) l2->va) < low) {
      low = L1X((uint32_t) l2->va);
    }
  }
}

static void
mmuswitchaddrspace(struct addrspace *s)
{
  struct pagel *pl;

  if (space == s) {
    return;
  } else {
    space = s;
  }
  
  mmuswitchl2(s->l2, PAGE_SIZE - sizeof(struct addrspace));
  
  for (pl = s->l2s; pl != nil; pl = pl->next) {
    mmuswitchl2((struct l2 *) pl->pa, PAGE_SIZE);
  }
}

void
mmuswitch(struct proc *p)
{
  mmuempty1();
  
  mmuswitchstack(&p->ustack);

  if (p->addrspace != nil) {
    mmuswitchaddrspace(p->addrspace);
  }
}

void
stackinit(struct stack *s)
{
  s->top = USTACK_TOP;
  s->bottom = USTACK_TOP;
  s->l2 = 0;
}

struct addrspace *
addrspacenew(reg_t page)
{
  struct addrspace *s;

  s = (struct addrspace *) page;
  s->refs = 1;
  s->l2s = nil;
  memset(s->l2, 0, PAGE_SIZE - sizeof(struct addrspace));

  return s;
}

reg_t
mappingfind(struct proc *p,
	    reg_t va,
	    int *flags)
{
  return ERR;
}

static struct l2 *
getl2new(struct l2 *l2, reg_t l1)
{
  l2->va = l1;
  l2->pa = 0;
	
  return l2;
}

static struct l2 *
getl2(struct addrspace *s, reg_t l1)
{
  struct pagel *pl, *prev;
  size_t i;

  for (i = 0; i < PAGE_SIZE - sizeof(struct addrspace);
       i += sizeof(struct l2)) {

    if (s->l2[i].va == 0) {
      return getl2new(&s->l2[i], l1);
    } else if (s->l2[i].va == l1) {
      return &s->l2[i];
    }
  }

  prev = nil;
  for (pl = s->l2s; pl != nil; prev = pl, pl = pl->next) {
    for (i = 0; i < PAGE_SIZE; i += sizeof(struct l2)) {
      if (s->l2[i].va == 0) {
	return getl2new(&s->l2[i], l1);
      } else if (s->l2[i].va == l1) {
	return &s->l2[i];
      }
    }
  }

  /* Get another page and use first */
  printf("need another page after 0x%h\n", prev);
  return nil;
}

int
mappingadd(struct addrspace *s,
	   reg_t va,
	   reg_t pa,
	   int flags)
{
  uint32_t tex, ap, c, b;
  struct l2 *l2;
  uint32_t *tab;

  l2 = getl2(s, L1X(va));
  if (l2 == nil) {
    printf("%i failed to find l2 for 0x%h\n", up->pid, va);
    return ERR;
  }
  
  tab = (uint32_t *) l2->pa;

  if (flags & PAGE_rw) {
    ap = AP_RW_RW;
  } else {
    ap = AP_RW_RO;
  }

  /* No caching for now */
  if (false) {
    tex = 7;
    c = 1;
    b = 0;
  } else {
    tex = 0;
    c = 0;
    b = 1;
  }

  tab[L2X(va)] = pa | L2_SMALL |
    (tex << 6) | (ap << 4) | (c << 3) | (b << 2);
  
  return OK;
}

int
mappingremove(struct addrspace *s,
	      reg_t va)
{
  return ERR;
}

