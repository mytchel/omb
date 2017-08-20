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
#include <string.h>
#include <uart.h>

extern uint32_t *_text_start;
extern uint32_t *_text_end;
extern uint32_t *_data_start;
extern uint32_t *_data_end;
extern uint32_t *_bss_start;
extern uint32_t *_bss_end;

void
putc(uart_regs_t uart, char c)
{
  if (c == '\n')
    putc(uart, '\r');
	
	while ((uart->lsr & (1 << 5)) == 0)
		;
	
	uart->hr = c;
}

void
puts(uart_regs_t uart, const char *c)
{
  while (*c)
    putc(uart, *c++);
}

void
main_uart(void)
{
	addr_resp_t aresp;
	addr_req_t areq;
	uart_regs_t uart;
	proc_page_t page;
	int pid;
	
	page = get_proc_page();
	areq = (addr_req_t) page->message_out;
	aresp = (addr_resp_t) page->message_in;
	
	areq->type = MESSAGE_addr;
	areq->from_type = ADDR_REQ_from_io;
	areq->from_addr = (void *) UART0;
	areq->to_type = ADDR_REQ_to_local;
	areq->to_addr = (void *) (((reg_t) &_bss_end + PAGE_SIZE) & PAGE_MASK);
	areq->len = sizeof(struct uart_regs);
	
	if (send(0) != OK) {
		while (true)
			;
	}
	
	uart = (uart_regs_t) aresp->va;
	
	puts(uart, "Hello from userspace!\n");
	
	while (true) {
		pid = recv();
		if (pid < 0) {
			continue;
		}
		
		puts(uart, (char *) page->message_in);
		
		reply(pid, OK);
	}
}

int
main(void)
{
	proc_page_t page;
	addr_req_t areq;
	proc_req_t preq;
	proc_resp_t presp;
	label_t *u;
	int pid;
		
	page = get_proc_page();	
	
	areq = (addr_req_t) page->message_out;
	preq = (proc_req_t) page->message_out;
	presp = (proc_resp_t) page->message_in;
	
	preq->type = MESSAGE_proc;
	preq->page_addr = (void *) 0x20000000;
	
	if (send(0) != OK) {
		while (true)
			;
	}
	
	pid = presp->pid;
		
	areq->type = MESSAGE_addr;
	areq->from_type = ADDR_REQ_from_ram;
	areq->to_type = ADDR_REQ_to_local;
	areq->to_addr = (void *) (((reg_t) &_bss_end + PAGE_SIZE) & PAGE_MASK);
	areq->len = (size_t) &_text_end - (size_t) &_text_start;
	
	if (send(0) != OK) {
		while (true)
			;
	}
		
	memcpy((void *) (((reg_t) &_bss_end + PAGE_SIZE) & PAGE_MASK),
	       &_text_start,
	       (size_t) &_text_end - (size_t) &_text_start);
	
	areq->type = MESSAGE_addr;
	areq->from_type = ADDR_REQ_from_local;
	areq->from_addr = (void *) (((reg_t) &_bss_end + PAGE_SIZE) & PAGE_MASK);
	areq->to_type = ADDR_REQ_to_other;
	areq->to = pid;
	areq->to_addr = &_text_start;
	areq->len = (size_t) &_text_end - (size_t) &_text_start;
	
	if (send(0) != OK) {
		while (true)
			;
	}
			
	areq->type = MESSAGE_addr;
	areq->from_type = ADDR_REQ_from_ram;
	areq->to_type = ADDR_REQ_to_local;
	areq->to_addr = (void *) (((reg_t) &_bss_end + PAGE_SIZE) & PAGE_MASK);
	areq->len = (size_t) &_data_end - (size_t) &_data_start;
	
	if (send(0) != OK) {
		while (true)
			;
	}
	
	memcpy((void *) (((reg_t) &_bss_end + PAGE_SIZE) & PAGE_MASK),
	       &_data_start,
	       (size_t) &_data_end - (size_t) &_data_start);
	
	areq->type = MESSAGE_addr;
	areq->from_type = ADDR_REQ_from_local;
	areq->from_addr = (void *) (((reg_t) &_bss_end + PAGE_SIZE) & PAGE_MASK);
	areq->to_type = ADDR_REQ_to_other;
	areq->to = pid;
	areq->to_addr = &_data_start;
	areq->len = (size_t) &_data_end - (size_t) &_data_start;
	
	if (send(0) != OK) {
		while (true)
			;
	}
	
		
	areq->type = MESSAGE_addr;
	areq->from_type = ADDR_REQ_from_ram;
	areq->to_type = ADDR_REQ_to_other;
	areq->to = pid;
	areq->to_addr = &_bss_start;
	areq->len = (size_t) &_bss_end - (size_t) &_bss_start;
	
	if (send(0) != OK) {
		while (true)
			;
	}
	
	areq->type = MESSAGE_addr;
	areq->from_type = ADDR_REQ_from_ram;
	areq->to_type = ADDR_REQ_to_other;
	areq->to = pid;
	areq->to_addr = (void *) (0x20000000 - 0x1000);
	areq->len = 0x1000;
	
	if (send(0) != OK) {
		while (true)
			;
	}
	
	u = (label_t *) page->message_out;
	u->pc = (reg_t) &main_uart;
	u->sp = 0x20000000;
	
	if (send(pid) != OK) {
		while (true)
			;
	}
	
	snprintf((char *) page->message_out, MESSAGE_LEN, "Hello my new proc %i\n", pid);
	send(pid);
	
	while (true)
		;
}

