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
#include <syscalls.h>
#include <stdarg.h>
#include <string.h>


/* Procs */


typedef enum {
  PROC_oncpu,
  PROC_suspend,
  PROC_ready,
  PROC_dead,
} procstate_t;

struct proc {
  struct proc *next; /* For global of procs */
  struct proc *snext; /* For scheduler */

  struct label label;

  procstate_t state;
  int pid;

  struct page *kstack;
  struct page *info; /* Page this struct is stored in */

  struct mbox *mbox;
};

struct proc *
procnew(struct page *info,
	struct page *kstack,
	struct page *mbox);

void
procexit(struct proc *p);

void
procready(struct proc *p);

void
procsuspend(struct proc *p);

struct proc *
findproc(int pid);

/* This must all be called with interrupts disabled */

void
schedule(void);


/* Pages */


struct page {
  unsigned int refs;
  reg_t pa;
};

struct mapping {
  struct page *to;
  reg_t va;
#define PAGEMODE_ro     0
#define PAGEMODE_rw     1
  int mode;
  struct mapping *next;
};

struct mappingtable {
  struct page *page;
  struct mappingtable *next;
  size_t len;
  struct mapping mappings[];
};

struct addrspace {
  int refs;
  struct page *page;
  struct mappingtable *mappings;
  size_t size;
  struct mapping *table[];
};



/* Messages */

struct mbox {
  struct page *page;
  size_t head, tail;
  size_t mlen, dlen;
  struct message **messages;
  uint8_t *data;
};

struct message {
  size_t rlen; /* Length of body taken from chunk */
  size_t len; /* Length of filled body */
  uint8_t body[];
};

struct mbox *
mboxnew(struct page *page);

void
kmessagefree(struct message *m);

int
ksend(int pid, void *message, size_t len);

struct message *
krecv(void);



/* Mem */


void *
memmove(void *dest, const void *src, size_t len);

void *
memset(void *dest, int c, size_t len);




/****** Machine Implimented ******/



void
puts(const char *);

/* Number of ticks since last call. */
uint32_t
ticks(void);

/* Clear ticks counter */
void
cticks(void);

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
forkfunc(struct proc *, int (*func)(void *), void *);

reg_t
forkchild(struct proc *);


/****** Global Variables ******/

extern struct proc *up;

