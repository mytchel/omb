/*
 *
 * Copyright (c) 2017 Mytchel Hammond <mytch@lackname.org>
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

#define AP_NO_NO	0
#define AP_RW_NO	1
#define AP_RW_RO	2
#define AP_RW_RW	3

#define L1X(va)          ((va) >> 20)
#define L2X(va)          (((va) >> 12) & ((1 << 8) - 1))
#define PAL1(entry)      ((entry) & 0xffffffc00)
#define PAL2(entry)      ((entry) & 0xffffff000)

#define L1_TYPE      0b11
#define L1_FAULT     0b00
#define L1_COARSE    0b01
#define L1_SECTION   0b10
#define L1_FINE      0b11

#define L2_TYPE      0b11
#define L2_FAULT     0b00
#define L2_LARGE     0b01
#define L2_SMALL     0b10
#define L2_TINY      0b11

struct addr {
	reg_t start;
	size_t len;
};

struct addr_holder {
	struct addr_holder *next;
	size_t len, n;
	struct addr addrs[];
};

void
imap(void *, void *, int, bool);

uint32_t
ttb[4096]__attribute__((__aligned__(16*1024))) = { L1_FAULT };

extern uint32_t *_ram_start;
extern uint32_t *_ram_end;

static struct addr_holder *ram = nil;

static space_t space = nil;

void
add_addr(struct addr_holder **a, reg_t start, reg_t end)
{
	struct addr_holder *h;

	if ((*a)->n == (*a)->len) {
		h = (struct addr_holder *) get_ram_page();
		if (h == nil) {
			return;
		}
		
		h->next = *a;
		*a = h;
			
		(*a)->n = 0;
		(*a)->len = (PAGE_SIZE - sizeof(struct addr_holder))
		                / sizeof(struct addr);
		                
		memset((*a)->addrs, 0, sizeof(struct addr) * (*a)->len);
	}
	
	(*a)->addrs[(*a)->n].start = start;
	(*a)->addrs[(*a)->n].len = end - start;
	(*a)->n++;
}

void
init_memory(void)
{
  int i;
  
  ram = (struct addr_holder *) &_ram_start;
	ram->next = nil;
			
	ram->n = 0;
	ram->len = (PAGE_SIZE - sizeof(struct addr_holder))
	                / sizeof(struct addr);
		
  add_addr(&ram, (uint32_t) &_ram_start + PAGE_SIZE, 
           (uint32_t) &_kernel_start);
  
  add_addr(&ram, (uint32_t) PAGE_ALIGN(&_kernel_end),
	         (uint32_t) &_ram_end);

  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  mmu_load_ttb(ttb);

  /* Give kernel unmapped access to all of ram. */	
  imap(&_ram_start, &_ram_end, AP_RW_NO, true);

  /* UART0, given to both kernel and possibly user. This may change */
  imap((void *) 0x44E09000, (void *) 0x44E0A000, AP_RW_NO, false);
  /* Watchdog */
  imap((void *) 0x44E35000, (void *) 0x44E36000, AP_RW_NO, false);
  /* DMTIMER2 for systick. */
  imap((void *) 0x48040000, (void *) 0x48041000, AP_RW_NO, false);
  /* INTCPS */
  imap((void *) 0x48200000, (void *) 0x48201000, AP_RW_NO, false);

  mmu_enable();
}

static void
mmu_empty(void)
{
	space_t s;
	int i;
	
	if (space == nil) {
		return;
	}
	
	for (s = space; s != nil; s = s->next) {
		for (i = 0; i < s->l2len; i++) {
			if (s->l2[i].tab != nil) {
				ttb[s->l2[i].va] = L1_FAULT;
			} else {
				break;
			}
		}
	}
	
	space = nil;
	mmu_invalidate();
}

static void
mmu_switch_l2(struct l2 *l2, size_t len)
{
	int i;
	
	for (i = 0; i < len; i++, l2++) {
		if (l2->tab != nil) {
			switch ((uint32_t) l2->tab & 1) {
			case 0:
				ttb[l2->va] = (uint32_t) l2->tab | L1_COARSE;
				break;
			case 1:
				ttb[l2->va] = ((uint32_t) l2->tab & (~1)) | L1_SECTION;
				break;
			}
		} else {
			break;
		}
	}
}

void
mmu_switch(space_t s)
{
	if (space == s) {
		return;
	} else {
		mmu_empty();
		space = s;
	}
	
	while (s != nil) {
		mmu_switch_l2(s->l2, s->l2len);
		s = s->next;
	}
}

space_t
space_new(reg_t page)
{
	space_t s = (space_t) page;
	
	s->next = nil;
	s->l2len = (PAGE_SIZE - sizeof(*s)) / sizeof(struct l2);
	
	memset(s->l2, 0, sizeof(struct l2) * s->l2len);
	
	return s;
}

static struct l2 *
l2_init(struct space *s, struct l2 *l2, reg_t l1)
{
	uint32_t *tab;
	
	tab = (uint32_t *) get_ram_page();
	if (tab == nil) {
		return nil;
	}
	
	l2->va = l1;
	l2->tab = tab;
	
	memset(l2->tab, 0, PAGE_SIZE);
	
	if (space == s) {
		ttb[l1] = (uint32_t) l2->tab | L1_COARSE;
	}
	
	return l2;
}

/* Has problem now that space can have sections and coarse. */

static struct l2 *
get_l2(space_t s, uint32_t l1, bool add)
{
	space_t ss;
	reg_t page;
	size_t i;
	
	for (ss = s; ss != nil; ss = ss->next) {
		for (i = 0; i < ss->l2len; i++) {
			if (ss->l2[i].tab == nil) {
				return add ? l2_init(ss, &ss->l2[i], l1) : nil;
			} else if (ss->l2[i].va == l1) {
				return &ss->l2[i];
			}
		}		
	}
	
	if (!add) {
		return nil;
	}
	
	page = (reg_t) get_ram_page();
	if (page == nil) {
		return nil;
	}
	
	ss = space_new(page);
	if (ss == nil) {
		return nil;
	}
	
	ss->next = s->next;
	s->next = ss;
	
	return l2_init(s, &ss->l2[0], l1);
}

bool
mapping_add(space_t s, reg_t pa, reg_t va, int flags)
{
	uint32_t tex, ap, c, b;
	struct l2 *l2;
	
	l2 = get_l2(s, L1X(va), true);
	if (l2 == nil) {
		return false;
	} else if ((uint32_t) l2->tab & 1) {
		/* Don't mess with coarse mappings. */
		return false;
	}
	
	/* Read writeable, no caching. */
	
	if (flags & ADDR_write) {
		ap = AP_RW_RW;
	} else {
		ap = AP_RW_RO;
	}
	
	if (flags & ADDR_cache) {
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
	
	return true;
}

reg_t
mapping_find(space_t s, reg_t va, int *flags)
{
	struct l2 *l2;
	reg_t v;
	
	l2 = get_l2(s, L1X(va), false);
	if (l2 == nil) {
		return nil;
	}
	
	if (l2->tab[L2X(va)] != L2_FAULT) {
		v = l2->tab[L2X(va)];
		
		*flags = ADDR_read | ADDR_exec;
		
		if (((va >> 4) & 3) == AP_RW_RW) {
			*flags |= ADDR_write;
		}
		
		if (((va >> 2) & 1) == 1) {
			*flags |= ADDR_cache;
		}
		
		return v & PAGE_MASK;
	} else {
		return nil;
	}
}

reg_t
mapping_remove(space_t s, reg_t va)
{
	struct l2 *l2;
	reg_t v;
	
	l2 = get_l2(s, L1X(va), false);
	if (l2 == nil) {
		return nil;
	}
	
	if (l2->tab[L2X(va)] != L2_FAULT) {
		v = l2->tab[L2X(va)];
		l2->tab[L2X(va)] = L2_FAULT;
		
		return v & PAGE_MASK;
	} else {
		return nil;
	}
}

reg_t
get_ram_page(void)
{
	struct addr_holder *a;
	size_t i;
	
	for (a = ram; a != nil; a = a->next) {
		for (i = 0; i < a->n; i++) {
			if (a->addrs[i].len > 0 &&
			    a->addrs[i].start >= (reg_t) &_ram_start &&
			    a->addrs[i].start + a->addrs[i].len <= (reg_t) &_ram_end) {
				
				a->addrs[i].len -= PAGE_SIZE;
				return (a->addrs[i].start + a->addrs[i].len);
			}
		}
	}
	
	return nil;
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
give_proc0_world(proc_t p)
{
	uint32_t pa, va, mask;
	space_t s;
	int i;
	
	va = PROC0_RAM_START;
	pa = (uint32_t) &_ram_start;

	mask = (AP_RW_RW << 10);
  mask |= (7 << 12) | (1 << 3) | (0 << 2);
  mask |= 1;
	
	s = p->space;
	for (i = 0; s->l2[i].tab != nil; i++)
		;
	
	while (pa < (uint32_t) &_ram_end) {
		if (i == s->l2len) {
			s->next = space_new(get_ram_page());
			s = s->next;
			i = 0;
		}
		
		s->l2[i].va = L1X(va);
		s->l2[i].tab = (uint32_t *) (pa | mask);
	 
		i++;
		pa += 1 << 20;
		va += 1 << 20;
	}
}