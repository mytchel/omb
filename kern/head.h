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
typedef struct page *page_t;
typedef struct page_list *page_list_t;
typedef struct section *section_t;

struct proc_list {
	proc_t p;
	proc_list_t next;
};

struct page {
	int section_id;
	reg_t pa, va;
};

struct page_list {
	page_list_t next;
	size_t len;
	page_t pages[];
};

struct section {
	int id;
	int flags;
	int creator_pid;
	proc_list_t granted;
};

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
	void *page_user;
	
	space_t space;
	
	proc_t waiting_on;
	proc_list_t waiting;
	
	uint8_t kstack[KSTACK_LEN];
	
	page_list_t page_list;
};

proc_t
proc_new(space_t space, 
         void *sys_page);

proc_t
find_proc(int pid);

void
schedule(proc_t next);

int
ksend(proc_t p);

proc_t
krecv(void);

int
kreply(proc_t p,
       int ret);

proc_t
kreply_recv(proc_t p,
            int ret);
            
section_t
section_find(int id);

int
section_new(int creator, int flags);

void
section_free(int id);

/* Machine dependant. */

int
debug(const char *fmt, ...);

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

space_t
space_new(reg_t page);

bool
mapping_add(space_t s, reg_t pa, reg_t va,
            bool write, bool cache);

reg_t
mapping_remove(space_t s, reg_t va);

void *
kernel_addr(space_t s, reg_t addr, size_t len);

void
mmu_switch(space_t s);

/* Variables. */

extern proc_t up;

extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

