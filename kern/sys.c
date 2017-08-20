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

int
ksend(proc_t p)
{
	proc_t *pp;
	
	if (p->state == PROC_dead) {
		return ERR;
	}
	
	up->waiting_on = p;
	
	do {
		for (pp = &p->waiting; 
		     *pp != nil;
		     pp = &((*pp)->wnext))
			;
		
	} while (!cas(pp, nil, up));	
	
	up->state = PROC_send;
	if (p->state == PROC_recv) {
		schedule(p);
	} else {
		schedule(nil);
	}
	
	return p->page->ret;
}

proc_t
krecv(void)
{
	proc_t w;
	
	while (true) {
		w = up->waiting;
	
		if (w != nil) {
			if (!cas(&up->waiting, w, w->wnext)) {
				continue;
			}

			memcpy(up->page->message_in, 
			       w->page->message_out,
			       MESSAGE_LEN);
			
			w->state = PROC_reply;
			w->wnext = nil;
			return w;
			
		} else {
			up->state = PROC_recv;
			schedule(nil);
		}
	}
}

int
kreply(proc_t p,
       int ret)
{
	if (p->state != PROC_reply || p->waiting_on != up) {
		return ERR;
	}
	
	memcpy(p->page->message_in,
	       up->page->message_out,
	       MESSAGE_LEN);
	
	p->page->ret = ret;
	
	schedule(p);
	
	return OK;
}

reg_t
sys_get_proc_page(void)
{
	return (reg_t) up->page_user;
}

reg_t
sys_send(int pid)
{
	proc_t p;
	
	p = find_proc(pid);
	if (p == nil) {
		return ERR;
	}

	return ksend(p);
}

reg_t
sys_recv(void)
{
	proc_t p;
	
	p = krecv();
	if (p == nil) {
		return ERR;
	} else {
		return p->pid;
	}
}

reg_t
sys_reply(int pid,
         int ret)
{
	proc_t p;
	
	p = find_proc(pid);
	if (p == nil) {
		return ERR;
	}
	
	return kreply(p, ret);
}

void *systab[NSYSCALLS] = {
	[SYSCALL_GET_PROC_PAGE]  = (void *) &sys_get_proc_page,
	[SYSCALL_SEND]           = (void *) &sys_send,
	[SYSCALL_RECV]           = (void *) &sys_recv,
	[SYSCALL_REPLY]          = (void *) &sys_reply,
};
	