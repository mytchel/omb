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
sysexit(void);

reg_t
sysfork(int flags);

reg_t
sysgetpid(void);

reg_t
syssendnb(int to, struct message *m);

reg_t
syssend(int to, struct message *m);

reg_t
sysrecvnb(struct message *m);

reg_t
sysrecv(struct message *m);

reg_t
sysmgrantnb(int to, int flags, void *start, size_t len);

reg_t
sysmgrant(int to, int flags, void *start, size_t len);

reg_t
sysmmap(int flags, void *start, size_t len);

void *systab[NSYSCALLS] = {
  [SYSCALL_EXIT]              = (void *) &sysexit,
  [SYSCALL_FORK]              = (void *) &sysfork,
  [SYSCALL_GETPID]            = (void *) &sysgetpid,
  [SYSCALL_SENDNB]            = (void *) &syssendnb,
  [SYSCALL_SEND]              = (void *) &syssend,
  [SYSCALL_RECVNB]            = (void *) &sysrecvnb,
  [SYSCALL_RECV]              = (void *) &sysrecv,
  [SYSCALL_MGRANTNB]          = (void *) &sysmgrant,
  [SYSCALL_MGRANT]            = (void *) &sysmgrantnb,
  [SYSCALL_MMAP]              = (void *) &sysmmap,
};
