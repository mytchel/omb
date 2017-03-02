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

static reg_t
fillgrant(struct grant *g, int flags, void *start, size_t len)
{
  size_t n;
  int f;

  g->from = up->pid;
  g->flags = flags;
  g->npages = len / PAGE_SIZE;

  for (n = 0; n < g->npages; n++) {
    g->pages[n] = mappingfind(up->space,
			      (reg_t) start + n * PAGE_SIZE,
			      &f);

    if (g->pages[n] == nil || checkflags(flags, f) != OK) {
      grantuntake(g);
      return ERR;
    }
  }

  /* Should this be happening here in mmap? */
  for (n = 0; n < g->npages; n++) {
    mappingremove(up->space, (reg_t) start + n * PAGE_SIZE);
  }

  return grantready(g);
}

reg_t
sysmgrant(int to, int flags, void *start, size_t len)
{
  struct proc *p;
  int r;
 
  printf("%i called mgrant to %i with flags %i from 0x%h len %i\n",
	 up->pid, to, flags, start, len);

  if ((reg_t) start % PAGE_SIZE != 0 || len % PAGE_SIZE != 0) {
    return ERR;
  }
  
  p = findproc(to);
  if (p == nil) {
    return ERR;
  }

  r = granttake(&p->grant);
  if (r != OK) {
    return r;
  }

  return fillgrant(&p->grant, flags, start, len);
}

reg_t
sysmgrantnb(int to, int flags, void *start, size_t len)
{
  struct proc *p;
  int r;

  printf("%i called mgrantnb to %i with flags %i from 0x%h len %i\n",
	 up->pid, to, flags, start, len);

  if ((reg_t) start % PAGE_SIZE != 0 || len % PAGE_SIZE != 0) {
    return ERR;
  }
  
  p = findproc(to);
  if (p == nil) {
    return ERR;
  }

  r = granttakenb(&p->grant);
  if (r != OK) {
    return r;
  }

  return fillgrant(&p->grant, flags, start, len);
}

reg_t
sysmmap(int flags, void *start, size_t len)
{
  reg_t va, pa;
  size_t i;
  int f;
  
  printf("%i called mmap with flags %i from 0x%h len %i\n",
	 up->pid, flags, start, len);

  if (up->grant.state != GRANT_READY) {
    return ERR;
  }

  if ((reg_t) start % PAGE_SIZE != 0 || len % PAGE_SIZE != 0) {
    return ERR;
  } else if (len != up->grant.npages * PAGE_SIZE) {
    return ERR;
  } else if (checkflags(up->grant.flags, flags) != OK) {
    return ERR;
  }

  va = (reg_t) start;
  for (i = 0; i < up->grant.npages; i++) {
    pa = mappingfind(up->space, va, &f);
    if (pa != nil) {
      /* TODO: do something about the mapped pages. Not sure what. */
      return ERR;
    }

    if (mappingadd(up->space, va, up->grant.pages[i], flags) != OK) {
      /* TODO: do something about the mapped pages. Not sure what. */
      return ERR;
    }
    
    va += PAGE_SIZE;
  }

  up->grant.state = GRANT_EMPTY;
  
  return OK;
}

