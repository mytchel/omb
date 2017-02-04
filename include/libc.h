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

#ifndef _LIBC_H_
#define _LIBC_H_

#include <err.h>

int
exit(int code) __attribute__((noreturn));

/* Should process aspects be (c)opied, created a(n)ew, or (s)hared. */

#define FORK_smem	(0<<0)
#define FORK_cmem	(1<<0)

int
fork(unsigned int flags);

int
getpid(void);

/* Returns an error or OK, message and reply must be of length
 * MESSAGELEN or greater, they can be the same memory region. 
 */
int
send(int pid, void *message, size_t len);

/* Returns an error or OK, message must be of length MESSAGELEN,
 * mid is the id of the message recieved.
 */
void *
recv(size_t *len);

bool
cas(void *addr, void *old, void *new);

unsigned int
atomicinc(unsigned int *addr);

unsigned int
atomicdec(unsigned int *addr);

#define roundptr(x) (x % sizeof(void *) != 0 ?			 \
		     x + sizeof(void *) - (x % sizeof(void *)) : \
		     x)

#endif
