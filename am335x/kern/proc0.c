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
	
	memset(&u, 0, sizeof(label_t));
	u.psr = MODE_USR;
	u.sp = USER_stack;
	u.pc = USER_start;
	
	drop_to_user(&u, &up->kstack[KSTACK_LEN]);
}

void
init_proc1(void)
{
	reg_t space_page, page, va, pa, o;
	space_t space;
	proc_t p;
	
	space_page = (reg_t) get_ram_page();
	page = (reg_t) get_ram_page();
	
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
	
	if (!mapping_add(space, page, USER_stack)) {
		debug("Failed to map 0x%h to 0x%h\n", page, USER_stack);
		while (true)
			;
	}
	
	p = proc_new(space, (void *) page);
	if (p == nil) {
		debug("Failed to create proc1!\n");
		while (true)
			;
	}
	
	p->page_user = (void *) USER_stack;
	func_label(&p->label, p->kstack, KSTACK_LEN, &proc1_main);
	
	p->state = PROC_ready;
}

static int
handle_addr_request(proc_t p, 
                    addr_req_t req,
                    addr_resp_t resp)
{
	reg_t (*get_page_f)(reg_t);
	reg_t pa, va;
	space_t s;
	size_t l;
	
	debug("have memory request from %i for len %i\n", p->pid, req->len);
	
	switch (req->from_type) {
	case ADDR_REQ_from_ram:
		get_page_f = (reg_t (*)(reg_t)) &get_ram_page;
		break;
			
	case ADDR_REQ_from_io:
		get_page_f = &get_io_page;
		break;
		
	case ADDR_REQ_from_local:
	default:
		return ERR;
	}

	switch (req->to_type) {
	case ADDR_REQ_to_local:
		s = p->space;
		va = (reg_t) req->to_addr;
		break;
	
	case ADDR_REQ_to_other:
	default:
		return ERR;
	}
	
	for (l = 0; l < req->len; l += PAGE_SIZE) {
		pa = get_page_f((reg_t) req->from_addr + l);
		if (pa == nil) {
			return ERR;
		}
		
		if (!mapping_add(s, pa, va + l)) {
			return ERR;
		}
	}
	
	resp->va = (void *) va;
	
	return OK;
}

void
proc0_main(void)
{
	addr_resp_t resp = (addr_resp_t) up->page->message_out;
	addr_req_t req = (addr_req_t) up->page->message_in;
	proc_t p;
	int ret;
	
	debug("in proc 0\n");
	
	init_proc1();
	
	while (true) {
		p = krecv();
		if (p == nil) {
			continue;
		}
		
		switch (req->type) {
		default:
			debug("bad request type %i from %i\n", req->type, p->pid);
			ret = ERR;
			break;
		
		case MESSAGE_addr:
			ret = handle_addr_request(p, req, resp);
			break;
		}
		
		if (kreply(p, ret) != OK) {
			debug("problem sending reply!\n");
		}
	}
}

proc_t
init_proc0(void)
{
	reg_t space_page, stack, page;
	space_t space;
	proc_t p;
	
	space_page = (reg_t) get_ram_page();
	page = (reg_t) get_ram_page();
	stack = (reg_t) get_ram_page();
	
	space = space_new(space_page);
	
	p = proc_new(space, (void *) page);
	if (p == nil) {
		debug("Failed to create proc0!\n");
		while (true)
			;
	}
	
	func_label(&p->label, (void *) stack, PAGE_SIZE, &proc0_main);
	
	p->state = PROC_ready;
	
	return p;
}
