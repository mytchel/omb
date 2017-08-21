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

#include <c.h>
#include <mach.h>
#include <sys.h>

static int
handle_addr_request(int pid,
                    addr_req_t req,
                    addr_resp_t resp)
{
	return ERR;
#if 0
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
#endif
}

static int
handle_proc_request(int pid,
                    proc_req_t req,
                    proc_resp_t resp)
{
	return ERR;
#if 0
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
#endif
}

int
main(void)
{
	proc_page_t page;
	message_t *type;
	int pid, ret;
		
	page = get_proc_page();	
	
	type = (message_t *) page->message_in;
	
	pid = recv();
	
	while (true) {
		switch (*type) {
		default:
			ret = ERR;
			break;
		
		case MESSAGE_addr:
			ret = handle_addr_request(pid, 
			                          (addr_req_t) page->message_in, 
			                          (addr_resp_t) page->message_out);
			break;
		
		case MESSAGE_proc:
			ret = handle_proc_request(pid, 
			                          (proc_req_t) page->message_in, 
			                          (proc_resp_t) page->message_out);
			break;
		}
		
		pid = reply_recv(pid, ret);
	}
}

