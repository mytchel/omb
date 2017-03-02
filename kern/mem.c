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

void *
heappop(void)
{
  struct heappage *p;

  do {
    p = up->heap;
  } while (p != nil && cas(&up->heap, p, p->next) != OK);

  return (void *) p;
}

void
heapadd(void *start)
{
  struct heappage *p;

  p = (struct heappage *) start;

  do {
    p->next = up->heap;
  } while (cas(&up->heap, p->next, p) != OK);
}

int
checkflags(int need, int got)
{
  return OK;
}

void
grantinit(struct grant *g, reg_t start)
{
  g->state = GRANT_EMPTY;
  g->from = 0;
  g->flags = 0;
  g->npages = 0;
  g->pages = (reg_t *) start;

  g->maxnpages = PAGE_SIZE / sizeof(reg_t);
}

void
grantfree(struct grant *g)
{
  heapadd(g->pages);
}

int
granttake(struct grant *g)
{
  if (cas(&g->state, (void *) GRANT_EMPTY, (void *) GRANT_PART) != OK) {
    procwlistadd(&g->waiting, up);
    procsend();
  }

  return OK;
}

int
granttakenb(struct grant *g)
{
  return cas(&g->state, (void *) GRANT_EMPTY, (void *) GRANT_PART);
}

int
grantready(struct grant *g)
{
  g->state = GRANT_READY;
  return OK;
}

int
grantuntake(struct grant *g)
{
  g->state = GRANT_EMPTY;
  return OK;
}
