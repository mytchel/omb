/*
 *
 * Copyright (c) 2017 Mytchel Hammond <mytch@lackname.org>
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

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#define SYSCALL_SEND              0
#define SYSCALL_RECV              1
#define SYSCALL_REPLY             2
#define SYSCALL_REPLY_RECV        3

#define SYSCALL_GET_PROC_PAGE     4
#define SYSCALL_GET_KERNEL_PAGE   5

#define SYSCALL_PROC_CREATE       6

#define SYSCALL_ADDR_OFFER        7
#define SYSCALL_ADDR_ACCEPT       8
#define SYSCALL_ADDR_REJECT       9

#define NSYSCALLS                10

#endif
