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
#define PAL1(entry)     (entry & 0xffffffc00)
#define PAL2(entry)     (entry & 0xffffff000)

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

static int
mappingaddl2(uint32_t *l2, reg_t va, reg_t pa, int flags);

uint32_t
ttb[4096]__attribute__((__aligned__(16*1024))) = { L1_FAULT };

static struct addrspace *space = nil;
static struct ustack *ustack = nil;
static uint32_t spacelow = UINT_MAX, ustacklow = UINT_MAX;
static uint32_t spacehigh = 0, ustackhigh = 0;

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
mmuempty1ustack(void)
{
  uint32_t i;

  for (i = ustacklow; i <= ustackhigh; i++) {
    if ((ttb[i] & L1_TYPE) == L1_COARSE) {
      ttb[i] = L1_FAULT;
    }
  }

  mmuinvalidate();

  ustacklow = UINT_MAX;
  ustackhigh = 0;
}

static void
mmuinsertl2space(reg_t va, uint32_t *l2)
{
  reg_t l1;

  l1 = L1X(va);

  ttb[l1] = (uint32_t) l2 | L1_COARSE;
  if (l1 > spacehigh) {
    spacehigh = l1;
  } else if (l1 < spacelow) {
    spacelow = l1;
  }
}

static void
mmuinsertl2ustack(reg_t va, uint32_t *l2)
{
  reg_t l1;

  l1 = L1X(va);

  ttb[l1] = (uint32_t) l2 | L1_COARSE;
  if (l1 > ustackhigh) {
    ustackhigh = l1;
  } else if (l1 < ustacklow) {
    ustacklow = l1;
  }
}

static void
mmuswitchustack(struct ustack *s)
{
  if (s == ustack) {
    return;
  } else {
    mmuempty1ustack();
    ustack = s;
  }
  
  if (s->tab != 0) {
    mmuinsertl2ustack(s->bottom, s->tab);
 }
}

static void
mmuswitchl2(struct l2 l2s[], size_t len)
{
  struct l2 *l2;
  size_t i;

  for (i = 0; i < len; i++) {
    l2 = &l2s[i];
    if (l2->tab != nil) {
      mmuinsertl2space(l2->va, l2->tab);
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
  mmuswitchustack(&p->ustack);
  mmuswitchaddrspace(p->addrspace);
}

void
ustackinit(struct ustack *s)
{
  s->top = USTACK_TOP;
  s->bottom = USTACK_TOP;
  s->tab = nil;
}

void
ustackfree(struct ustack *s)
{
  reg_t pa;

  if (s->tab == nil) {
    return;
  }
  
  while (s->bottom < s->top) {
    pa = L2X(s->bottom);
    heapadd((void *) (s->tab[pa] & PAGE_MASK));
    s->bottom += PAGE_SIZE;
  }

  heapadd((void *) s->tab);
  s->tab = nil;
}

int
ustackcopy(struct ustack *n, struct ustack *o)
{
  reg_t pn, po;

  ustackinit(n);

  if (o->tab == nil) {
    return OK;
  }

  n->tab = (uint32_t *) heappop();
  if (n->tab == nil) {
    return ENOMEM;
  }
  
  memset((void *) n->tab, 0, PAGE_SIZE);

  for (n->bottom = o->top - PAGE_SIZE;
       n->bottom >= o->bottom;
       n->bottom -= PAGE_SIZE) {

    pn = (reg_t) heappop();
    if (pn == nil) {
      ustackfree(n);
      return ENOMEM;
    }

    if (mappingaddl2(n->tab, n->bottom, pn, MEM_r|MEM_w) != OK) {
      heapadd((void *) pn);
      ustackfree(n);
      return ERR;
    }

    po = PAL2(o->tab[L2X(o->bottom)]);
    memmove((void *) pn, (void *) po, PAGE_SIZE);
  }

  return OK;
}

struct addrspace *
addrspacenew(reg_t start)
{
  struct addrspace *s;

  s = (struct addrspace *) start;

  s->l2len = (PAGE_SIZE - sizeof(struct addrspace) / sizeof(struct l2));
  memset(s->l2, 0, s->l2len * sizeof(struct l2));

  return s;
}

struct addrspace *
addrspacecopy(struct addrspace *o)
{
  return nil;
}

void
addrspacefree(struct addrspace *s)
{
  int i;

  for (i = 0; i < s->l2len; i++) {
    if (s->l2[i].va == nil) {
      break;
    } else {
      heapadd(s->l2[i].tab);
    }
  }
}

static struct l2 *
getl2new(struct addrspace *s, struct l2 *l2, reg_t l1)
{
  l2->va = l1;

  l2->tab = (uint32_t *) heappop();
  if (l2->tab == nil) {
    return nil;
  }
 
  memset(l2->tab, 0, PAGE_SIZE);

  if (space == s) {
    mmuinsertl2space(l2->va, l2->tab);
  }
  
  return l2;
}

static struct l2 *
getl2(struct addrspace *s, reg_t l1, bool add)
{
  size_t i;

  for (i = 0; i < s->l2len; i++) {
    if (s->l2[i].tab == nil) {
      if (add) {
	return getl2new(s, &s->l2[i], l1);
      } else {
	return nil;
      }
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

  if (flags & MEM_w) {
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

  l2 = getl2(s, L1X(va), true);
  if (l2 == nil) {
    return ERR;
  }
  
  return mappingaddl2(l2->tab, va, pa, flags);
}

int
mappingremove(struct addrspace *s,
	      reg_t va)
{
  struct l2 *l2;

  l2 = getl2(s, L1X(va), false);
  if (l2 == nil) {
    return ERR;
  }
  
  if (l2->tab[L2X(va)] != L2_FAULT) {
    l2->tab[L2X(va)] = L2_FAULT;
    return OK;
  } else {
    return ERR;
  }
}

static reg_t
pagefind(uint32_t *tab, reg_t va, int *flags)
{
  reg_t entry;
  reg_t ap;
  int f;

  entry = tab[L2X(va)];

  ap = entry & (3 << 4);

  f = MEM_r;
  if (ap != AP_RW_RW) {
    f |= MEM_w;
  }
  
  *flags = f;
  return entry & PAGE_MASK;

}

reg_t
mappingfind(struct addrspace *s,
	    reg_t va,
	    int *flags)
{
  struct l2 *l2;

  l2 = getl2(s, L1X(va), false);
  if (l2 != nil) {
    return nil;
  }

  return pagefind(l2->tab, va, flags);
}

int
fixustack(struct ustack *s, reg_t addr)
{
  reg_t new, bottom;
  
  bottom = s->bottom - PAGE_SIZE;

  if (s->tab == nil) {
    s->tab = (uint32_t *) heappop();
    if (s->tab == nil) {
      return ERR;
    }

    mmuinsertl2ustack(bottom, s->tab);
  }

  new = (reg_t) heappop();
  if (new == nil) {
    return ERR;
  }

  s->bottom = bottom;
  
  return mappingaddl2(s->tab, s->bottom, new, MEM_r|MEM_w);
}

int
fixfault(reg_t addr)
{
  struct ustack *s;
  
  s = &up->ustack;
  if (addr < s->bottom && addr + PAGE_SIZE > s->bottom) {
    return fixustack(s, addr);
  }

  return ERR;
}

int
validaddr(void *addr, size_t len, int flags)
{
  reg_t off, va, l;
  int f;

  va = ((reg_t) addr) & PAGE_MASK;

  off = ((reg_t) addr) - va;
  
  for (l = 0; l < len + off; l += PAGE_SIZE) {
    if (va + l >= up->ustack.bottom && va + l <= up->ustack.top) {
      if (pagefind(up->ustack.tab, va + l, &f) == nil) {
	return ERR;
      }
    } else if (mappingfind(up->addrspace, va + l, &f) == nil) {
      return ERR;
    }

    if (checkflags(flags, f) != OK) {
      return ERR;
    }
  }
  
  return OK;
}
