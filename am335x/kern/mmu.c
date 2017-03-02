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

uint32_t
ttb[4096]__attribute__((__aligned__(16*1024))) = { L1_FAULT };

static struct space *space = nil;

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
mmuempty1(void)
{
  uint32_t i;

  if (space == nil) {
    return;
  }
  
  for (i = 0; i < space->l2len; i++) {
    if (space->l2[i].tab != nil) {
      ttb[space->l2[i].va] = L1_FAULT;
    } else {
      break;
    }
  }

  mmuinvalidate();
}

static void
mmuswitchl2(struct l2 l2s[], size_t len)
{
  struct l2 *l2;
  size_t i;

  for (i = 0; i < len; i++) {
    l2 = &l2s[i];
    if (l2->tab != nil) {
      ttb[l2->va] = (uint32_t) l2->tab | L1_COARSE;
    } else {
      break;
    }
  }
}

void
mmuswitch(struct space *s)
{
  if (space == s) {
    return;
  } else {
    mmuempty1();
    space = s;
  }

  if (s != nil) {
    mmuswitchl2(s->l2, s->l2len);
  }
}

struct space *
spacenew(reg_t start)
{
  struct space *s;

  s = (struct space *) start;

  s->l2len = (PAGE_SIZE - sizeof(struct space) / sizeof(struct l2));
  memset(s->l2, 0, s->l2len * sizeof(struct l2));

  return s;
}

struct space *
spacecopy(struct space *o)
{
  return nil;
}

void
spacefree(struct space *s)
{
  int i;

  for (i = 0; i < s->l2len; i++) {
    if (s->l2[i].tab != nil) {
      heapadd(s->l2[i].tab);
    } else {
      break;
    }
  }
}

static struct l2 *
getl2new(struct space *s, struct l2 *l2, reg_t l1)
{
  l2->va = l1;
  l2->tab = (uint32_t *) heappop();
  if (l2->tab == nil) {
    return nil;
  }

  memset(l2->tab, 0, PAGE_SIZE);

  if (space == s) {
    ttb[l1] = (uint32_t) l2->tab | L1_COARSE;
  }
  
  return l2;
}

static struct l2 *
getl2(struct space *s, reg_t l1, bool add)
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

int
mappingadd(struct space *s,
	   reg_t va,
	   reg_t pa,
	   int flags)
{
  uint32_t tex, ap, c, b;
  struct l2 *l2;

  l2 = getl2(s, L1X(va), true);
  if (l2 == nil) {
    return ERR;
  }

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

  l2->tab[L2X(va)] = pa | L2_SMALL |
    (tex << 6) | (ap << 4) | (c << 3) | (b << 2);

  return OK;
}

int
mappingremove(struct space *s,
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

reg_t
mappingfind(struct space *s,
	    reg_t va,
	    int *flags)
{
  struct l2 *l2;
  reg_t entry;
  reg_t ap;
  int f;

  l2 = getl2(s, L1X(va), false);
  if (l2 == nil) {
    return nil;
  }

  entry = l2->tab[L2X(va)];

  ap = entry & (3 << 4);

  f = MEM_r;
  if (ap != AP_RW_RW) {
    f |= MEM_w;
  }
  
  *flags = f;
  return entry & PAGE_MASK;
}

int
validaddr(void *addr, size_t len, int flags)
{
  reg_t off, va, l;
  int f;

  va = ((reg_t) addr) & PAGE_MASK;

  off = ((reg_t) addr) - va;
  
  for (l = 0; l < len + off; l += PAGE_SIZE) {
    if (mappingfind(up->space, va + l, &f) == nil) {
      return ERR;
    }

    if (checkflags(flags, f) != OK) {
      return ERR;
    }
  }
  
  return OK;
}
