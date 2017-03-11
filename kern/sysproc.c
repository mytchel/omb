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

static void
reclaimheap(struct heappage *h, void *start, size_t len)
{
  printf("should reclaim heap\n");
}

static struct heappage *
buildheap(void *start, size_t len)
{
  struct heappage *heap, *h;
  reg_t pa;
  size_t l;
  int f;
  
  printf("remove heap pages\n");
  heap = nil;
  for (l = 0; l < len; l += PAGE_SIZE) {
    printf("find mapping for 0x%h\n", (reg_t) start + l);
    pa = mappingfind(up->space, (reg_t) start + l, &f);
    if (pa == nil || checkflags(MEM_r|MEM_w, f) != OK) {
      goto err;
    } else if (mappingremove(up->space, (reg_t) start + l) != OK) {
      goto err;
    }

    printf("add 0x%h to kheap\n", pa);
    h = (struct heappage *) pa;
    h->next = heap;
    heap = h;
  }

  printf("heap built\n");
  return heap;

 err:
  printf("error building heap\n");
  reclaimheap(heap, start, l);
  return nil;
}

static struct space *
buildspace(void *start, size_t len, void *offset)
{
  struct space *s;
  reg_t pa, l;
  void *p;
  int f;
  
  printf("build space\n");

  p = heappop();
  if (p == nil) {
    return nil;
  }
  
  s = spacenew((reg_t) p);

  for (l = 0; l < len; l += PAGE_SIZE) {
    printf("check 0x%h\n", (reg_t) start + l);
    
    pa = mappingfind(up->space, (reg_t) start + l, &f);
    if (pa == nil) {
      continue;
    }

    if (mappingremove(up->space, (reg_t) start + l) != OK) {
      goto err;
    }

    printf("map 0x%h -> 0x%h -> 0x%h\n", (reg_t) start + l, pa, (reg_t) offset + l);
    if (mappingadd(s, (reg_t) offset + l, pa, f) != OK) {
      goto err;
    }
  }
  
  return s;

 err:
  /* TODO: Reclaim pages */
  return nil;
}

reg_t
sysfork(void *page, void *kstack, void *kheap, va_list ap)
{
  void *start, *offset, *entry, *ustacktop, *arg;
  struct heappage *heap;
  reg_t ppage, pkstack;
  int pagef, kstackf;
  struct space *space;
  size_t hlen, len;
  struct proc *p;
  
  printf("%i called fork\n", up->pid);

  len = sizeof(size_t)
    + sizeof(void *)
    + sizeof(size_t)
    + sizeof(void *)
    + sizeof(void *)
    + sizeof(void *)
    + sizeof(void *);

  printf("check ap 0x%h\n", ap);
  
  if (validaddr(ap, len, MEM_r) != OK) {
    return ERR;
  }
  
  hlen = va_arg(ap, size_t);
  start = va_arg(ap, void *);
  len = va_arg(ap, size_t);
  offset = va_arg(ap, void *);
  entry = va_arg(ap, void *);
  ustacktop = va_arg(ap, void *);
  arg = va_arg(ap, void *);

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

  printf("remove page\n");
  if (mappingremove(up->space, (reg_t) page) != OK) {
    goto err0;
  }

  printf("remove kstack\n");
  if (mappingremove(up->space, (reg_t) kstack) != OK) {
    goto err1;
  }

  heap = buildheap(kheap, hlen);
  if (heap == nil) {
    goto err2;
  }
  
  space = buildspace(start, len, offset);
  if (space == nil) {
    printf("error building space\n");
    goto err3;
  }
  
  printf("make proc\n");
  p = procnew(ppage, pkstack, heap, space);

  printf("%i fork to new proc pid %i\n", up->pid, p->pid);

  forkchild(p, entry, ustacktop, arg);
  procready(p);

  return p->pid;

 err3:
  reclaimheap(heap, kheap, hlen);
 err2:
  printf("reclaim kstack\n");
  mappingadd(up->space, (reg_t) kstack, pkstack, kstackf);
 err1:
  printf("reclaim page\n");
  mappingadd(up->space, (reg_t) page, ppage, pagef);
 err0:
  printf("return err\n");
  return ERR;
}

