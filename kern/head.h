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

#include <libc.h>
#include <syscalls.h>
#include <mem.h>
#include <types.h>
#include <stdarg.h>
#include <fs.h>
#include <fssrv.h>
#include <string.h>

#ifdef _am335x_
#include "../am335x/head.h"
#else
#error Need to set arch
#endif

struct lock {
  struct proc *holder;
  struct proc *wlist;
};
  
struct page {
  int refs;
  /* Not changable */
  reg_t pa;
  bool forceshare;
  struct page *next;
  struct page **from;
};

struct pagel {
  reg_t va;
  bool rw, c;
  struct page *p;
  struct pagel *next;
};

struct mgroup {
  int refs;
  struct lock lock;
  struct pagel *pages;
};

typedef enum { CHAN_pipe, CHAN_file, CHAN_max } chan_t;

struct chan {
  int refs;
  struct lock lock;
	
  chan_t type;
  int mode;
	
  void *aux;
};

struct chantype {
  int (*read)(struct chan *, void *, size_t);
  int (*write)(struct chan *, void *, size_t);
  int (*seek)(struct chan *, size_t, int);
  void (*close)(struct chan *);
};

struct path {
  char s[NAMEMAX];
  struct path *prev, *next;
};

struct fgroup {
  int refs;
  struct lock lock;
  struct chan **chans;
  size_t nchans;
};

struct bindingfid {
  int refs;
  
  uint32_t fid, attr;
  char name[NAMEMAX];

  struct binding *binding;
  struct bindingfid *parent;
  struct bindingfid *children;
  struct bindingfid *cnext;
};

struct fstransaction {
  size_t len;
  uint32_t rid, type;
  struct response *resp;
  struct fstransaction *next;
  struct proc *proc;
};

struct binding {
  int refs;
  struct lock lock;
	
  struct chan *in, *out;

  int nreqid;
  struct fstransaction *waiting; 
  struct proc *srv; /* Kernel proc that handles responses */

  struct bindingfid *fids;
};

struct bindingl {
  struct binding *binding;

  struct bindingfid *boundfid;
  struct bindingfid *rootfid;

  struct bindingl *next;
};

struct ngroup {
  int refs;
  struct lock lock;
  struct bindingl *bindings;
};

typedef enum {
  PROC_oncpu,
  PROC_suspend,
  PROC_ready,
  PROC_sleeping,
  PROC_waiting,
  PROC_dead,
} procstate_t;

struct proc {
  struct proc *next; /* For list of procs in list */

  int exitcode;

  int nchildren;
  struct proc *deadchildren;
  
  struct label label;

  struct mmu *mmu;
  
  procstate_t state;
  uint32_t pid;
  struct proc *parent;

  struct bindingfid *root;
  struct path *dot;
  struct chan *dotchan;

  uint32_t quanta;

  struct page *kstack;
  struct pagel *ustack;

  struct mgroup *mgroup;
  struct fgroup *fgroup;
  struct ngroup *ngroup;

  uint32_t sleep; /* in ticks */
  void *aux;
};

/****** Initialisation ******/

void
schedulerinit(void);

void
rootfsinit(void);

void
heapinit(void *, size_t);


/****** General Functions ******/

void
lock(struct lock *);

void
unlock(struct lock *);

/* Procs */

struct proc *
procnew(void);

struct proc *
waitchild(void);

void
procexit(struct proc *, int code);

void
procfree(struct proc *);

void
procready(struct proc *);

void
procsuspend(struct proc *p);

void
procwait(struct proc *p);

void
procsleep(struct proc *p, uint32_t ms);

void
procyield(struct proc *p);

/* This must all be called with interrupts disabled */

void
schedule(void);

/* Pages and Mgroup */

void
pagefree(struct page *);

void
pagelfree(struct pagel *);

struct pagel *
pagelcopy(struct pagel *);

struct pagel *
wrappage(struct page *p, reg_t va, bool rw, bool c);

struct mgroup *
mgroupnew(void);

struct mgroup *
mgroupcopy(struct mgroup *old);

void
mgroupfree(struct mgroup *m);

/* Memory */

bool
fixfault(void *);

/* Finds the physical address of addr in p's address
 * space and checks if it exstends through to size. 
 * If size is 0, checks if it exstends through to a
 * 0 byte. If it failes any of these checks returns nil.
 */
void *
kaddr(struct proc *p, const void *addr, size_t size);

reg_t
insertpages(struct mgroup *m, struct pagel *pagel,
	    reg_t addr, size_t size, bool fix);

int
kexec(struct chan *f, int argc, char *argv[]);

/* Channels */

struct chan *
channew(int, int);

void
chanfree(struct chan *);

/* Paths */

struct path *
strtopath(struct path *prev, const char *str);

char *
pathtostr(struct path *, size_t *);

struct path *
pathcopy(struct path *);

void
pathfree(struct path *);

/* Fgroup */

struct fgroup *
fgroupnew(void);

void
fgroupfree(struct fgroup *);

struct fgroup *
fgroupcopy(struct fgroup *);

int
fgroupaddchan(struct fgroup *, struct chan *);

int
fgroupreplacechan(struct fgroup *, struct chan *, int fd);

struct chan *
fdtochan(struct fgroup *, int);

/* Ngroup */

struct ngroup *
ngroupnew(void);

void
ngroupfree(struct ngroup *);

struct ngroup *
ngroupcopy(struct ngroup *);

struct binding *
bindingnew(struct chan *wr, struct chan *rd, uint32_t rootattr);

void
bindingfree(struct binding *);

struct bindingl *
ngroupfindbindingl(struct ngroup *n, struct bindingfid *fid);

int
ngroupaddbinding(struct ngroup *n, struct binding *b,
		 struct bindingfid *boundfid, 
		 struct bindingfid *rootfid);

int
ngroupremovebinding(struct ngroup *n, struct bindingfid *fid);

/* IPC */

int
mountproc(void *);

bool
pipenew(struct chan **rd, struct chan **wr);

int
piperead(struct chan *c, void *buf, size_t len);

int
pipewrite(struct chan *c, void *buf, size_t len);

int
filestat(struct path *path, struct stat *stat);

struct chan *
fileopen(struct path *path,
	 uint32_t mode, uint32_t cattr,
	 int *err);

void
bindingfidfree(struct bindingfid *fid);

struct bindingfid *
findfile(struct path *path, int *err);

int
fileremove(struct path *);

int
fileread(struct chan *c, void *buf, size_t len);

int
filewrite(struct chan *c, void *buf, size_t len);

int
fileseek(struct chan *c, size_t offset, int whence);

int
kmountloop(struct chan *in, struct binding *b, struct fsmount *mount);

/* Debug */

void
printf(const char *, ...);

void
panic(const char *, ...);

/****** Machine Implimented ******/

/* Type and psr are ignored. */
void
droptouser(struct label *regs, void *kstack)__attribute__((noreturn));

void
dumpregs(struct label *);

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

int
nullprocfunc(void *);

void
forkfunc(struct proc *, int (*func)(void *), void *);

reg_t
forkchild(struct proc *);

void
readyexec(struct label *ureg, void *entry, int argc, char *argv[]);

void
mmuswitch(struct mmu *);

void
mmuputpage(struct pagel *);

struct mmu *
mmunew(void);

void
mmufree(struct mmu *mmu);

bool
procwaitintr(int);

struct page *
getrampage(void);

struct page *
getiopage(void *addr);

/* Returns old state of interrupts for resetting. */

intrstate_t
setintr(intrstate_t);

bool
cas(void *addr, void *old, void *new);


/****** Global Variables ******/

extern struct proc *up;

extern void *syscalltable[NSYSCALLS];

extern struct chantype *chantypes[CHAN_max];

extern struct binding *rootfsbinding;
