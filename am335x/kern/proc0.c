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
init_proc1(void)
{
	reg_t space_page, page, va, pa, o;
	space_t space;
	label_t *u;
	proc_t p;
	
	space_page = (reg_t) get_ram_page();
	page = (reg_t) get_ram_page();
	
	space = space_new(space_page);
	
	va = USER_start;
	pa = (reg_t) &_init_start;
	
	for (o = 0; 
	     pa + o < (reg_t) &_init_end;
	     o += PAGE_SIZE) {
	  
		if (!mapping_add(space, pa + o, va + o, true, true)) {
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
	
	if (!mapping_add(space, pa, USER_stack - PAGE_SIZE, true, true)) {
		debug("Failed to map 0x%h to 0x%h\n", pa, USER_stack - PAGE_SIZE);
		while (true)
			;
	}
	
	if (!mapping_add(space, page, USER_stack, true, false)) {
		debug("Failed to map 0x%h to 0x%h\n", page, USER_stack);
		while (true)
			;
	}
	
	p = proc_new(space, (void *) page);
	
	p->page_user = (void *) USER_stack;
	
	u = (label_t *) up->page->message_out;
	u->pc = USER_start;
	u->sp = USER_stack;
	
	if (ksend(p) != OK) {
		debug("Failed to send regs to new proc!\n");
		while (true)
			;
	}
}

static int
handle_addr_request(proc_t p, 
                    addr_req_t req,
                    addr_resp_t resp)
{
	reg_t (*get_page_f)(reg_t, space_t);
	space_t from, to;
	reg_t pa, va;
	proc_t other;
	bool w, c;
	size_t l;
	
	switch (req->from_type) {
	case ADDR_REQ_from_ram:
		get_page_f = (reg_t (*)(reg_t, space_t)) &get_ram_page;
		from = nil;
		break;
			
	case ADDR_REQ_from_io:
		get_page_f = (reg_t (*)(reg_t, space_t)) &get_io_page;
		from = nil;
		break;
		
	case ADDR_REQ_from_local:
		get_page_f = (reg_t (*)(reg_t, space_t)) &get_space_page;
		from = p->space;
		break;
		
	default:
		return ERR;
	}

	switch (req->to_type) {
	case ADDR_REQ_to_local:
		to = p->space;
		va = (reg_t) req->to_addr;
		break;
	
	case ADDR_REQ_to_other:
		other = find_proc(req->to);
		if (other == nil) {
			return ERR;
		}
		
		to = other->space;
		va = (reg_t) req->to_addr;
		break;
		
	default:
		return ERR;
	}
	
	if (req->flags & ADDR_REQ_flag_write) {
		if (req->flags & ADDR_REQ_flag_exec) {
			return ERR;
		} else {
			w = true;
		}
	} else {
		w = false;
	}
	
	if (req->flags & ADDR_REQ_flag_cache) {
		c = true;
	} else {
		c = false;
	}
	
	for (l = 0; l < req->len; l += PAGE_SIZE) {		
		pa = get_page_f((reg_t) req->from_addr + l, from);
		if (pa == nil) {
			return ERR;
		}
		
		if (!mapping_add(to, pa, va + l, w, c)) {
			return ERR;
		}
	}
	
	resp->va = (void *) va;
	
	return OK;
}

static int
handle_proc_request(proc_t p, 
                    proc_req_t req,
                    proc_resp_t resp)
{
	reg_t space_page, page;
	space_t space;
	proc_t n;
	
	space_page = (reg_t) get_ram_page();
	page = (reg_t) get_ram_page();
	
	space = space_new(space_page);
		
	if (!mapping_add(space, page, (reg_t) req->page_addr, true, false)) {
		/* TODO: free pages. */
		return ERR;
	}
	
	n = proc_new(space, (void *) page);
	
	n->page_user = req->page_addr;
	
	resp->pid = n->pid;
	
	return OK;
}

void
proc0_main(void)
{
	message_t *type = (message_t *) up->page->message_in;
	proc_t p;
	int ret;
	
	init_proc1();
	
	while (true) {
		p = krecv();
		if (p == nil) {
			continue;
		}
		
		switch (*type) {
		default:
			debug("bad request type %i from %i\n", *type, p->pid);
			ret = ERR;
			break;
		
		case MESSAGE_addr:
			ret = handle_addr_request(p, 
			                          (addr_req_t) up->page->message_in, 
			                          (addr_resp_t) up->page->message_out);
			break;
		
		case MESSAGE_proc:
			ret = handle_proc_request(p, 
			                          (proc_req_t) up->page->message_in, 
			                          (proc_resp_t) up->page->message_out);
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
		
	func_label(&p->label, (void *) stack, PAGE_SIZE, &proc0_main);
	
	p->state = PROC_ready;
	
	return p;
}
