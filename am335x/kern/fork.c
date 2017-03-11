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
#include "trap.h"

void
forkfunc(struct proc *p, int (*func)(void *), void *arg)
{
  memset(&p->label, 0, sizeof(struct label));

  p->label.psr = MODE_SVC;
  p->label.sp = (uint32_t) p->kstack + PAGE_SIZE;
  p->label.pc = (uint32_t) &funcloader;

  p->label.sp -= sizeof(uint32_t);
  memmove((void *) p->label.sp, &func, sizeof(uint32_t));
  p->label.sp -= sizeof(uint32_t);
  memmove((void *) p->label.sp, &arg, sizeof(uint32_t));
}

void
forkchild(struct proc *p, void *entry, void *ustacktop, void *arg)
{
  struct label *l;

  l = &p->label;
  
  memset(l, 0, sizeof(struct label));

  l->psr = (uint32_t) MODE_USR;
  l->sp = (uint32_t) ustacktop;
  l->regs[0] = (uint32_t) arg;
  l->pc = (uint32_t) entry;
}

