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

#include "procs.h"

typedef struct section *section_t;
typedef struct section_holder *section_holder_t;

struct section {
	reg_t start;
	size_t len;
};

struct section_holder {
	section_holder_t next;
	size_t nsections;
	struct section sections[];
};

static section_holder_t ram = nil, io = nil;

reg_t
get_ram(size_t len)
{
	section_holder_t h;
	section_t s;
	int i;
	
	for (h = ram; h != nil; h = h->next) {
		for (i = 0; i < h->nsections; i++) {
			s = &h->sections[i];
			if (s->len >= len) {
				s->len -= len;
				return s->start + s->len;
			}
		}
	}
	return nil;
}

void
addr_add(section_holder_t holder, reg_t start, size_t len)
{
	section_t s;
	int i;
	
	while (holder != nil) {
		for (i = 0; i < holder->nsections; i++) {
			s = &holder->sections[i];
			if (s->len == 0) {
				s->start = start;
				s->len = len;
				return;
			}
		}
		
		if (holder->next == nil) {
			holder->next = (section_holder_t) get_ram(PAGE_SIZE);
			if (holder->next == nil) {
				/* Error! */
				return;
			}
			
			holder->next->next = nil;
			holder->next->nsections = (PAGE_SIZE - sizeof(struct section_holder))
		  	  / sizeof(struct section);
		  	  
			memset(holder->next->sections, 0,
			       holder->next->nsections * sizeof(struct section));
		}
		
		holder = holder->next;
	}
}

reg_t
get_io(reg_t addr, size_t len)
{
	section_holder_t h;
	section_t s;
	int i;
	
	for (h = io; h != nil; h = h->next) {
		for (i = 0; i < h->nsections; i++) {
			s = &h->sections[i];
			if (s->start <= addr &&
			    addr + len <= s->start + s->len) {
			
				if (addr + len < s->start + s->len) {
					addr_add(io, addr + len, 
					         s->start + s->len - addr - len);
				}
				
				s->len = addr - s->start;
				
				return addr;
			}
		}
	}
	
	return nil;
}

static void
init_ram_sections(kernel_page_t kern_page)
{
	region_t r;
	int i;
		
	for (i = 0; i < kern_page->nregions; i++) {
		r = &kern_page->regions[i];
		if (r->start == nil) break;
		if (r->type != REGION_ram) continue;
		
		if (ram == nil) {
			ram = (section_holder_t) r->start;
			ram->next = nil;
			ram->nsections = (PAGE_SIZE - sizeof(struct section_holder))
		  	  / sizeof(struct section);
			memset(ram->sections, 0, ram->nsections * sizeof(struct section));
		
			addr_add(ram, r->start + PAGE_SIZE, r->len - PAGE_SIZE);
		
		} else {
			addr_add(ram, r->start, r->len);
		}
	}
}

static void
init_io_sections(kernel_page_t kern_page)
{
	region_t r;
	int i;
	
	io = (section_holder_t) get_ram(PAGE_SIZE);	
	io->next = nil;
	io->nsections = (PAGE_SIZE - sizeof(struct section_holder))
	    / sizeof(struct section);
	memset(io->sections, 0, io->nsections * sizeof(struct section));
	
	for (i = 0; i < kern_page->nregions; i++) {
		r = &kern_page->regions[i];
		if (r->start == nil) break;
		if (r->type != REGION_io) continue;
		
		addr_add(io, r->start, r->len);
	}
}

static void
init_sections(kernel_page_t kern_page)
{
	init_ram_sections(kern_page);
	init_io_sections(kern_page);
}

static void
init_procs(proc_page_t my_page)
{
	reg_t page, stack, space;
	proc_init_req_t req;
	int i, pid;
	
	req = (proc_init_req_t) my_page->m_out;
	req->type = MESSAGE_proc_init;
	
	i = 0;
	while (procs[i] != nil) {
		page = get_ram(PAGE_SIZE);
		space = get_ram(PAGE_SIZE);
		stack = get_ram(PAGE_SIZE);
		
		pid = proc_create((proc_page_t) page, (void *) space);
		
		req->pc = 0x82006000 + procs[i];
		req->sp = stack + PAGE_SIZE;
		
		send(pid);
		
		i++;
	}
}

static int
handle_addr_request(int pid,
                    addr_req_t req,
                    addr_resp_t resp)
{
	reg_t addr;
	size_t len;
	int flags;
	
	len = PAGE_ALIGN(req->len);
	if (len == 0) {
		return ERR;
	}

	flags = ADDR_read|ADDR_write|ADDR_give;
	
	if (req->addr != nil) {
		addr = get_io(req->addr, len);
	} else {
		addr = get_ram(len);
		flags |= ADDR_cache;
	}
	
	if (addr == nil) {
		return ERR;
	}
	
	return addr_accept(pid, (void *) addr, len, flags);
}

int
main(void)
{
	kernel_page_t kern_page;
	proc_page_t page;
	message_t *type;
	int pid, ret;
	
	page = get_proc_page();
	kern_page = get_kernel_page();
	
	init_sections(kern_page);

	init_procs(page);
	
	type = (message_t *) page->m_in;

	pid = recv(PID_ALL);
	
	while (true) {
		switch (*type) {
		default:
			ret = ERR;
			break;
		
		case MESSAGE_addr:
			ret = handle_addr_request(pid, 
			                          (addr_req_t) page->m_in, 
			                          (addr_resp_t) page->m_out);
			break;
		}
		
		pid = reply_recv(pid, ret, PID_ALL);
	}
}

