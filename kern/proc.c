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

void add_to_list_back(struct proc **, struct proc *);
bool remove_from_list(struct proc **, struct proc *);

#define MAX_PROCS 512

static uint32_t nextpid = 0;

static struct proc procs[MAX_PROCS] = { 0 };
proc_t alive = nil;
proc_t up = nil;

static struct proc *
next_proc(void)
{
	proc_t p;
	
	if (alive == nil) {
		return nil;
	} else if (up != nil) {
		p = up->next;
	} else {
		p = alive;
	}
	
	do {
		if (p == nil) {
			p = alive;
			
		} else if (p->state == PROC_ready) {
			return p;
			
		} else {
			p = p->next;
		}
	} while (p != up);
	
	return p != nil && p->state == PROC_ready ? p : nil;
}

void
schedule(proc_t n)
{
	if (up != nil) {
		if (up->state == PROC_oncpu) {
			up->state = PROC_ready;
		}
    
		if (set_label(&up->label)) {
			return;
		}
	}
	
	if (n != nil) {
		up = n;
	} else {
		up = next_proc();
	}
		
	set_systick(1000);

	if (up != nil) {
		up->state = PROC_oncpu;
		mmu_switch(up->space);
		goto_label(&up->label);
		
	} else {
		debug("idle\n");
		
		set_intr(INTR_on);
		while (true)
			;
	}
}

void
add_to_list_back(proc_t *l, proc_t p)
{
	proc_t pp;

	p->next = nil;
  
	while (true) {
		for (pp = *l; pp != nil && pp->next != nil; pp = pp->next)
			;

		if (pp == nil) {
			if (cas(l, nil, p)) {
				break;
			}
		} else if (cas(&pp->next, nil, p)) {
			break;
		}
	}
}

bool
remove_from_list(proc_t *l, proc_t p)
{
	proc_t pt;

	while (true) {
		if (*l == p) {
			if (cas(l, p, p->next)) {
				return true;
			}
		} else {
			for (pt = *l; pt != nil && pt->next != p; pt = pt->next)
				;

			if (pt == nil) {
				return false;
			} else if (cas(&pt->next, p, p->next)) {
				return true;
			}
		}
	}
}
	
static void
proc_start(void)
{
	label_t *u = (label_t *) up->page->message_in;
	proc_t p;
	
	p = krecv();
	if (p == nil) {
		/* Do something. */
	}
	
	if (kreply(p, OK) != OK) {
		/* Do something. */
	}
		
	drop_to_user(u, up->kstack, KSTACK_LEN);
}

proc_t
proc_new(space_t space, void *page)
{
  int pid, npid;
  proc_t p;
  
	do {
		pid = nextpid;
		npid = (pid + 1) % MAX_PROCS;
	} while (!cas(&nextpid, 
	              (void *) pid, 
	              (void *) npid));
	
  p = &procs[pid];
	memset(p, 0, sizeof(struct proc));
  
  p->pid = pid;
  p->space = space;

	p->page = page;
	
	memset(p->page, 0, PAGE_SIZE);
	p->page->pid = pid;
	
  p->next = nil;
  p->wnext = nil;

	func_label(&p->label, p->kstack, KSTACK_LEN, &proc_start);
	
	p->state = PROC_ready;
	
	add_to_list_back(&alive, p);
		
  return p;
}

proc_t
find_proc(int pid)
{
  proc_t p;
  
  for (p = alive; p != nil; p = p->next) {
  	if (p->pid == pid) {
  		return p;
  	}
  }
  
  return nil;
}
