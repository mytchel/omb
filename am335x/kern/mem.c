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

#define L1X(va)          (va >> 20)
#define L2X(va)          ((va >> 12) & ((1 << 8) - 1))
#define PAL1(entry)      (entry & 0xffffffc00)
#define PAL2(entry)      (entry & 0xffffff000)

#define L1_TYPE      0b11
#define L1_FAULT     0b00
#define L1_COARSE    0b01
#define L1_SECTION   0b10
#define L1_FINE      0b11

#define L2_TYPE      0b11
#define L2_FAULT     0b00
#define L2_LARGE     0b01
#define L2_SMALL     0b10
#define L2_TINY       0b11

uint32_t
ttb[4096]__attribute__((__aligned__(16*1024))) = { L1_FAULT };


extern uint32_t *ram_start;
extern uint32_t *ram_end;
extern uint32_t *kernel_start;
extern uint32_t *kernel_end;

void
init_memory(void)
{
  int i;
  
#if 0
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

#endif

  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  mmu_load_ttb(ttb);

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

