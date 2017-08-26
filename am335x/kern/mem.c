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

void
imap(void *, void *, int, bool);

uint32_t
ttb[4096]__attribute__((__aligned__(16*1024))) = { L1_FAULT };

extern uint32_t *_ram_start;
extern uint32_t *_ram_end;

static space_t space = nil;

void
init_memory(void)
{
  int i;

  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  mmu_load_ttb(ttb);

  /* Give kernel unmapped access to all of ram. */	
  imap(&_ram_start, &_ram_end, AP_RW_NO, true);

	/* TODO: These should be small pages not sections. */
  imap((void *) 0x44E09000, (void *) 0x44E0A000, AP_RW_NO, false); /* UART0 */
  imap((void *) 0x44E35000, (void *) 0x44E36000, AP_RW_NO, false); /* Watchdog */
  imap((void *) 0x48040000, (void *) 0x48041000, AP_RW_NO, false); /* DMTIMER2 for systick. */
  imap((void *) 0x48200000, (void *) 0x48201000, AP_RW_NO, false); /* INTCPS */

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
	
	if ((reg_t) &_kernel_start <= va && va <= (reg_t) &_kernel_end) {
		return false;
	}
	
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
give_proc_section(proc_t p, reg_t start, reg_t end, int flags)
{
	size_t len;
	int s_id;
	
	len = end - start;
	
	s_id = section_new(p, len, flags);
	if (s_id < 0) {
		panic("Failed to create new section!\n");
	}
	
	if (!page_list_add(p->page_list, s_id,
	                   start, start,
	                   0, len)) {
	                   
		panic("Failed to add 0x%h %i to page list for proc %i!\n",
		      start, len, p->pid);
	}
}

void
give_proc0_world(proc_t p0)
{
	give_proc_section(p0, (reg_t) &_ram_start, (reg_t) &_kernel_start,
	                  ADDR_read|ADDR_write|ADDR_cache);
	                  
	give_proc_section(p0, (reg_t) &_kernel_end, (reg_t) &_ram_end,
	                  ADDR_read|ADDR_write|ADDR_cache);
	                  	
  give_proc_section(p0, 0x47400000, 0x47404000, ADDR_read|ADDR_write); /* USB */
  give_proc_section(p0, 0x44E31000, 0x44E32000, ADDR_read|ADDR_write); /* DMTimer1 */
  give_proc_section(p0, 0x48042000, 0x48043000, ADDR_read|ADDR_write); /* DMTIMER3 */
  give_proc_section(p0, 0x44E09000, 0x44E0A000, ADDR_read|ADDR_write); /* UART0 */
  give_proc_section(p0, 0x48022000, 0x48023000, ADDR_read|ADDR_write); /* UART1 */
  give_proc_section(p0, 0x48024000, 0x48025000, ADDR_read|ADDR_write); /* UART2 */
  give_proc_section(p0, 0x44E07000, 0x44E08000, ADDR_read|ADDR_write); /* GPIO0 */
  give_proc_section(p0, 0x4804c000, 0x4804d000, ADDR_read|ADDR_write); /* GPIO1 */
  give_proc_section(p0, 0x481ac000, 0x481ad000, ADDR_read|ADDR_write); /* GPIO2 */
  give_proc_section(p0, 0x481AE000, 0x481AF000, ADDR_read|ADDR_write); /* GPIO3 */
  give_proc_section(p0, 0x48060000, 0x48061000, ADDR_read|ADDR_write); /* MMCHS0 */
  give_proc_section(p0, 0x481D8000, 0x481D9000, ADDR_read|ADDR_write); /* MMC1 */
  give_proc_section(p0, 0x47810000, 0x47820000, ADDR_read|ADDR_write); /* MMCHS2 */
  give_proc_section(p0, 0x44E35000, 0x44E36000, ADDR_read|ADDR_write); /* Watchdog */
  give_proc_section(p0, 0x44E05000, 0x44E06000, ADDR_read|ADDR_write); /* DMTimer0 */
}