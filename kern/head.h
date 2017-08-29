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
#include <sys.h>
#include <syscalls.h>
#include <string.h>

typedef struct proc *proc_t;
typedef struct proc_list *proc_list_t;

typedef enum {
	PROC_dead,
	PROC_enter,
	PROC_ready,
	PROC_oncpu,
	PROC_send,
	PROC_reply,
	PROC_recv,
	PROC_recving,
} procstate_t;

#define KSTACK_LEN 512

struct proc {
	label_t label;
	
	procstate_t state;
	int pid;
	proc_t next;
	
	proc_page_t page;
	
	proc_t waiting_on;
	proc_list_t waiting;
	
	struct {
		reg_t start;
		size_t len;
		int flags;
		int to;
	} addr_offer;
	
	addr_space_t space;
	
	uint8_t kstack[KSTACK_LEN];
};

struct proc_list {
	proc_t proc;
	proc_list_t next;
};

proc_t
proc_new(void *sys_page);

proc_t
find_proc(int pid);

void
schedule(proc_t next);

bool
proc_list_add(proc_list_t *list, proc_t p);

proc_t
proc_list_remove(proc_list_t *list, int pid);

int
ksend(proc_t p);

proc_t
krecv(int pid);

int
kreply(proc_t p,
       int ret);

proc_t
kreply_recv(proc_t p,
            int ret,
            int pid);

/* Machine dependant. */

int
debug(const char *fmt, ...);

void
panic(const char *fmt, ...);

void
set_systick(uint32_t ms);

intr_t
set_intr(intr_t i);

int
set_label(label_t *l);

int
goto_label(label_t *l) __attribute__((noreturn));

void
drop_to_user(label_t *l, 
             void *kstack, 
             size_t stacklen)
__attribute__((noreturn));

void
func_label(label_t *l, 
           void *stack, 
           size_t stacklen,
           void (*func)(void));

bool
space_map(addr_space_t space, 
          reg_t pa, reg_t va,
          int flags,
          reg_t (*get_page)(void));

reg_t
space_find(addr_space_t space,
           reg_t va,
           int *flags);

void
space_unmap(addr_space_t space,
            reg_t va);
            
/* Variables. */

extern proc_t up;
extern kernel_page_t kernel_page;