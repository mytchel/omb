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
getrampages(struct grant *g, size_t len)
{
  size_t i;
  
  g->npages = len / PAGE_SIZE;
  if (g->npages >= g->maxnpages) {
    return ERR;
  }

  for (i = 0; i < g->npages; i++) {
    g->pages[i] = getrampage();
    if (g->pages[i] == nil) {
      /* TODO: add pages back */
      return ERR;
    }
  }

  return OK;
}

static int
getiopages(struct grant *g, reg_t pa, size_t len)
{
  size_t i;
  
  g->npages = len / PAGE_SIZE;
  if (g->npages >= g->maxnpages) {
    return ERR;
  }

  for (i = 0; i < g->npages; i++) {
    g->pages[i] = getiopage(pa + i * PAGE_SIZE);
    if (g->pages[i] == nil) {
      /* TODO: add pages back */
      return ERR;
    }
  }

  return OK;
}

static int
handlereq(struct memreq *req)
{
  struct memresp resp;
  struct grant *g;
  struct proc *p;

  resp.type = COM_MEMRESP;
  
  printf("memproc got memreq from %i wanting %i bytes, with flags %i, pa 0x%h\n",
	 req->from, req->len, req->flags, req->pa);

  p = findproc(req->from);
  if (p == nil) {
    return ERR;
  }

  g = &p->grant;
  if (granttake(g) != OK) {
    printf("failed to take grant\n");
    resp.ret = ERR;
    return ksend(req->from, (struct message *) &resp);
  }

  g->from = up->pid;
  g->flags = req->flags;

  if (req->pa == nil) {
    /* random ram page */
    resp.ret = getrampages(g, req->len);
  } else {
    /* specific pages */
    resp.ret = getiopages(g, req->pa, req->len);
  }

  if (resp.ret == OK) {
    grantready(g);
  } else {
    if (grantuntake(g) != OK) {
      printf("memproc failed to untake grant after failing to get pages!\n");
    }
  }
      
  return ksend(req->from, (struct message *) &resp);
}

static int
memprocfunc(void *arg)
{
  struct message m;

  printf("memproc on pid %i\n", up->pid);
  
  while (true) {
    if (krecv(&m) == OK) {
      if (m.type == COM_MEMREQ) {
	if (handlereq((struct memreq *) &m) != OK) {
	  printf("error handling request!\n");
	}
      }
    }
  }

  return ERR;
}

void
memprocinit(void)
{
  reg_t page, kstack, mbox, grant;
  struct heappage *heap, *h;
  struct proc *p;
  size_t hsize;

  page = getrampage();
  kstack = getrampage();
  mbox = getrampage();
  grant = getrampage();

  heap = nil;
  for (hsize = 0; hsize < 1024 * 64; hsize += PAGE_SIZE) {
    h = (struct heappage *) getrampage();
    h->next = heap;
    heap = h;
  }
  
  p = procnew(page, kstack, mbox, grant, heap, nil);

  forkfunc(p, &memprocfunc, nil);
  procready(p);
}

