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
#include "fns.h"

extern uint32_t *_init_start;
extern uint32_t *_init_end;

void
proc1_main(void)
{
	label_t u;
	
	memset(&u, 0, sizeof(label_t));
	u.psr = MODE_USR;
	u.sp = 0x20000000;
	u.pc = 0x1000;
	
	drop_to_user(&u, (void *) (up->kstack + PAGE_SIZE));
}

void
init_proc1(void)
{
	reg_t page, stack, space_page, va, pa, o;
	space_t space;
	proc_t p;
	
	debug("create proc 1\n");
	
	debug("init at 0x%h, to 0x%h\n", &_init_start, &_init_end);
	
	page = (reg_t) get_ram_page();
	stack = (reg_t) get_ram_page();
	space_page = (reg_t) get_ram_page();
	
	space = space_new(space_page);
	
	debug("map space\n");
	
	va = 0x1000;
	pa = (reg_t) &_init_start;
	
	for (o = 0; 
	     pa + o < (reg_t) &_init_end;
	     o += PAGE_SIZE) {
	     
		debug("map 0x%h to 0x%h\n", pa + o, va + o);
		
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
	
	if (!mapping_add(space, pa, 0x20000000 - PAGE_SIZE)) {
		debug("Failed to map 0x%h to 0x%h\n", pa, 0x20000000 - PAGE_SIZE);
		while (true)
			;
	}
	
	p = proc_new(page, stack, space);
	if (p == nil) {
		debug("Failed to create proc1!\n");
		while (true)
			;
	}
	
	proc_func(p, &proc1_main);
	
	p->state = PROC_ready;
}

void
proc0_main(void)
{
	char m[MESSAGE_LEN];
	proc_t p;
		
	debug("proc0 started!\n");
	init_proc1();
	
	debug("proc0 looping\n");
	while (true) {
		p = krecv(m);
		if (p == nil) {
			debug("umm\n");
			continue;
		}
		
		debug("got message '%s' from %i\n", m, p->pid);
		
		snprintf(m, sizeof(m), "Hello %i!", p->pid);
		
		if (kreply(p, m) != OK) {
			debug("problem sending reply!\n");
		}
	}
}

proc_t
init_proc0(void)
{
	reg_t page, stack, space_page;
	space_t space;
	proc_t p;
	
	page = (reg_t) get_ram_page();
	stack = (reg_t) get_ram_page();
	space_page = (reg_t) get_ram_page();
	
	space = space_new(space_page);
	
	p = proc_new(page, stack, space);
	if (p == nil) {
		debug("Failed to create proc0!\n");
		while (true)
			;
	}
	
	proc_func(p, &proc0_main);
	
	p->state = PROC_ready;
	
	return p;
}
