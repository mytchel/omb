/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
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

#include "../kern/head.h"
#include "fns.h"

extern char *initcodetext, *initcodedata;
extern int initcodetextlen, initcodedatalen;

static void
initmainproc(void);

int
main(void)
{
  puts("Bom Booting...\n");

  memoryinit();
  intcinit();
  timersinit();
  watchdoginit();

  schedulerinit();

  rootfsinit();
  initmainproc();

  puts("Starting procs...\n");
  
  schedule();
	
  /* Should never be reached. */
  return 0;
}

static struct pagel *
copysegment(void *start, bool rw, bool c, char **buf, size_t len)
{
  struct pagel *pl, *pp, *pages;
  struct page *pg;
  size_t i, l;

  pp = nil;
  pages = nil;

  i = 0;
  while (len > 0) {
    pg = getrampage();
    if (pg == nil)
      panic("copy segment ran out of pages!\n");

    pl = wrappage(pg, start, rw, c);
    if (pl == nil)
      panic("copy segment failed to wrap page!\n");
    
    l = len > PAGE_SIZE ? PAGE_SIZE : len;

    memmove(pg->pa, (uint8_t *) buf + i, l);
		
    len -= l;	
    i += l;
		
    start += PAGE_SIZE;

    if (pp == nil) {
      pages = pl;
    } else {
      pp->next = pl;
    }

    pp = pl;
  }
	
  return pages;
}

static int
mainproc(void *arg)
{
  struct label ureg;

  setintr(INTR_OFF);

  memset(&ureg, 0, sizeof(struct label));

  ureg.sp = USTACK_TOP;
  ureg.psr = MODE_USR;
  ureg.pc = 4;

  droptouser(&ureg);

  return 0; /* Never reached. */
}

void
initmainproc(void)
{
  struct proc *p;
  struct pagel *pl;
  struct page *pg;
  struct path *path;
		
  p = procnew(10);
  if (p == nil) {
    printf("Failed to create main proc\n");
    return;
  }
	
  forkfunc(p, &mainproc, nil);

  pg = getrampage();
  p->ustack = wrappage(pg, (void *) (USTACK_TOP - PAGE_SIZE),
		      true, true);

  p->mgroup = mgroupnew();
  p->mgroup->pages = copysegment((void *) 0, false, true,
				 &initcodetext, initcodetextlen);

  for (pl = p->mgroup->pages; pl != nil; pl = pl->next) {
    if (pl->next == nil) {
      break;
    }
  }

  pl->next = copysegment((void *) ((uint32_t) pl->va + PAGE_SIZE),
	      true, true, &initcodedata, initcodedatalen);
	
  p->parent = nil;

  p->fgroup = fgroupnew();
  p->ngroup = ngroupnew();

  path = nil;
  ngroupaddbinding(p->ngroup, rootfsbinding, path, ROOTFID);

  procready(p);
}

int
nullprocfunc(void *arg)
{
  while (true) {

  }

  return 0;
}

