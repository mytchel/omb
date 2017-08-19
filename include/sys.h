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
	MESSAGE_memory,
	MESSAGE_proc,
} message_t;

typedef struct memory_req *memory_req_t;
typedef struct memory_resp *memory_resp_t;

struct memory_req {
	message_t type; /* = MESSAGE_memory */

#define MEMORY_REQ_from_ram    0
#define MEMORY_REQ_from_io     1
#define MEMORY_REQ_from_local  2
	
	int from_type;
	void *from_addr;
	
	
#define MEMORY_REQ_to_local    0
#define MEMORY_REQ_to_other    1

	int to_type;
	int other;
	void *va; /* nil for random free place. */
	
	
	size_t len;
};

ASSERT_MESSAGE_SIZE(memory_req);

struct memory_resp {
	message_t type; /* = MESSAGE_memory */
	
	void *va; /* Where it is. nil for failure. */
};

ASSERT_MESSAGE_SIZE(memory_resp);
              
              
typedef struct proc_req *proc_req_t;
typedef struct proc_resp *proc_resp_t;

struct proc_req {
	message_t type; /* = MESSAGE_proc */
	
};

ASSERT_MESSAGE_SIZE(proc_req);
              
              
struct proc_resp {
	message_t type; /* = MESSAGE_proc */
	
};

ASSERT_MESSAGE_SIZE(proc_resp);

#endif
