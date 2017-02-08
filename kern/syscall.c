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

#include <head.h>

reg_t
sysexit(void)
{
  printf("%i called exit\n", up->pid);
  return ERR;
}

reg_t
sysfork(int flags, struct label *regs)
{
  printf("%i called fork\n", up->pid);
  return ERR;
}

reg_t
sysgetpid(void)
{
  printf("%i called getpid\n", up->pid);
  return up->pid;
}

reg_t
syssendnb(int *status)
{
  printf("%i called sendnb\n", up->pid);
  return ERR;
}

reg_t
sysrecvnb(int *status)
{
  printf("%i called recvnb\n", up->pid);
  return ERR;
}

void *systab[NSYSCALLS] = {
  [SYSCALL_EXIT]              = (void *) &sysexit,
  [SYSCALL_FORK]              = (void *) &sysfork,
  [SYSCALL_GETPID]            = (void *) &sysgetpid,
  [SYSCALL_SENDNB]            = (void *) &syssendnb,
  [SYSCALL_RECVNB]            = (void *) &sysrecvnb,
};
