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

reg_t
sysgetpid(void)
{
  printf("%i called getpid\n", up->pid);
  return up->pid;
}

reg_t
sysexit(void)
{
  intr_t i;
  
  printf("%i called exit\n", up->pid);

  i = setintr(INTR_off);
  procexit(up);
  schedule();
  setintr(i);
  
  return ERR;
}

reg_t
sysfork(void *page, void *kstack, void *ustacktop, va_list ap)
{
  reg_t ppage, pkstack, pmbox, pgrant, pheap, vheap;
  int pagef, kstackf, mboxf, grantf;
  void *mbox, *grant, *heapstart;
  struct heappage *heap, *h;
  size_t hsize, size;
  struct space *space;
  struct proc *p;
  int r, f;
  
  printf("%i called fork\n", up->pid);

  size = sizeof(void *) + sizeof(void *)
    + sizeof(void *) + sizeof(size_t);

  printf("check ap 0x%h\n", ap);
  
  if (validaddr(ap, size, MEM_r) != OK) {
    return ERR;
  }
  
  mbox = va_arg(ap, void *);
  grant = va_arg(ap, void *);
  heapstart = va_arg(ap, void *);
  hsize = va_arg(ap, size_t);

  printf("mbox 0x%h\ngrant 0x%h\nheapstart 0x%h\n hsize %i\n",
	 mbox, grant, heapstart, hsize);

  printf("check page\n");
  ppage = mappingfind(up->space, (reg_t) page, &pagef);
  if (ppage == nil || checkflags(MEM_r|MEM_w, pagef) != OK) {
    return ERR;
  }
  
  printf("check page\n");
  pkstack = mappingfind(up->space, (reg_t) kstack, &kstackf);
  if (pkstack == nil || checkflags(MEM_r|MEM_w, kstackf) != OK) {
    return ERR;
  }

  printf("check page\n");
  pmbox = mappingfind(up->space, (reg_t) mbox, &mboxf);
  if (pmbox == nil || checkflags(MEM_r|MEM_w, mboxf) != OK) {
    printf("mbox mapping bad 0x%h %i\n", pmbox, mboxf);
    return ERR;
  }

  printf("check page\n");
  pgrant = mappingfind(up->space, (reg_t) grant, &grantf);
  if (pgrant == nil || checkflags(MEM_r|MEM_w, grantf) != OK) {
    return ERR;
  }

  printf("remove page\n");
  if (mappingremove(up->space, (reg_t) page) != OK) {
    goto err0;
  }

  printf("remove page\n");
  if (mappingremove(up->space, (reg_t) kstack) != OK) {
    goto err1;
  }
  
  printf("remove page\n");
  if (mappingremove(up->space, (reg_t) mbox) != OK) {
    goto err2;
  }
  
  printf("remove page\n");
  if (mappingremove(up->space, (reg_t) grant) != OK) {
    goto err3;
  }

  printf("remove heap pages\n");
  heap = nil;
  vheap = (reg_t) heapstart;
  for (size = 0; size < hsize; size += PAGE_SIZE) {
    pheap = mappingfind(up->space, vheap + size, &f);
    if (pheap == nil || checkflags(MEM_r|MEM_w, f) != OK) {
      goto err4;
    } else if (mappingremove(up->space, vheap + size) != OK) {
      goto err4;
    }

    h = (struct heappage *) pheap;
    h->next = heap;
    heap = h;
  }

  printf("get space\n");
  space = up->space;
  do {
    r = space->refs;
  } while (cas(&space->refs, (void *) r, (void *) (r + 1)) != OK);

  printf("make proc\n");
  p = procnew(ppage, pkstack, pmbox, pgrant, heap, space);

  printf("%i fork to new proc pid %i\n", up->pid, p->pid);

  r = forkchild(p);

  return r;

 err4:
  /* TODO: reclaim heap on failure. */
  mappingadd(up->space, (reg_t) grant, pgrant, grantf);
 err3:
  mappingadd(up->space, (reg_t) mbox, pmbox, mboxf);
 err2:
  mappingadd(up->space, (reg_t) kstack, pkstack, kstackf);
 err1:
  mappingadd(up->space, (reg_t) page, ppage, pagef);
 err0:
  return ERR;
}

