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

struct addr {
	reg_t start;
	size_t len;
};

struct addr_holder {
	struct addr_holder *next;
	size_t len, n;
	struct addr addrs[];
};

static struct addr_holder *ram;
static struct addr_holder *io;

reg_t
get_ram_page(void);

void
add_addr(struct addr_holder **a, reg_t start, reg_t end)
{
	struct addr_holder *h;

	if ((*a)->n == (*a)->len) {
		h = (struct addr_holder *) get_ram_page();
		if (h == nil) {
			return;
		}
		
		h->next = *a;
		*a = h;
			
		(*a)->n = 0;
		(*a)->len = (PAGE_SIZE - sizeof(struct addr_holder))
		                / sizeof(struct addr);
		                
		memset((*a)->addrs, 0, sizeof(struct addr) * (*a)->len);
	}
	
	(*a)->addrs[(*a)->n].start = start;
	(*a)->addrs[(*a)->n].len = end - start;
	(*a)->n++;
}

reg_t
get_ram_page(void)
{
	struct addr_holder *a;
	size_t i;
	
	for (a = ram; a != nil; a = a->next) {
		for (i = 0; i < a->n; i++) {
			if (a->addrs[i].len > 0) {
				a->addrs[i].len -= PAGE_SIZE;
				return (a->addrs[i].start + a->addrs[i].len);
			}
		}
	}
	
	return nil;
}

reg_t
get_io_page(reg_t addr)
{
	struct addr_holder *a;
	size_t i;
	
	for (a = io; a != nil; a = a->next) {
		for (i = 0; i < a->n; i++) {
			if (a->addrs[i].len > 0 &&
			    a->addrs[i].start <= addr &&
			    a->addrs[i].start + a->addrs[i].len > addr) {
			  
			  if (a->addrs[i].start + a->addrs[i].len > addr + PAGE_SIZE) {
			  	/* Add another spot for remainder of chunk. */
			  	add_addr(&io, addr + PAGE_SIZE, 
			  	         a->addrs[i].start + a->addrs[i].len);
			  }
			  
			  /* Shrink chunk. */
			  a->addrs[i].len = addr - a->addrs[i].start;
			  				
				return addr;
			}
		}
	}
	
	return nil;
}

static int
handle_addr_request(int pid,
                    addr_req_t req,
                    addr_resp_t resp)
{
	
	return ERR;
}

int
main(void)
{
	proc_page_t page;
	message_t *type;
	int pid, ret;
		
	page = get_proc_page();	
	
	type = (message_t *) page->message_in;
	
	ram = (struct addr_holder *) PROC0_RAM_START;
	io = (struct addr_holder *) get_ram_page();
	io->next = nil;
	io->n = 0;
	io->len = (PAGE_SIZE - sizeof(struct addr_holder))
	                / sizeof(struct addr);
		
	add_addr(&io, 0x47400000, 0x47404000); /* USB */
  add_addr(&io, 0x44E31000, 0x44E32000); /* DMTimer1 */
  add_addr(&io, 0x48042000, 0x48043000); /* DMTIMER3 */
  add_addr(&io, 0x44E09000, 0x44E0A000); /* UART0 */
  add_addr(&io, 0x48022000, 0x48023000); /* UART1 */
  add_addr(&io, 0x48024000, 0x48025000); /* UART2 */
  add_addr(&io, 0x44E07000, 0x44E08000); /* GPIO0 */
  add_addr(&io, 0x4804c000, 0x4804d000); /* GPIO1 */
  add_addr(&io, 0x481ac000, 0x481ad000); /* GPIO2 */
  add_addr(&io, 0x481AE000, 0x481AF000); /* GPIO3 */
  add_addr(&io, 0x48060000, 0x48061000); /* MMCHS0 */
  add_addr(&io, 0x481D8000, 0x481D9000); /* MMC1 */
  add_addr(&io, 0x47810000, 0x47820000); /* MMCHS2 */
  add_addr(&io, 0x44E35000, 0x44E36000); /* Watchdog */
  add_addr(&io, 0x44E05000, 0x44E06000); /* DMTimer0 */
	
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
		}
		
		pid = reply_recv(pid, ret);
	}
}

