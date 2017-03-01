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

static int
memprocfunc(void *arg)
{
  struct message m;
  struct mtest *t;
  int from;

  printf("memproc on pid %i\n", up->pid);
  
  while (true) {
    if (krecv(&m) == OK) {
      printf("memproc got message from %i of type %i\n", m.from, m.type);
      if (m.type == 1) {
	t = (struct mtest *) m.body;

	m.type = 1;
	printf("memproc got message from %i saying '%s'\n",
	       m.from, t->buf);

	from = m.from;

	memmove(t->buf, "Hello", 6);

	ksend(from, &m);
      }
    }
  }

  return ERR;
}

void
memprocinit(void)
{
  reg_t page, kstack, mbox;
  struct heappage *heap, *h;
  struct proc *p;
  size_t hsize;

  page = getrampage();
  kstack = getrampage();
  mbox = getrampage();

  heap = nil;
  for (hsize = 0; hsize < 1024 * 64; hsize += PAGE_SIZE) {
    h = (struct heappage *) getrampage();
    h->next = heap;
    heap = h;
  }
  
  p = procnew(page, kstack, mbox, heap, nil);

  forkfunc(p, &memprocfunc, nil);
  procready(p);
}

