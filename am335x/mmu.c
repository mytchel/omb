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

static struct addrspace *space = nil;
static struct stack *stack = nil;
static uint32_t spacelow = UINT_MAX, stacklow = UINT_MAX;
static uint32_t spacehigh = 0, stackhigh = 0;

void
mmuinit(void)
{
  int i;

  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  mmuloadttb(ttb);
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
    ttb[L1X(x)] = x | mask;
    x += 1 << 20;
  }
}

void
mmuempty1space(void)
{
  uint32_t i;

  for (i = spacelow; i <= spacehigh; i++) {
    if ((ttb[i] & L1_TYPE) == L1_COARSE) {
      ttb[i] = L1_FAULT;
    }
  }

  mmuinvalidate();

  spacelow = UINT_MAX;
  spacehigh = 0;
}

void
mmuempty1stack(void)
{
  uint32_t i;

  for (i = stacklow; i <= stackhigh; i++) {
    if ((ttb[i] & L1_TYPE) == L1_COARSE) {
      ttb[i] = L1_FAULT;
    }
  }

  mmuinvalidate();

  stacklow = UINT_MAX;
  stackhigh = 0;
}

static void
mmuinsertl2space(reg_t va, reg_t pa)
{
  reg_t l1;

  l1 = L1X(va);

  ttb[l1] = pa | L1_COARSE;
  if (l1 > spacehigh) {
    spacehigh = l1;
  } else if (l1 < spacelow) {
    spacelow = l1;
  }
}

static void
mmuinsertl2stack(reg_t va, reg_t pa)
{
  reg_t l1;

  l1 = L1X(va);

  ttb[l1] = pa | L1_COARSE;
  if (l1 > stackhigh) {
    stackhigh = l1;
  } else if (l1 < stacklow) {
    stacklow = l1;
  }
}

static void
mmuswitchstack(struct stack *s)
{
  if (s == stack) {
    return;
  } else {
    mmuempty1stack();
    stack = s;
  }
  
  if (s->l2 != 0) {
    mmuinsertl2stack(s->bottom, s->l2);
 }
}

static void
mmuswitchl2(struct l2 l2s[], size_t len)
{
  struct l2 *l2;
  size_t i;

  for (i = 0; i < len; i++) {
    l2 = &l2s[i];
    if (l2->pa != 0) {
      mmuinsertl2space(l2->va, l2->pa);
    } else {
      break;
    }
  }
}

static void
mmuswitchaddrspace(struct addrspace *s)
{
  if (space == s) {
    return;
  } else {
    mmuempty1space();
    space = s;
  }

  if (s != nil) {
    mmuswitchl2(s->l2, s->l2len);
  }
}

void
mmuswitch(struct proc *p)
{
  mmuswitchstack(&p->ustack);
  mmuswitchaddrspace(p->addrspace);
}

void
stackinit(struct stack *s)
{
  s->top = USTACK_TOP;
  s->bottom = USTACK_TOP;
  s->l2 = nil;
}

struct addrspace *
addrspacenew(reg_t page)
{
  struct addrspace *s;

  s = (struct addrspace *) page;
  s->refs = 1;
  s->l2len = (PAGE_SIZE - sizeof(struct addrspace) / sizeof(struct l2));

  memset(s->l2, 0, s->l2len * sizeof(struct l2));

  return s;
}

static struct l2 *
getl2new(struct addrspace *s, struct l2 *l2, reg_t l1)
{
  l2->va = l1;
  l2->pa = kgetpage();

  if (l2->pa == nil) {
    return nil;
  }
 
  memset((void *) l2->pa, 0, PAGE_SIZE);

  if (up->addrspace == s) {
    mmuinsertl2space(l2->va, l2->pa);
  }
  
  return l2;
}

static struct l2 *
getl2(struct addrspace *s, reg_t l1)
{
  size_t i;

  for (i = 0; i < s->l2len; i++) {
    if (s->l2[i].pa == 0) {
      return getl2new(s, &s->l2[i], l1);
    } else if (s->l2[i].va == l1) {
      return &s->l2[i];
    }
  }

  return nil;
}

static int
mappingaddl2(uint32_t *l2, reg_t va, reg_t pa, int flags)
{
  uint32_t tex, ap, c, b;

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

  l2[L2X(va)] = pa | L2_SMALL |
    (tex << 6) | (ap << 4) | (c << 3) | (b << 2);

  return OK;
}

int
mappingadd(struct addrspace *s,
	   reg_t va,
	   reg_t pa,
	   int flags)
{
  struct l2 *l2;
  uint32_t *tab;

  l2 = getl2(s, L1X(va));
  if (l2 == nil) {
    return ERR;
  }
  
  tab = (uint32_t *) l2->pa;

  return mappingaddl2(tab, va, pa, flags);
}

int
mappingremove(struct addrspace *s,
	      reg_t va)
{
  return ERR;
}

reg_t
mappingfind(struct proc *p,
	    reg_t va,
	    int *flags)
{
  return ERR;
}

int
fixstack(struct stack *stack, reg_t addr)
{
  uint32_t *tab;
  reg_t new, bottom;
  
  bottom = stack->bottom - PAGE_SIZE;

  if (stack->l2 == nil) {
    stack->l2 = kgetpage();
    if (stack->l2 == nil) {
      return ERR;
    }

    mmuinsertl2stack(bottom, stack->l2);
  }

  tab = (uint32_t *) stack->l2;
  
  new = kgetpage();
  if (new == nil) {
    return ERR;
  }

  stack->bottom = bottom;
  
  return mappingaddl2(tab, stack->bottom, new, PAGE_rw);
}

int
fixfault(reg_t addr)
{
  struct stack *stack;
  
  stack = &up->ustack;
  if (addr < stack->bottom && addr + PAGE_SIZE > stack->bottom) {
    return fixstack(stack, addr);
  }

  return ERR;
}

