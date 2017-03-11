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

#ifndef _C_H_
#define _C_H_

#include <err.h>
#include <types.h>

int
exit(void) __attribute__((noreturn));

/* proc      -> A page of ram for the process table entry.
 * kheap     -> Start of ram for the processes kernel heap.
 * hlen      -> Length of new kernel heap.
 * start     -> Start of the new processes virtual address space.
 *             This will be unmapped from the callers address space.
 * len       -> Length of address space given to new process.
 * offset    -> Where start should be mapped in the new address space.
 * entry     -> Start executing here.
 * ustacktop -> Initial user stack.
 * arg       -> Argument given to new process.
 */

int
fork(void *proc, void *kstack,
     void *kheap, size_t hlen,
     void *start, size_t len, void *offset,
     void *entry, void *ustacktop,
     void *arg);

int
getpid(void);

int
cas(void *addr, void *old, void *new);

#define roundptr(x) (x % sizeof(void *) != 0 ?			 \
		     x + sizeof(void *) - (x % sizeof(void *)) : \
		     x)

#define STATIC_ASSERT(COND, MSG) \
  typedef char static_assertion_##MSG[(COND)?1:-1]

#endif
