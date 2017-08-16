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
#include <string.h>

typedef struct proc *proc_t;

typedef enum {
	PROC_oncpu,
	PROC_ready,
	PROC_dead,
	PROC_send,
	PROC_reply,
	PROC_recv,
	PROC_recving,
} procstate_t;

struct proc {
	label_t label;
	
	procstate_t state;
	int pid;
	
	reg_t kstack;
	
	void *smessage, *rmessage;
	proc_t waiting;
	proc_t waiting_on;
	
	proc_t next;
	proc_t wnext;
};

proc_t
proc_new(reg_t page,
        reg_t kstack);

proc_t
find_proc(int pid);

void
schedule(proc_t next);

int
ksend(proc_t p,
      void *s, 
      void *r);

proc_t
krecv(void *m);

int
kreply(proc_t p,
       void *m);

int
debug(const char *fmt, ...);

void
memmove(void *dst, const void *src, size_t len);

/* Machine dependant. */

void
puts(const char *c);

void
set_systick(uint32_t ms);

intr_t
set_intr(intr_t i);

int
set_label(label_t *l);

void
proc_func(proc_t p, void (*func)(void));

int
goto_label(label_t *l) __attribute__((noreturn));

/* Variables. */

proc_t up;
