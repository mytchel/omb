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

#include <c.h>
#include <syscalls.h>
#include <com.h>
#include <mem.h>
#include <stdarg.h>
#include <string.h>


#define ASSERT_PAGE_SIZE(m)				 \
  STATIC_ASSERT(sizeof(struct m) <= PAGE_SIZE, \
		too_big##m)

struct heappage {
  struct heappage *next;
};

ASSERT_PAGE_SIZE(heappage);

struct grant {
  #define GRANT_EMPTY  1
  #define GRANT_PART   2
  #define GRANT_READY  3
  int state;
  struct proc *waiting;
  
  int from;
  int flags;
  size_t npages, maxnpages;
  reg_t *pages;
};

struct mbox {
  size_t head, rtail, wtail;
  struct proc *swaiting, *rwaiting;
  size_t len;
  struct message *messages;
};

typedef enum {
  PROC_dead,
  PROC_suspend,
  PROC_recv,
  PROC_send,
  PROC_ready,
  PROC_oncpu,
} procstate_t;

struct proc {
  struct proc *snext; /* For scheduler */
  struct proc *wnext; /* For wait list */

  struct label label;

  procstate_t state;
  int pid;

  reg_t kstack;

  struct heappage *heap;
  struct space *space;
  struct mbox mbox;
  struct grant grant;
};

ASSERT_PAGE_SIZE(proc);


struct proc *
procnew(reg_t page,
	reg_t kstack,
	reg_t mbox,
	reg_t grant,
	struct heappage *heap,
	struct space *space);

void
procexit(struct proc *p);

void
procready(struct proc *p);

void
procsuspend(struct proc *p);

void
procrecv(void);

void
procsend(void);

struct proc *
findproc(int pid);

int
procwlistadd(struct proc **pp, struct proc *p);

struct proc *
procwlistpop(struct proc **pp);

 /* Must all be called with interrupts disabled */
void
schedule(void);

int
setlabel(struct label *);

int
gotolabel(struct label *) __attribute__((noreturn));

void
nilfunc(void);

void
droptouser(struct label *u, reg_t ksp)
  __attribute__((noreturn));

void
forkfunc(struct proc *p, int (*func)(void *), void *arg);

reg_t
forkchild(struct proc *p);

void *
heappop(void);

void
heapadd(void *start);

void
mboxinit(struct mbox *mbox, reg_t start);

void
mboxclose(struct mbox *m);

int
ksendnb(int pid, struct message *m);

int
ksend(int pid, struct message *m);

int
krecvnb(struct message *m);

int
krecv(struct message *m);

struct space *
spacenew(reg_t start);

void
spacefree(struct space *s);

int
validaddr(void *addr, size_t len, int flags);

int
mappingadd(struct space *s,
	   reg_t va,
	   reg_t pa,
	   int flags);

int
mappingremove(struct space *s,
	      reg_t va);

reg_t
mappingfind(struct space *s,
	    reg_t va,
	    int *flags);

int
checkflags(int need, int got);

void
grantinit(struct grant *g, reg_t start);

void
grantfree(struct grant *g);

int
granttake(struct grant *g);

int
granttakenb(struct grant *g);

int
grantuntake(struct grant *g);

int
grantready(struct grant *g);

void
mmuswitch(struct space *s);

void *
memmove(void *dest, const void *src, size_t len);

void *
memset(void *dest, int c, size_t len);

void
puts(const char *);

intr_t
setintr(intr_t i);

uint32_t
tickstoms(uint32_t);

uint32_t
mstoticks(uint32_t);

void
setsystick(uint32_t ticks);

extern struct proc *up;
