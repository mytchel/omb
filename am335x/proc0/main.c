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
}

int
main(void)
{
	proc_page_t page;
	message_t *type;
	int pid, ret;
		
	page = get_proc_page();	
	
	type = (message_t *) page->m_in;
	
#if 0
  give_proc_section(p, 0x47400000, 0x47404000, ADDR_read|ADDR_write); /* USB */
  give_proc_section(p, 0x44E31000, 0x44E32000, ADDR_read|ADDR_write); /* DMTimer1 */
  give_proc_section(p, 0x48042000, 0x48043000, ADDR_read|ADDR_write); /* DMTIMER3 */
  give_proc_section(p, 0x44E09000, 0x44E0A000, ADDR_read|ADDR_write); /* UART0 */
  give_proc_section(p, 0x48022000, 0x48023000, ADDR_read|ADDR_write); /* UART1 */
  give_proc_section(p, 0x48024000, 0x48025000, ADDR_read|ADDR_write); /* UART2 */
  give_proc_section(p, 0x44E07000, 0x44E08000, ADDR_read|ADDR_write); /* GPIO0 */
  give_proc_section(p, 0x4804c000, 0x4804d000, ADDR_read|ADDR_write); /* GPIO1 */
  give_proc_section(p, 0x481ac000, 0x481ad000, ADDR_read|ADDR_write); /* GPIO2 */
  give_proc_section(p, 0x481AE000, 0x481AF000, ADDR_read|ADDR_write); /* GPIO3 */
  give_proc_section(p, 0x48060000, 0x48061000, ADDR_read|ADDR_write); /* MMCHS0 */
  give_proc_section(p, 0x481D8000, 0x481D9000, ADDR_read|ADDR_write); /* MMC1 */
  give_proc_section(p, 0x47810000, 0x47820000, ADDR_read|ADDR_write); /* MMCHS2 */
  give_proc_section(p, 0x44E35000, 0x44E36000, ADDR_read|ADDR_write); /* Watchdog */
  give_proc_section(p, 0x44E05000, 0x44E06000, ADDR_read|ADDR_write); /* DMTimer0 */
#endif

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

