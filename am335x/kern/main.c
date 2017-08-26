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

#include "head.h"
#include "fns.h"

extern void *_proc0_text_start;
extern void *_proc0_text_end;
extern void *_proc0_data_start;
extern void *_proc0_data_end;
extern void *_proc0_bss_start;
extern void *_proc0_bss_end;

extern void *_proc1_text_start;
extern void *_proc1_text_end;
extern void *_proc1_data_start;
extern void *_proc1_data_end;
extern void *_proc1_bss_start;
extern void *_proc1_bss_end;

#define USER_start 0x1000
#define USER_stack 0x20000000

static void
proc_start(void)
{
	label_t u;
	
	u.sp = USER_stack;
	u.pc = USER_start;
	
	drop_to_user(&u, up->kstack, KSTACK_LEN);
}

static proc_t
init_proc(void *text, size_t tlen,
          void *data, size_t dlen,
          void *bss, size_t blen)
{
	reg_t space_page, page, stack, va, pa, o;
	space_t space;
	proc_t p;
	
	space_page = (reg_t) get_ram_page();
	
	space = space_new(space_page);
	
	va = USER_start;
	
	pa = (reg_t) text;
	for (o = 0;  o < tlen;  o += PAGE_SIZE, va += PAGE_SIZE) {
		if (!mapping_add(space, pa + o, va, false, true)) {
			return nil;
		}
	}
	
	pa = (reg_t) data;
	for (o = 0; o < dlen; o += PAGE_SIZE, va += PAGE_SIZE) {
		if (!mapping_add(space, pa + o, va, true, true)) {
			return nil;
		}
	}
	
	for (o = 0; o < blen; o += PAGE_SIZE, va += PAGE_SIZE) {
		pa = (reg_t) get_ram_page();
		memset((void *) pa, 0, PAGE_SIZE);
		if (!mapping_add(space, pa, va, true, true)) {
			return nil;
		}
	}
	
	stack = (reg_t) get_ram_page();
	if (!mapping_add(space, stack, USER_stack - PAGE_SIZE, true, false)) {
		return nil;
	}
	
	page = (reg_t) get_ram_page();
	if (!mapping_add(space, page, USER_stack, true, false)) {
		return nil;
	}
	
	p = proc_new(space, (void *) page);
	
	p->page_user = (void *) USER_stack;
	
	func_label(&p->label, p->kstack, KSTACK_LEN, &proc_start);
	
	return p;
}

int
kmain(void)
{
	proc_t p0;
	
  debug("OMB Booting...\n");

	init_intc();
	init_watchdog();
	init_timers();
	init_memory();
	
	p0 = init_proc(&_proc0_text_start,
	              (size_t) &_proc0_text_end - (size_t) &_proc0_text_start,
	              &_proc0_data_start,
	              (size_t) &_proc0_data_end - (size_t) &_proc0_data_start,
	              &_proc0_bss_start,
	              (size_t) &_proc0_bss_end - (size_t) &_proc0_bss_start);
	
	give_proc0_world(p0);
	
	init_proc(&_proc1_text_start,
	              (size_t) &_proc1_text_end - (size_t) &_proc1_text_start,
	               &_proc1_data_start,
	              (size_t) &_proc1_data_end - (size_t) &_proc1_data_start,
	               &_proc1_bss_start,
	               (size_t) &_proc1_bss_end - (size_t) &_proc1_bss_start);
	
	schedule(p0);
  
  /* Never reached */
  return 0;
}

