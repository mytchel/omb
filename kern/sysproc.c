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

reg_t
sysgetpid(void)
{
  printf("%i called getpid\n", up->pid);
  return up->pid;
}

reg_t
sysexit(void)
{
  intr_t i;
  
  printf("%i called exit\n", up->pid);

  i = setintr(INTR_off);
  procexit(up);
  schedule();
  setintr(i);
  
  return ERR;
}

reg_t
sysfork(void)
{
  return ERR;
  
  #if 0
  reg_t page, kstack, mbox;
  struct addrspace *addrspace;
  struct proc *p;
  int r;
  
  printf("%i called fork\n", up->pid);

  page = kgetpage();
  kstack = kgetpage();
  mbox = kgetpage();

  addrspace = nil;
  
  printf("sharing mem (for now)\n");

  addrspace = up->addrspace;
  do {
    r = addrspace->refs;
  } while (!cas(&addrspace->refs, (void *) r, (void *) (r + 1)));

  p = procnew(page, PAGE_SIZE,
	      kstack, PAGE_SIZE,
	      mbox, PAGE_SIZE,
	      addrspace);

  printf("%i fork to new proc pid %i\n", up->pid, p->pid);

  r = ustackcopy(&p->ustack, &up->ustack);
  if (r != OK) {
    procexit(p);
    return r;
  }
  
  r = forkchild(p);

  return r;
  #endif
}

