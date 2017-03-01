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
messagetest(void)
{
  struct message m;
  struct mtest *t;
  int i;

  m.type = 1;
  t = (struct mtest *) m.body;

  for (i = 0; i < 256; i++) {
    memmove(t->buf, "Hello.", 7);
    t->buf[0] = 'H' + i % 15;
    
    printf("%i sending message to 1 with type %i, and buf '%s'\n",
	   up->pid, m.type, t->buf);
   
    if (ksend(1, &m) != OK) {
      printf("ksend failed\n");
      return ERR;
    }

    if (krecv(&m) != OK) {
      printf("krecv failed\n");
      return ERR;
    }

    printf("%i recieved message with type %i, from %i, with buf '%s'\n",
	   up->pid, m.type, m.from, t->buf);
  }

  return OK;
}

static int
mainprocfunc(void *arg)
{
  struct label u;
  reg_t va, pa, o;
  int r;

  printf("mainproc on pid %i\n", up->pid);
  printf("message test\n");

  if (messagetest() == OK) {
    printf("\n\nmessage test: success!\n\n\n");
  } else {
    printf("\nmessage test: failed!\n\n\n\n");
    while (true)
      ;
  }

  printf("load init code\n");
  
  va = 0x1000;
  pa = (reg_t) &initcode;

  printf("init code at 0x%h len %i\n", pa, initcodelen);

  for (o = 0; o < initcodelen; o += PAGE_SIZE) {
    printf("map from 0x%h to 0x%h\n", pa + o, va + o);
    r = mappingadd(up->addrspace, va + o, pa + o, MEM_rw);
    if (r != OK) {
      printf("error mapping 0x%h -> 0x%h for init!\n",
	     va + o, pa + o);
      return ERR;
    }
  }

  memset(&u, 0, sizeof(struct label));
  u.psr = MODE_USR;
  u.sp = USTACK_TOP;
  u.pc = va;

  printf("start init code\n");
  
  droptouser(&u, up->kstack + PAGE_SIZE);
  
  return OK;
}

void
mainprocinit(void)
{
  reg_t page, kstack, mbox, aspage;
  struct addrspace *addrspace;
  struct heappage *heap, *h;
  struct proc *p;
  size_t hsize;
  
  page = getrampage();
  kstack = getrampage();
  mbox = getrampage();
  aspage = getrampage();

  heap = nil;
  for (hsize = 0; hsize < 1024 * 64; hsize += PAGE_SIZE) {
    h = (struct heappage *) getrampage();
    h->next = heap;
    heap = h;
  }
 
  addrspace = addrspacenew(aspage);

  p = procnew(page, kstack, mbox, heap, addrspace);

  forkfunc(p, &mainprocfunc, nil);
  procready(p);
}
