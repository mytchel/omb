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

#ifndef _C_H_
#define _C_H_

#include <err.h>
#include <types.h>

/* All messages must be MESSAGE_LEN bytes long. */
#define MESSAGE_LEN 64

typedef struct proc_page *proc_page_t;
typedef struct kernel_page *kernel_page_t;

struct proc_page {
	int pid;
	
	uint8_t m_in[MESSAGE_LEN], m_out[MESSAGE_LEN];
	int m_ret;
};

typedef enum {
	REGION_ram,
	REGION_io,
} region_type_t;

struct region {
	region_type_t type;
	reg_t start;
	size_t len;
};

struct kernel_page {
	size_t nregions;
	struct region regions[];
};

proc_page_t
get_proc_page(void);

kernel_page_t
get_kernel_page(void);

#define PID_ALL        -1

int
send(int pid);

int
recv(int pid);

int
reply(int pid,
      int ret);

int
reply_recv(int pid,
           int ret,
           int rpid);

bool
cas(void *addr, void *old, void *new);

#define roundptr(x) (x % sizeof(void *) != 0 ?			 \
		     x + sizeof(void *) - (x % sizeof(void *)) : \
		     x)

#define STATIC_ASSERT(COND, MSG) \
  typedef char static_assertion_##MSG[(COND)?1:-1]


void
memcpy(void *dst, const void *src, size_t len);

void
memset(void *dst, uint8_t v, size_t len);

#endif
