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

extern uint32_t *init_start;
extern uint32_t *init_end;

static int
mainprocfunc(void *arg)
{
  reg_t va, pa;
  int r;

  va = 0x1000;
  pa = (reg_t) &init_start;

  while (pa < (reg_t) &init_end) {
    r = mappingadd(up->addrspace, va, pa, PAGE_rw);
    if (r != OK) {
      printf("error mapping 0x%h -> 0x%h for init!\n",
	     va, pa);
      return ERR;
    }

    va += PAGE_SIZE;
    pa += PAGE_SIZE;
  }

  printf("mappings done\n");
  
  return OK;
}

void
mainprocinit(void)
{
  reg_t page, kstack, mboxpage, aspage;
  struct mbox *mbox;
  struct addrspace *addrspace;
  struct proc *p;
  
  page = getrampage();
  kstack = getrampage();
  mboxpage = getrampage();
  aspage = getrampage();

  mbox = mboxnew(mboxpage);
  addrspace = addrspacenew(aspage);

  p = procnew(page, kstack, mbox, addrspace);

  forkfunc(p, &mainprocfunc, nil);
  procready(p);
}
