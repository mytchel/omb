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
#include <am335x/uart.h>

static uart_regs_t uart;

static void
putc(char c)
{
  if (c == '\n')
    putc('\r');
	
	while ((uart->lsr & (1 << 5)) == 0)
		;
	
	uart->hr = c;
}

static void
puts(const char *c)
{
  while (*c)
    putc(*c++);
}

/*
static char
getc(void)
{
	while (uart->lsr & 1)
		;
	
	return uart->hr & 0xff;
}
*/

void
main(void)
{
	addr_req_t areq;
	proc_page_t page;
	int pid;
	
	page = get_proc_page();
	
	areq = (addr_req_t) page->m_out;	
	areq->type = MESSAGE_addr;
	areq->addr = UART0;
	areq->len = PAGE_ALIGN(sizeof(struct uart_regs));
	
	if (addr_offer(0, (void *) 0xa0000, areq->len,
	               ADDR_read|ADDR_write|ADDR_take) != OK) {
		/* Fuck. */
		while (true)
			;
	}
	
	if (send(0) != OK) {
		/* Fuck. */
		while (true)
			;
	}
	
	uart = (uart_regs_t) 0xa0000;
	
	puts("Hello from userspace!\n");
	
	pid = recv(PID_ALL);
	while (true) {
		puts((char *) page->m_in);
		
		pid = reply_recv(pid, OK, PID_ALL);
	}
}
