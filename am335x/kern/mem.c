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

extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static uint8_t kernel_page_page[PAGE_SIZE]__attribute__((__aligned__(PAGE_SIZE)))
    = { 0 };

kernel_page_t kernel_page = (kernel_page_t) kernel_page_page;

void
init_memory(void)
{
  int i;

  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  imap(&_ram_start, &_ram_end, AP_RW_RW, true);

	/* TODO: These should be small pages not sections. */
  imap((void *) 0x44E09000, (void *) 0x44E0A000, AP_RW_NO, false); /* UART0 */
  imap((void *) 0x44E35000, (void *) 0x44E36000, AP_RW_NO, false); /* Watchdog */
  imap((void *) 0x48040000, (void *) 0x48041000, AP_RW_NO, false); /* DMTIMER2 for systick. */
  imap((void *) 0x48200000, (void *) 0x48201000, AP_RW_NO, false); /* INTCPS */

	kernel_page->regions[kernel_page->nregions].type = REGION_ram;
	kernel_page->regions[kernel_page->nregions].start = (reg_t) &_ram_start;
	kernel_page->regions[kernel_page->nregions].len = (reg_t) &_kernel_start - (reg_t) &_ram_start;
	kernel_page->nregions++;
	
	kernel_page->regions[kernel_page->nregions].type = REGION_ram;
	kernel_page->regions[kernel_page->nregions].start = (reg_t) &_kernel_end;
	kernel_page->regions[kernel_page->nregions].len = (reg_t) &_ram_end - (reg_t) &_kernel_end;
	kernel_page->nregions++;
	
	imap((void *) 0x47400000, (void *) 0x47404000, AP_RW_RW, false); /* USB */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x47400000;
	kernel_page->regions[kernel_page->nregions].len = 0x47404000 - 0x47400000;
	kernel_page->nregions++;
	    
  imap((void *) 0x44E31000, (void *) 0x44E32000, AP_RW_RW, false); /* DMTimer1 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x44E31000;
	kernel_page->regions[kernel_page->nregions].len = 0x44E32000 - 0x44E31000;
	kernel_page->nregions++;
	
  imap((void *) 0x44E09000, (void *) 0x44E0A000, AP_RW_RW, false); /* UART0 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x44E09000;
	kernel_page->regions[kernel_page->nregions].len = 0x44E0A000 - 0x44E09000;
	kernel_page->nregions++;
	
  imap((void *) 0x44E07000, (void *) 0x44E08000, AP_RW_RW, false); /* GPIO0 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x44E07000;
	kernel_page->regions[kernel_page->nregions].len = 0x44E08000 - 0x44E07000;
	kernel_page->nregions++;
	
  imap((void *) 0x4804c000, (void *) 0x4804d000, AP_RW_RW, false); /* GPIO1 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x4804c000;
	kernel_page->regions[kernel_page->nregions].len = 0x4804d000 - 0x4804c000;
	kernel_page->nregions++;
	
  imap((void *) 0x481ac000, (void *) 0x481ad000, AP_RW_RW, false); /* GPIO2 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x481ac000;
	kernel_page->regions[kernel_page->nregions].len = 0x481ad000 - 0x481ac000;
	kernel_page->nregions++;
	
  imap((void *) 0x481AE000, (void *) 0x481AF000, AP_RW_RW, false); /* GPIO3 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x481AE000;
	kernel_page->regions[kernel_page->nregions].len = 0x481AF000 - 0x481AE000;
	kernel_page->nregions++;
	
  imap((void *) 0x48060000, (void *) 0x48061000, AP_RW_RW, false); /* MMCHS0 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x481AE000;
	kernel_page->regions[kernel_page->nregions].len = 0x481AF000 - 0x481AE000;
	kernel_page->nregions++;
	
  imap((void *) 0x47810000, (void *) 0x47820000, AP_RW_RW, false); /* MMCHS2 */
	kernel_page->regions[kernel_page->nregions].type = REGION_io;
	kernel_page->regions[kernel_page->nregions].start = 0x47810000;
	kernel_page->regions[kernel_page->nregions].len = 0x47820000 - 0x47810000;
	kernel_page->nregions++;
	
  mmu_load_ttb(ttb);
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
l2_map(uint32_t *tab, uint32_t pa, uint32_t va,
       uint32_t ap, uint32_t cachable)
{
	uint32_t tex, b;
	
	if (cachable) {
		tex = 7;
		b = 0;
	} else {
		tex = 0;
		b = 1;
	}
	
	tab[L2X(va)] = pa | L2_SMALL | 
	    (tex << 6) | (ap << 4) | (cachable << 3) | (b << 2);
}

bool
space_map(addr_space_t space, 
          reg_t pa, reg_t va,
          int flags,
          reg_t (*get_page)(void *arg),
          void *arg)
{
	uint32_t *tab, ap, c;
	
	if (space->tab[L1X(va)] == L1_FAULT) {
		tab = (uint32_t *) get_page(arg);
		if (tab == nil) {
			return false;
		}
		
		memset(tab, 0, PAGE_SIZE);
		
		space->tab[L1X(va)] = (uint32_t) tab | L1_COARSE;
	} else {
		tab = (uint32_t *) (space->tab[L1X(va)] & (~L1_TYPE));
	}
	
	if (flags & ADDR_cache) {
		c = 1;
	} else {
		c = 0;
	}
	
	if (flags & ADDR_write) {
		ap = AP_RW_RW;
	} else {
		ap = AP_RW_RO;
	}
	
	l2_map(tab, pa, va, ap, c);
	
	return true;
}

int
flags_from_map(reg_t v)
{
	int f = ADDR_read;
	
	if (((v >> 4) & 0b11) == AP_RW_RW) {
		f |= ADDR_write;
	}
	
	if (((v >> 2) & 0b1) == 0b1) {
		f |= ADDR_cache;
	}
	
	return f;
}

reg_t
space_find(addr_space_t space,
           reg_t va, int *flags)
{
	uint32_t *tab, pa;
	
	tab = (uint32_t *) (space->tab[L1X(va)] & (~L1_TYPE));
	if (tab == nil) {
		return nil;
	}
	
	pa = tab[L2X(va)];
	*flags = flags_from_map(pa);
	return pa & PAGE_MASK;	
}

void
space_unmap(addr_space_t space,
            reg_t va)
{
	uint32_t *tab;
	
	tab = (uint32_t *) (space->tab[L1X(va)] & (~L1_TYPE));
	if (tab != nil) {
		tab[L2X(va)] = L2_FAULT;
	}
}
