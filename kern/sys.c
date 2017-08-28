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
	debug("%i ksend to %i\n", up->pid, p->pid);
	
	if (p->state == PROC_dead) {
		debug("%i is dead, erroring\n", p->pid);
		return ERR;
	}
	
	up->waiting_on = p;
	
	if (!proc_list_add(&p->waiting, up)) {
		return ERR;
	}
	
	up->state = PROC_send;
	if (p->state == PROC_recv) {
		schedule(p);
	} else {
		schedule(nil);
	}
	
	debug("%i got reply %i\n", up->pid, up->page->m_ret);
	return up->page->m_ret;
}

proc_t
krecv(int pid)
{
	proc_t w;
	
	debug("%i krecv\n", up->pid);
	
	while (true) {
		w = proc_list_remove(&up->waiting, pid);
	
		if (w != nil) {
			memcpy(up->page->m_in, 
			       w->page->m_out,
			       MESSAGE_LEN);
			
			w->state = PROC_reply;
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
	debug("%i kreply to %i\n", up->pid, p->pid);
	
	if (p->state != PROC_reply || p->waiting_on != up) {
		return ERR;
	}
	
	memcpy(p->page->m_in,
	       up->page->m_out,
	       MESSAGE_LEN);
	
	p->page->m_ret = ret;
	
	schedule(p);
	
	return OK;
}


proc_t
kreply_recv(proc_t p,
            int ret,
            int pid)
{
	debug("%i kreply_recv to %i\n", up->pid, p->pid);
	
	if (p->state != PROC_reply || p->waiting_on != up) {
		return nil;
	}
	
	memcpy(p->page->m_in,
	       up->page->m_out,
	       MESSAGE_LEN);
	
	p->page->m_ret = ret;
	
	up->state = PROC_recv;
	schedule(p);
	
	return krecv(pid);
}

reg_t
sys_get_proc_page(void)
{
	return (reg_t) up->page;
}

reg_t
sys_get_kernel_page(void)
{
	return (reg_t) kernel_page;
}

reg_t
sys_send(int pid)
{
	proc_t p;
	
	debug("%i send to %i\n", up->pid, pid);
	
	p = find_proc(pid);
	if (p == nil) {
		debug("didnt find %i\n", pid);
		return ERR;
	}

	return ksend(p);
}

reg_t
sys_recv(int pid)
{
	proc_t p;
	
	debug("%i recv from %i\n", up->pid, pid);
	
	p = krecv(pid);
	if (p == nil) {
		return ERR;
	} else {
		debug("%i recved from %i\n", up->pid, p->pid);
		return p->pid;
	}
}

reg_t
sys_reply(int pid,
          int ret)
{
	proc_t p;
	
	debug("%i reply to %i with %i\n", up->pid, pid, ret);
	
	p = find_proc(pid);
	if (p == nil) {
		return ERR;
	}
	
	return kreply(p, ret);
}

reg_t
sys_reply_recv(int pid,
               int ret,
               int rpid)
{
	proc_t p;
	
	debug("%i reply to %i with %i, now recv from %i\n", up->pid, pid, ret, rpid);
	
	p = find_proc(pid);
	if (p == nil) {
		return ERR;
	}
	
	p = kreply_recv(p, ret, rpid);
	if (p == nil) {
		return ERR;
	} else {
		return p->pid;
	}
}

reg_t
sys_proc_create(proc_page_t page)
{
	proc_t p;
	
	p = proc_new((void *) page);
	if (p == nil) {
		return ERR;
	}
	
	return p->pid;
}

void *systab[NSYSCALLS] = {
	[SYSCALL_GET_PROC_PAGE]    = (void *) &sys_get_proc_page,
	[SYSCALL_GET_KERNEL_PAGE]  = (void *) &sys_get_kernel_page,
	[SYSCALL_SEND]             = (void *) &sys_send,
	[SYSCALL_RECV]             = (void *) &sys_recv,
	[SYSCALL_REPLY]            = (void *) &sys_reply,
	[SYSCALL_REPLY_RECV]       = (void *) &sys_reply_recv,
	[SYSCALL_PROC_CREATE]      = (void *) &sys_proc_create,
};
	