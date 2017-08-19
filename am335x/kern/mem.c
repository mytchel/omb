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

typedef enum {
	ADDR_ram,
	ADDR_io,
} addr_t;

struct addr {
	reg_t start;
	size_t len;
	addr_t type;
};

struct addr_holder {
	struct addr_holder *next;
	size_t len, n;
	struct addr addrs[];
};

uint32_t
ttb[4096]__attribute__((__aligned__(16*1024))) = { L1_FAULT };

extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static struct addr_holder *addrs;

static space_t space = nil;

void *
get_ram_page(void)
{
	struct addr_holder *a;
	size_t i;
	
	for (a = addrs; a != nil; a = a->next) {
		for (i = 0; i < a->n; i++) {
			if (a->addrs[i].len > 0 && a->addrs[i].type == ADDR_ram) {
				a->addrs[i].len -= PAGE_SIZE;
				return (void *) (a->addrs[i].start + a->addrs[i].len);
			}
		}
	}
	
	return nil;
}

static void
add_addr(reg_t start, reg_t end, addr_t type)
{
	struct addr_holder *h;

	if (addrs->n == addrs->len) {
		h = (struct addr_holder *) get_ram_page();
		h->next = addrs;
		addrs = h;
			
		addrs->n = 0;
		addrs->len = (PAGE_SIZE - sizeof(struct addr_holder))
		                / sizeof(struct addr);
		                
		memset(addrs->addrs, 0, sizeof(struct addr) * addrs->len);
	}
	
	addrs->addrs[addrs->n].start = start;
	addrs->addrs[addrs->n].len = end - start;
	addrs->addrs[addrs->n].type = type;
	addrs->n++;
}

void *
get_io_page(reg_t addr)
{
	struct addr_holder *a;
	size_t i;
	
	for (a = addrs; a != nil; a = a->next) {
		for (i = 0; i < a->n; i++) {
			if (a->addrs[i].type == ADDR_io &&
			    a->addrs[i].start <= addr &&
			    a->addrs[i].start + a->addrs[i].len > addr) {
			  
			  if (a->addrs[i].start + a->addrs[i].len > addr + PAGE_SIZE) {
			  	/* Add another spot for remainder of chunk. */
			  	add_addr(addr + PAGE_SIZE, 
			  	         a->addrs[i].start + a->addrs[i].len,
			  	         ADDR_io);
			  }
			  
			  /* Shrink chunk. */
			  a->addrs[i].len = addr - a->addrs[i].start;
			  				
				return (void *) addr;
			}
		}
	}
	
	return nil;
}

void
init_memory(void)
{
  int i;
  
  addrs = (struct addr_holder *)
      PAGE_ALIGN((uint32_t) &_kernel_end + PAGE_SIZE - 1);
	addrs->next = nil;
			
	addrs->n = 0;
	addrs->len = (PAGE_SIZE - sizeof(struct addr_holder))
	                / sizeof(struct addr);
		
  add_addr((reg_t) addrs + PAGE_SIZE, 
           (uint32_t) &_ram_end,
           ADDR_ram);
  
  add_addr((uint32_t) &_ram_start,
	         PAGE_ALIGN((uint32_t) &_kernel_start),
	         ADDR_ram);

  add_addr(0x47400000, 0x47404000, ADDR_io); /* USB */
  add_addr(0x44E31000, 0x44E32000, ADDR_io); /* DMTimer1 */
  add_addr(0x48042000, 0x48043000, ADDR_io); /* DMTIMER3 */
  add_addr(0x44E09000, 0x44E0A000, ADDR_io); /* UART0 */
  add_addr(0x48022000, 0x48023000, ADDR_io); /* UART1 */
  add_addr(0x48024000, 0x48025000, ADDR_io); /* UART2 */
  add_addr(0x44E07000, 0x44E08000, ADDR_io); /* GPIO0 */
  add_addr(0x4804c000, 0x4804d000, ADDR_io); /* GPIO1 */
  add_addr(0x481ac000, 0x481ad000, ADDR_io); /* GPIO2 */
  add_addr(0x481AE000, 0x481AF000, ADDR_io); /* GPIO3 */
  add_addr(0x48060000, 0x48061000, ADDR_io); /* MMCHS0 */
  add_addr(0x481D8000, 0x481D9000, ADDR_io); /* MMC1 */
  add_addr(0x47810000, 0x47820000, ADDR_io); /* MMCHS2 */
  add_addr(0x44E35000, 0x44E36000, ADDR_io); /* Watchdog */
  add_addr(0x44E05000, 0x44E06000, ADDR_io); /* DMTimer0 */

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
			ttb[l2->va] = (uint32_t) l2->tab | L1_COARSE;
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
	
	tab = get_ram_page();
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
mapping_add(space_t s, reg_t pa, reg_t va)
{
	uint32_t tex, ap, c, b;
	struct l2 *l2;
	
	l2 = get_l2(s, L1X(va), true);
	if (l2 == nil) {
		return false;
	}
	
	/* Read writeable, no caching. */
	
	ap = AP_RW_RW;
	tex = 0;
	c = 0;
	b = 1;
	
	l2->tab[L2X(va)] = pa | L2_SMALL |
	    (tex << 6) | (ap << 4) | (c << 3) | (b << 2);
	
	return true;
}

void *
kernel_addr(space_t s, reg_t addr, size_t len)
{
	reg_t va, pa, off;
	struct l2 *l2;
	size_t l;
	
	va = addr & PAGE_MASK;
	off = addr - va;
	
	pa = 0;
	for (l = 0; l < off + len; l += PAGE_SIZE) {
		l2 = get_l2(s, L1X(va + l), true);
		if (l2 == nil) {
			return nil;
		}
	
		if (l2->tab[L2X(va)] == L2_FAULT) {
			return nil;
			
		} else if (l == 0) {
			pa = (l2->tab[L2X(va)] & PAGE_MASK)
			    + off;
		}		
	}
	
	return (void *) pa;
}

