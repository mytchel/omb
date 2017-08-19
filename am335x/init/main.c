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
#include <sys.h>
#include <string.h>
#include <uart.h>

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

int
main(void)
{
	proc_page_t page;
	addr_resp_t resp;
	addr_req_t req;
	uart_regs_t uart;
		
	page = get_proc_page();	
	
	req = (addr_req_t) page->message_out;
	resp = (addr_resp_t) page->message_in;
	
	req->type = MESSAGE_addr;
	req->from_type = ADDR_REQ_from_io;
	req->from_addr = (void *) UART0;
	req->to_type = ADDR_REQ_to_local;
	req->to_addr = (void *) 0xf000;
	req->len = sizeof(struct uart_regs);
	
	if (send(0) != OK) {
		while (true)
			;
	}
	
	uart = (uart_regs_t) resp->va;
	
	puts(uart, "Hello from userspace!\n");
	
	while (true)
		;
}

