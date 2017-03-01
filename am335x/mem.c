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

static void
addrampages(uint32_t, uint32_t);

static void
addiopages(uint32_t, uint32_t);

extern uint32_t *ram_start;
extern uint32_t *ram_end;
extern uint32_t *kernel_start;
extern uint32_t *kernel_end;

struct pageholder *rampages = nil;
struct pageholder *iopages = nil;

void
memoryinit(void)
{
  addrampages(PAGE_ALIGN((uint32_t) &kernel_end + PAGE_SIZE - 1),
	      (uint32_t) &ram_end);
  
  addrampages((uint32_t) &ram_start,
	      PAGE_ALIGN((uint32_t) &kernel_start));

  addiopages(0x47400000, 0x47404000); /* USB */
  addiopages(0x44E31000, 0x44E32000); /* DMTimer1 */
  addiopages(0x48042000, 0x48043000); /* DMTIMER3 */
  addiopages(0x44E09000, 0x44E0A000); /* UART0 */
  addiopages(0x48022000, 0x48023000); /* UART1 */
  addiopages(0x48024000, 0x48025000); /* UART2 */
  addiopages(0x44E07000, 0x44E08000); /* GPIO0 */
  addiopages(0x4804c000, 0x4804d000); /* GPIO1 */
  addiopages(0x481ac000, 0x481ad000); /* GPIO2 */
  addiopages(0x481AE000, 0x481AF000); /* GPIO3 */
  addiopages(0x48060000, 0x48061000); /* MMCHS0 */
  addiopages(0x481D8000, 0x481D9000); /* MMC1 */
  addiopages(0x47810000, 0x47820000); /* MMCHS2 */
  addiopages(0x44E35000, 0x44E36000); /* Watchdog */
  addiopages(0x44E05000, 0x44E06000); /* DMTimer0 */
  addiopages(0x48040000, 0x48041000); /* DMTIMER2 */

  mmuinit();

  /* Give kernel unmapped access to all of ram. */	
  imap(&ram_start, &ram_end, AP_RW_NO, true);

  /* UART0, given to both kernel and possibly users. This may change */
  imap((void *) 0x44E09000, (void *) 0x44E0A000, AP_RW_NO, false);
  /* Watchdog */
  imap((void *) 0x44E35000, (void *) 0x44E36000, AP_RW_NO, false);
  /* DMTIMER2 */
  imap((void *) 0x48040000, (void *) 0x48041000, AP_RW_NO, false);
  /* INTCPS */
  imap((void *) 0x48200000, (void *) 0x48201000, AP_RW_NO, false);

  mmuenable();
}

static void
initpages(struct pageholder *p, struct pageholder **from, size_t npages,
	  uint32_t start, page_t type)
{
  size_t i;

  for (i = 0; i < npages; i++) {
    p[i].pa = start;
    p[i].type = type;
    p[i].next = &p[i+1];

    start += PAGE_SIZE;
  }

  p[i-1].next = *from;
  *from = &p[0];
}

void
addrampages(uint32_t start, uint32_t end)
{
  struct pageholder *new;
  uint32_t npages, size;

  size = end - start;
  npages = 0;

  new = (struct pageholder *) start;

  while (size > (PAGE_ALIGN(sizeof(struct pageholder) * npages)
		 + PAGE_SIZE * npages)) {
    npages++;
  }

  npages--;

  start = PAGE_ALIGN(start + sizeof(struct pageholder) * npages +
		     PAGE_SIZE - 1);

  initpages(new, &rampages, npages, start, PAGE_ram);
}

void
addiopages(uint32_t start, uint32_t end)
{
  static reg_t holder = 0;
  static size_t used = 0;

  struct pageholder *p;
  size_t npages, cpages;

  npages = (end - start) / PAGE_SIZE;

  while (npages > 0) {
    if (holder == 0 || (used + sizeof(struct pageholder)) > PAGE_SIZE) {
      holder = getrampage();
      if (holder != 0) {
	used = 0;
      } else {
	puts("kernel failed to get a page!\n");
	while (true)
	  ;
      }
    }

    if (npages * sizeof(struct pageholder) > PAGE_SIZE - used) {
      cpages = (PAGE_SIZE - used) / PAGE_SIZE;
    } else {
      cpages = npages;
    }

    p = (struct pageholder *) (holder + used);

    initpages(p, &iopages, cpages, start, PAGE_io);

    used += cpages * sizeof(struct pageholder);
    npages -= cpages;
  }
}

reg_t
getrampage(void)
{
  struct pageholder *p;

  do {
    p = rampages;
  } while (!cas(&rampages, p, p->next));

  return p->pa;
}
