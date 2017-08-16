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

static uint32_t nextpid = 0;

static proc_t procs = nil;
proc_t up = nil;

static struct proc *
next_proc(void)
{
	proc_t p;
	
	p = up->next;
	do {
		if (p == nil) {
			p = procs;
		} else if (p->state == PROC_ready) {
			return p;
		} else {
			p = p->next;
		}
	} while (p != up);
	
	return p->state == PROC_ready ? p : nil;
}

void
schedule(proc_t n)
{
	debug("schedule\n");
	
	if (up != nil) {
		debug("take %i off cpu\n", up->pid);
	
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
		debug("put %i on cpu\n", up->pid);
		
		up->state = PROC_oncpu;
		goto_label(&up->label);
	} else {
		debug("go idle\n");
		
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
	
proc_t
proc_new(reg_t page, 
        reg_t kstack)
{
  proc_t p;
	
  p = (proc_t) page;

  p->state = PROC_dead;
  p->kstack = kstack;

  p->next = nil;
  p->wnext = nil;
  p->waiting = nil;
  p->waiting_on = nil;

	do {
		p->pid = nextpid;
	} while (!cas(&nextpid, 
	              (void *) p->pid, 
	              (void *) (p->pid + 1)));

	add_to_list_back(&procs, p);

  return p;
}

proc_t
find_proc(int pid)
{
  proc_t p;
  
  for (p = procs; p != nil; p = p->next) {
  	if (p->pid == pid) {
  		return p;
  	}
  }
  
  return nil;
}
