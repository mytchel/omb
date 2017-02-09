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

#include <libc.h>
#include <message.h>
#include <syscalls.h>
#include <stdarg.h>
#include <string.h>


/* Pages */

struct pagel {
  reg_t pa;
  struct pagel *next;
};

#define PAGE_ro     0<<0
#define PAGE_rw     1<<0
 
/* Procs */


typedef enum {
  PROC_oncpu,
  PROC_suspend,
  PROC_ready,
  PROC_recv,
  PROC_dead,
} procstate_t;


/* Can not excede PAGE_SIZE */

struct proc {
  struct proc *next; /* For global list of procs */
  struct proc *snext; /* For scheduler */

  struct label label;

  procstate_t state;
  int pid;

  reg_t kstack;
  struct stack ustack;

  struct addrspace *addrspace;
  struct mbox *mbox;
};


/* Messages */

struct mbox {
  size_t head, rtail, wtail;
  size_t len;
  struct message messages[];
};


/* Procs */

struct proc *
procnew(reg_t page,
	reg_t kstack,
	struct mbox *mbox,
	struct addrspace *addrspace);

void
procexit(struct proc *p);

void
procready(struct proc *p);

void
procsuspend(struct proc *p);

void
procrecv(struct proc *p);

struct proc *
findproc(int pid);

/* This must all be called with interrupts disabled */

void
schedule(void);

/* Messages */

struct mbox *
mboxnew(reg_t page);

void
mboxfree(struct mbox *m);

int
ksendnb(int pid, struct message *m);

int
ksend(int pid, struct message *m);

int
krecvnb(struct message *m);

int
krecv(struct message *m);


/* Mem */

void *
memmove(void *dest, const void *src, size_t len);

void *
memset(void *dest, int c, size_t len);


/* Implimented by each arch */

void
puts(const char *);

intr_t
setintr(intr_t i);

reg_t
kgetpage(void);

uint32_t
tickstoms(uint32_t);

uint32_t
mstoticks(uint32_t);

void
setsystick(uint32_t ticks);

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
forkfunc(struct proc *, int (*func)(void *), void *);

reg_t
forkchild(struct proc *);

struct addrspace *
addrspacenew(reg_t page);

struct addrspace *
addrspacecopy(struct addrspace *o);

void
addrspacefree(struct addrspace *s);

void
stackinit(struct stack *s);

int
stackcopy(struct stack *n, struct stack *o);

void
stackfree(struct stack *s);

int
fixfault(reg_t addr);

void *
kaddr(void *addr, size_t len);

reg_t
mappingfind(struct proc *p,
	    reg_t va,
	    int *flags);

int
mappingadd(struct addrspace *s,
	   reg_t va,
	   reg_t pa,
	   int flags);

int
mappingremove(struct addrspace *s,
	      reg_t va);

void
mmuswitch(struct proc *p);


/****** Global Variables ******/

extern struct proc *up;
extern void *systab[NSYSCALLS];
