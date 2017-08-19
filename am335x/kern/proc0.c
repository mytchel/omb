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

#include <head.h>
#include <sys.h>
#include "fns.h"

extern uint32_t *_init_start;
extern uint32_t *_init_end;

/* These should probably be defined elseware. */
#define USER_start 0x1000
#define USER_stack 0x20000000

void
proc1_main(void)
{
	label_t u;
	
	debug("proc1 starting\n");
	
	memset(&u, 0, sizeof(label_t));
	u.psr = MODE_USR;
	u.sp = USER_stack;
	u.pc = USER_start;
	
	drop_to_user(&u, &up->kstack[KSTACK_LEN]);
}

void
init_proc1(void)
{
	reg_t space_page, va, pa, o;
	space_t space;
	proc_t p;
	
	space_page = (reg_t) get_ram_page();
	
	space = space_new(space_page);
	
	va = USER_start;
	pa = (reg_t) &_init_start;
	
	for (o = 0; 
	     pa + o < (reg_t) &_init_end;
	     o += PAGE_SIZE) {
	  
		if (!mapping_add(space, pa + o, va + o)) {
			debug("Failed to map 0x%h to 0x%h\n", pa + o, va + o);
			while (true)
				;
		}
	}
	
	pa = (reg_t ) get_ram_page();
	if (pa == nil) {
		debug("Failed to get ram for init!\n");
		while (true)
			;
	}
	
	if (!mapping_add(space, pa, USER_stack - PAGE_SIZE)) {
		debug("Failed to map 0x%h to 0x%h\n", pa, USER_stack - PAGE_SIZE);
		while (true)
			;
	}
	
	p = proc_new(space);
	if (p == nil) {
		debug("Failed to create proc1!\n");
		while (true)
			;
	}
	
	func_label(&p->label, p->kstack, KSTACK_LEN, &proc1_main);
	
	p->state = PROC_ready;
}

static int
handle_memory_request(proc_t p, 
                      memory_req_t req,
                      memory_resp_t resp)
{
	debug("have memory request from %i\n", p->pid);
	
	return ERR;
}

void
proc0_main(void)
{
	char s[MESSAGE_LEN], r[MESSAGE_LEN];
	memory_resp_t resp = (memory_resp_t) r;
	memory_req_t req = (memory_req_t) s;
	proc_t p;
	int ret;
	
	init_proc1();
	
	while (true) {
		debug("proc0 waiting for message\n");
		
		p = krecv(s);
		if (p == nil) {
			continue;
		}
		
		switch (req->type) {
		default:
			ret = ERR;
			break;
		
		case MESSAGE_memory:
			ret = handle_memory_request(p, req, resp);
			break;
		}
		
		if (kreply(p, ret, r) != OK) {
			debug("problem sending reply!\n");
		}
	}
}

proc_t
init_proc0(void)
{
	reg_t space_page, stack;
	space_t space;
	proc_t p;
	
	space_page = (reg_t) get_ram_page();
	stack = (reg_t) get_ram_page();
	
	space = space_new(space_page);
	
	p = proc_new(space);
	if (p == nil) {
		debug("Failed to create proc0!\n");
		while (true)
			;
	}
	
	func_label(&p->label, (void *) stack, PAGE_SIZE, &proc0_main);
	
	p->state = PROC_ready;
	
	return p;
}
