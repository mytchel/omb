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

static void addtolistback(struct proc **, struct proc *);
static bool removefromlist(struct proc **, struct proc *);

static uint32_t nextpos = 0;
static uint32_t nextcode = 1;

#define PROCSMAX 1024

static struct proc *procs[PROCSMAX] = {nil};
static struct proc *ready = nil;

struct proc *up = nil;

static struct proc *
nextproc(void)
{
  struct proc *p;

  p = ready;

  if (p == nil) {
    return nil;
  } else {
    ready = p->snext;
    return p;
  }
}

void
schedule(void)
{
  if (up != nil) {
    if (up->state == PROC_oncpu) {
      up->state = PROC_ready;
      addtolistback(&ready, up);
    }
    
    if (setlabel(&up->label)) {
      return;
    }
  }

  up = nextproc();
  setsystick(mstoticks(100));

  if (up == nil) {
    nilfunc();
 } else {
    up->state = PROC_oncpu;
    mmuswitch(up);
    gotolabel(&up->label);
  }
}

void
addtolistback(struct proc **l, struct proc *p)
{
  struct proc *pp;

  p->snext = nil;
  
  while (true) {
    for (pp = *l; pp != nil && pp->snext != nil; pp = pp->snext)
      ;

    if (pp == nil) {
      if (cas(l, nil, p) == OK) {
	break;
      }
    } else if (cas(&pp->snext, nil, p) == OK) {
      break;
    }
  }
}

bool
removefromlist(struct proc **l, struct proc *p)
{
  struct proc *pt;

  while (true) {
    if (*l == p) {
      if (cas(l, p, p->snext)) {
	return true;
      }
    } else {
      for (pt = *l; pt != nil && pt->snext != p; pt = pt->snext)
	;

      if (pt == nil) {
	return false;
      } else if (cas(&pt->snext, p, p->snext) == OK) {
	return true;
      }
    }
  }
}
	
struct proc *
procnew(reg_t page, reg_t kstack, reg_t mbox, reg_t grant,
	struct heappage *heap,
	struct addrspace *addrspace)
{
  int pos, code, ncode;
  struct proc *p;
	
  p = (struct proc *) page;

  p->kstack = kstack;
  p->heap = heap;

  mboxinit(&p->mbox, mbox);
  ustackinit(&p->ustack);
  grantinit(&p->grant, grant);

  p->addrspace = addrspace;

  p->state = PROC_suspend;

  p->wnext = nil;
  p->snext = nil;

  /* Get a random code, this will suffice for now. */
  do {
    code = nextcode;
    ncode = ((code + 13) * 3) & 0xffff;
  } while (cas(&nextcode, (void *) code, (void *) ncode) != OK);

  /* Put proc in the procs table, and set pid. */
  do {
    do {
      pos = nextpos % PROCSMAX;
    } while (cas(&nextpos, (void *) pos, (void *) (pos + 1)) != OK);

    p->pid = (pos << 16) | code;
 
  } while (cas(&procs[pos], nil, p) != OK);

  return p;
}

void
procexit(struct proc *p)
{
  struct addrspace *space;
  intr_t i;

  printf("procexit %i\n", p->pid);
  
  if (p->state == PROC_ready) {
    removefromlist(&ready, p);
  }

  ustackfree(&p->ustack);

  do {
    space = p->addrspace;
  } while (cas(&p->addrspace, space, nil) != OK);

  if (space != nil) {
    addrspacefree(space);
  }

  mboxclose(&p->mbox);

  /* TODO: free kstack and proc page */

  while (cas(&procs[p->pid >> 16], p, nil) != OK)
    ;

  i = setintr(INTR_off);
  if (p == up) {
    up = nil;
    schedule();
  }
  setintr(i);
}

void
procsuspend(struct proc *p)
{
  intr_t i;

  p->state = PROC_suspend;

  if (p->state == PROC_ready) {
    removefromlist(&ready, p);
  }

  i = setintr(INTR_off);
  if (p == up) {
    schedule();
  }
  setintr(i);
}

void
procrecv(void)
{
  intr_t i;

  i = setintr(INTR_off);
  up->state = PROC_recv;
  schedule();
  setintr(i);
}

void
procsend(void)
{
  intr_t i;

  i = setintr(INTR_off);
  up->state = PROC_send;
  schedule();
  setintr(i);
}

void
procready(struct proc *p)
{
  if (p->state == PROC_ready || p->state == PROC_oncpu) {
    return;
  }

  p->state = PROC_ready;
  addtolistback(&ready, p);
}

struct proc *
findproc(int pid)
{
  struct proc *p;
  int pos;

  pos = pid >> 16;
  if (pos > PROCSMAX) {
    return nil;
  }
  
  p = procs[pos];
  if (p == nil) {
    return nil;
  } else if (p->pid == pid) {
    return p;
  } else {
    return nil;
  }
}

int
procwlistadd(struct proc **pp, struct proc *p)
{
  while (*pp != nil)
    pp = &((*pp)->wnext);

  p->wnext = nil;

  return cas(pp, nil, p);
}

struct proc *
procwlistpop(struct proc **pp)
{
  struct proc *p;

  do {
    p = *pp;
  } while (p != nil && cas(pp, p, p->wnext) != OK);

  return p;
}
