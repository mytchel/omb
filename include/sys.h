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

#ifndef _SYS_H_
#define _SYS_H_

#define ASSERT_MESSAGE_SIZE(m) \
    STATIC_ASSERT(sizeof(struct m) <= MESSAGE_LEN, \
                  too_big##m)

typedef enum {
	MESSAGE_addr,
	MESSAGE_proc,
	MESSAGE_proc_init,
} message_t;

typedef struct addr_req *addr_req_t;
typedef struct addr_resp *addr_resp_t;

struct addr_req {
	message_t type; /* = MESSAGE_addr */

#define ADDR_REQ_from_ram    0
#define ADDR_REQ_from_io     1
#define ADDR_REQ_from_local  2
	
	int from_type;
	void *from_addr;
		
#define ADDR_REQ_to_local    0
#define ADDR_REQ_to_other    1

	int to_type;
	int to;
	void *to_addr;
	
#define ADDR_REQ_flag_read    (1<<0)
#define ADDR_REQ_flag_write   (1<<1)
#define ADDR_REQ_flag_exec    (1<<2)
#define ADDR_REQ_flag_cache   (1<<3)
	int flags;
	size_t len;
};

ASSERT_MESSAGE_SIZE(addr_req);

struct addr_resp {
	message_t type; /* = MESSAGE_addr */
	
	void *va; /* Where it is. nil for failure. */
};

ASSERT_MESSAGE_SIZE(addr_resp);


typedef struct proc_req *proc_req_t;
typedef struct proc_resp *proc_resp_t;

struct proc_req {
	message_t type; /* = MESSAGE_proc */
	void *page_addr;
};

ASSERT_MESSAGE_SIZE(proc_req);
              
              
struct proc_resp {
	message_t type; /* = MESSAGE_proc */
	
	int pid;
};

struct proc_init_req {
	message_t type; /* = MESSAGE_proc_init */
	reg_t pc;
	reg_t sp;
};

struct proc_init_resp {
	message_t type; /* = MESSAGE_proc_init */
};

ASSERT_MESSAGE_SIZE(proc_resp);

#endif
