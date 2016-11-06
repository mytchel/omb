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

#include "head.h"

static void addtolistfront(struct proc **, struct proc *);
static void addtolistback(struct proc **, struct proc *);
static void removefromlist(struct proc **, struct proc *);

struct priority_queue {
  uint32_t quanta;
  struct proc *ready, *used;
};

static uint32_t nextpid = 1;

static struct priority_queue queues[17];
static struct proc *nullproc;

static struct proc *waitchildren = nil;
static struct proc *sleeping = nil;
static struct proc *suspended = nil;

struct proc *up = nil;

void
schedulerinit(void)
{
 int i;
  
  for (i = 0; i < 17; i++) {
    queues[i].quanta = mstoticks((18 - i) * 1000 + 10);
    queues[i].ready = nil;
    queues[i].used = nil;
  }

  queues[0].quanta = mstoticks(5);
  
  nullproc = procnew(0);
  if (nullproc == nil) {
    panic("Failed to create null proc!\n");
  }

  forkfunc(nullproc, &nullprocfunc, nil);
  procready(nullproc);
}

static struct proc *
popnextproc(void)
{
  struct proc *p;
  unsigned int i;

  for (i = 1; i < 17; i++) {
    p = queues[i].ready;
    if (p != nil) {
      queues[i].ready = p->next;
      p->list = nil;
      return p;
    }
  }

  return nil;
}

static struct proc *
nextproc(void)
{
  struct proc *p;
  unsigned int i;
  
  p = popnextproc();
  if (p != nil) {
    return p;
  }

  for (i = 1; i < 17; i++) {
    queues[i].ready = queues[i].used;
    queues[i].used = nil;
  }
  
  p = popnextproc();
  if (p != nil) {
    return p;
  }

  removefromlist(nullproc->list, nullproc);
  nullproc->timeused = 0;
  return nullproc;
}

static void
updatesleeping(uint32_t t)
{
  struct proc *p, *pp, *pt;

  pp = nil;
  p = sleeping;
  while (p != nil) {
    if (t >= p->sleep) {
      if (pp == nil) {
	sleeping = p->next;
      } else {
	pp->next = p->next;
      }

      pt = p->next;

      p->state = PROC_ready;
      p->list = nil;
      addtolistfront(&queues[p->priority].ready, p);
      p = pt;
    } else {
      p->sleep -= t;

      pp = p;
      p = p->next;
    }
  }
}

void
schedule(void)
{
  uint32_t t, q;

  if (up && setlabel(&up->label)) {
    return;
  }

  t = ticks();

  if (up != nil) {
    up->timeused += t;

    if (up->state == PROC_oncpu) {
      up->state = PROC_ready;

      q = queues[up->priority].quanta;
      if (up->timeused + 50 < q) {
	addtolistback(&queues[up->priority].ready, up);
      } else {
	up->timeused = 0;
	addtolistback(&queues[up->priority].used, up);
      }
    }
  }

  updatesleeping(t);
	
  up = nextproc();
  up->state = PROC_oncpu;

  mmuswitch(up);
  
  cticks();
  setsystick(queues[up->priority].quanta
	     - up->timeused);

  gotolabel(&up->label);
}
	
struct proc *
procnew(unsigned int priority)
{
  struct proc *p;
	
  p = malloc(sizeof(struct proc));
  if (p == nil) {
    return nil;
  }
	
  p->pid = nextpid++;
  p->priority = priority;

  p->mmu = mmunew();

  p->kstack = getrampage();
  if (p->kstack == nil) {
    free(p);
    return nil;
  }
 
  p->ureg = nil;

  p->timeused = 0;

  p->cnext = nil;
  p->children = nil;
  p->deadchildren = nil;
  p->parent = nil;

  p->dot = nil;
  p->dotchan = nil;
  p->root = nil;

  p->ustack = nil;
  p->mgroup = nil;
  p->fgroup = nil;
  p->ngroup = nil;

  p->state = PROC_suspend;
  addtolistfront(&suspended, p);

  return p;
}

void
procexit(struct proc *p, int code)
{
  struct proc *c, *cprev;
  
  p->state = PROC_dead;
  p->exitcode = code;
  
  if (p->dotchan != nil) {
    chanfree(p->dotchan);
    p->dotchan = nil;
  }

  if (p->fgroup != nil) {
    fgroupfree(p->fgroup);
    p->fgroup = nil;
  }

  if (p->ngroup != nil) {
    ngroupfree(p->ngroup);
    p->ngroup = nil;
  }

  if (p->dot != nil) {
    pathfree(p->dot);
    p->dot = nil;
  }

  if (p->ustack != nil) {
    pagelfree(p->ustack);
    p->ustack = nil;
  }

  if (p->mmu != nil) {
    mmufree(p->mmu);
    p->mmu = nil;
  }
  
  if (p->mgroup != nil) {
    mgroupfree(p->mgroup);
    p->mgroup = nil;
  }

  removefromlist(p->list, p);
  p->list = nil;

  /* Update childrens parent to be their grandparent */

  cprev = nil;
  for (c = p->children; c != nil; cprev = c, c = c->cnext) {
    c->parent = p->parent;
  }

  if (cprev == nil) {
    p->children = p->cnext;
  } else {
    cprev->cnext = p->cnext;
  }

  if (p->parent != nil) {
    /* Remove proc from parents child list */
    cprev = nil;
    for (c = p->parent->children; c != nil; cprev = c, c = c->cnext) {
      if (c == p) {
	/* And add procs children to parents child list */
	if (cprev == nil) {
	  p->parent->children = p->children;
	} else {
	  cprev->cnext = p->children;
	}

	p->children = nil;

	break;
      }
    }

    if (c == nil) {
      goto err;
    }
    
    addtolistback(&p->parent->deadchildren, p);

    p->next = p->deadchildren;

    if (p->parent->state == PROC_waiting
	&& p->parent->list == &waitchildren) {
      procready(p->parent);
    }

    goto norm;
  }

  /* Not really an error but what ever */
 err:
  procfree(p);

 norm:
  if (p == up) {
    up = nil;
  }
}

void
procfree(struct proc *p)
{
  pagefree(p->kstack);
  free(p);
}

struct proc *
procwaitchildren(void)
{
  struct proc *p;

  if (up->children == nil) {
    return nil;
  } else if (up->deadchildren == nil) {
    setintr(INTR_OFF);
    procwait(up, &waitchildren);
    schedule();
  }

  p = up->deadchildren;
  up->deadchildren = p->next;

  setintr(INTR_ON);

  return p;
}

void
procsetpriority(struct proc *p, unsigned int priority)
{
  p->priority = priority;

  if (p->state == PROC_ready) {
    removefromlist(p->list, p);
    p->timeused = 0;
    addtolistback(&queues[p->priority].ready, p);
  }
}

void
procready(struct proc *p)
{
  p->state = PROC_ready;

  removefromlist(p->list, p);

  p->timeused = 0;
  addtolistfront(&queues[p->priority].ready, p);
}

void
procsleep(struct proc *p, uint32_t ms)
{
  p->state = PROC_sleeping;
  p->sleep = mstoticks(ms);
  
  removefromlist(p->list, p);
  addtolistfront(&sleeping, p);
}

void
procyield(struct proc *p)
{
  p->state = PROC_sleeping;
  p->timeused = 0;

  removefromlist(p->list, p);
  addtolistback(&queues[p->priority].used, p);
}

void
procsuspend(struct proc *p)
{
  p->state = PROC_suspend;

  removefromlist(p->list, p);
  addtolistfront(&suspended, p);
}

void
procwait(struct proc *p, struct proc **wlist)
{
  p->state = PROC_waiting;

  removefromlist(p->list, p);
  addtolistback(wlist, p);
}

void
addtolistfront(struct proc **l, struct proc *p)
{
  p->list = l;
  p->next = *l;
  *l = p;
}

void
addtolistback(struct proc **l, struct proc *p)
{
  struct proc *pp;

  p->next = nil;
  p->list = l;

  if (*l == nil) {
    *l = p;
  } else {
    for (pp = *l; pp->next != nil; pp = pp->next)
      ;
    pp->next = p;
  }
}

void
removefromlist(struct proc **l, struct proc *p)
{
  struct proc *pp, *pt;

  if (l == nil) {
    return;
  }
  
  pp = nil;
  for (pt = *l; pt != nil && pt != p; pp = pt, pt = pt->next)
    ;

  if (pp == nil) {
    *l = p->next;
  } else {
    pp->next = p->next;
  }

  p->list = nil;
}
