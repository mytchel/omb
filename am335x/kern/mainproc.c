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
#include "trap.h"

extern char initcode[];
extern size_t initcodelen;

static int
mainprocfunc(void *arg)
{
  struct label u;
  reg_t va, pa, o;
  int r;

  printf("mainproc on pid %i\n", up->pid);

  printf("load init code\n");
  
  va = 0x1000;
  pa = (reg_t) &initcode;

  printf("init code at 0x%h len %i\n", pa, initcodelen);

  for (o = 0; o < initcodelen; o += PAGE_SIZE) {
    r = mappingadd(up->space, va + o, pa + o, MEM_r|MEM_w|MEM_x);
    if (r != OK) {
      printf("error mapping 0x%h -> 0x%h for init!\n",
	     va + o, pa + o);
      return r;
    }
  }

  pa = (reg_t) heappop();
  if (pa == nil) {
    printf("mainproc failed to get a page for user stack!\n");
    return ENOMEM;
  }
  
  r = mappingadd(up->space, USTACK_TOP - PAGE_SIZE, pa, MEM_r|MEM_w);
  if (r != OK) {
    printf("error mapping 0x%h -> 0x%h for init!\n",
	   USTACK_TOP - PAGE_SIZE, pa);
    return r;
  } 

  memset(&u, 0, sizeof(struct label));
  u.psr = MODE_USR;
  u.sp = USTACK_TOP;
  u.pc = va;

  printf("start init code\n");
  
  droptouser(&u, up->kstack + PAGE_SIZE);
  
  return ERR;
}

void
mainprocinit(void)
{
  reg_t page, kstack, mbox, grant, aspage;
  struct heappage *heap, *h;
  struct space *space;
  struct proc *p;
  size_t hsize;
  
  page = getrampage();
  kstack = getrampage();
  mbox = getrampage();
  grant = getrampage();
  aspage = getrampage();

  heap = nil;
  for (hsize = 0; hsize < 1024 * 64; hsize += PAGE_SIZE) {
    h = (struct heappage *) getrampage();
    h->next = heap;
    heap = h;
  }
 
  space = spacenew(aspage);

  p = procnew(page, kstack, mbox, grant, heap, space);

  forkfunc(p, &mainprocfunc, nil);
  procready(p);
}
