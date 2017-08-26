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
	return (reg_t) up->page_user;
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

bool
check_flags(int need, int want)
{
	if ((want & ADDR_write) && (want & ADDR_exec)) {
		return false;
	} else if ((want & ADDR_write) && !(need & ADDR_write)) {
		return false;
	} else {
		return true;
	}
}

reg_t
sys_section_create(reg_t start,
                   size_t len,
                   int flags)
{
	int s_id, f;
	size_t off;
	reg_t pa;
	
	s_id = section_new(up->pid, len, flags);
	if (s_id < 0) {
		return ERR;
	}
	
	for (off = 0; off < len; off += PAGE_SIZE) {
		pa = mapping_find(up->space, start + off, &f);
		if (pa == nil || !check_flags(f, flags)) {
			/* Do something. */
			return ERR;
		}
		
		if (!page_list_add(up->page_list, s_id, off,
		                   pa, start + off)) {
			/* Do something. */
			return ERR;
		}
	}
	
	return s_id;
}

reg_t
sys_section_grant(int pid, int s_id, bool unmap)
{
	section_t s;
	page_t page;
	proc_t dest;
	size_t off;
	
	s = section_find(s_id);
	if (s == nil) {
		return ERR;
	}
	
	dest = find_proc(pid);
	if (dest == nil) {
		return ERR;
	}
	
	for (off = 0; off < s->len; off += PAGE_SIZE) {
		page = page_list_find(up->page_list, s_id, off);
		if (page == nil) {
			/* Do something. */
			return ERR;
		}
		
		if (!page_list_add(dest->page_list, s_id, off,
		                   page->pa, nil)) {
			/* Do something. */
			return ERR;
		}
	}
		
	if (!proc_list_add(&s->granted, dest)) {
		return ERR;
	}
	
	for (off = 0; unmap && off < s->len; off += PAGE_SIZE) {
		page = page_list_find(up->page_list, s_id, off);
		if (page == nil) {
			/* Do something. */
			return ERR;
		} else if (page->va != nil) {	
			mapping_remove(up->space, page->va);
			page->va = nil;
		}
	}
	
	return OK;
}

reg_t
sys_section_map(int s_id, reg_t start, size_t off,
                size_t len, int flags)
{
	section_t s;
	page_t page;
	size_t o;
	
	/* TODO: check if space is safe to map. */
	
	s = section_find(s_id);
	if (s == nil || !check_flags(s->flags, flags)) {
		return ERR;
	}
	
	for (o = 0; o < len; o += PAGE_SIZE) {
		page = page_list_find(up->page_list, s_id, off + o);
		if (page == nil) {
			/* Do something. */
			return ERR;
		}
		
		if (!mapping_add(up->space, page->pa, start + o, flags)) {
			/* Do something. */
			return ERR;
		}
	}
	
	return OK;
}

int
proc_section_revoke(section_t s, proc_t p)
{
	section_t sn;
	reg_t va, pa;
	page_t page;
	size_t off;
	proc_t pn;
	
	for (off = 0; off < s->len; off += PAGE_SIZE) {
		if (!page_list_remove(p->page_list,
		                      s->id, off, &va, &pa)) {
			/* Do something. */
			return ERR;
		}
			
		if (va != nil) {
			mapping_remove(p->space, va);
		}
			
		while ((page = page_list_find_pa(p->page_list, pa)) != nil) {
			sn = section_find(page->section_id);
			if (sn == nil) {
				return ERR;
			}
			
			while ((pn = proc_list_remove(&sn->granted, PID_ALL)) != nil) {
				proc_section_revoke(sn, pn);
			}
			
			proc_section_revoke(sn, p);
		}
	}
	
	return OK;
}

reg_t
sys_section_revoke(int s_id, int pid)
{
	section_t s;
	proc_t p;
	
	s = section_find(s_id);
	if (s == nil) {
		return ERR;
	}
	
	while ((p = proc_list_remove(&s->granted, pid)) != nil) {
		proc_section_revoke(s, p);
	}
	
	return OK;
}

void *systab[NSYSCALLS] = {
	[SYSCALL_GET_PROC_PAGE]  = (void *) &sys_get_proc_page,
	[SYSCALL_SEND]           = (void *) &sys_send,
	[SYSCALL_RECV]           = (void *) &sys_recv,
	[SYSCALL_REPLY]          = (void *) &sys_reply,
	[SYSCALL_REPLY_RECV]     = (void *) &sys_reply_recv,
	
	[SYSCALL_SECTION_CREATE] = (void *) &sys_section_create,
	[SYSCALL_SECTION_GRANT]  = (void *) &sys_section_grant,
	[SYSCALL_SECTION_MAP]    = (void *) &sys_section_map,
	[SYSCALL_SECTION_REVOKE] = (void *) &sys_section_revoke,
};
	