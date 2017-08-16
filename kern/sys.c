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
ksend(proc_t p,
      void *s, 
      void *r)
{
	proc_t *pp;
	intr_t i;
	
	if (p->state == PROC_dead) {
		return ERR;
	}
	
	up->smessage = s;
	up->rmessage = r;
	
	up->waiting_on = p;
	
	do {
		for (pp = &p->waiting; 
		     *pp != nil;
		     pp = &((*pp)->wnext))
			;
		
	} while (!cas(pp, nil, up));	
	
	i = set_intr(INTR_off);
	
	if (p->state == PROC_recv) {
		p->state = PROC_ready;
		schedule(p);
	} else {
		schedule(nil);
	}
	
	set_intr(i);
	
	return OK;
}

proc_t
krecv(void *m)
{
	intr_t i;
	proc_t w;
	
	while (true) {
		w = up->waiting;
	
		if (w != nil) {
			if (!cas(&up->waiting, w, w->wnext)) {
				continue;
			}

			memmove(m, w->smessage, MESSAGE_LEN);
			w->state = PROC_reply;
			w->wnext = nil;
			return w;
			
		} else {
			i = set_intr(INTR_off);
			up->state = PROC_recv;
			schedule(nil);
			set_intr(i);
		}
	}
}

int
kreply(proc_t p,
       void *m)
{
	intr_t i;
	
	if (p->state != PROC_reply || p->waiting_on != up) {
		return ERR;
	}
	
	memmove(p->rmessage, m, MESSAGE_LEN);
	
	i = set_intr(INTR_off);
	p->state = PROC_ready;
	schedule(nil);
	set_intr(i);
	
	return OK;
}
       