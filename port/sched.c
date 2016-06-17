#include "types.h"
#include "mem.h"
#include "../include/proc.h"
#include "../include/com.h"

void kmain(void *);

static void __idle__(void *arg)
{
	while (true);
}

static struct proc *current, procs[MAX_PROCS];

static bool adding;
static uint32_t next_pid;

void
scheduler_init(void)
{
	int i;
	
	kprintf("scheduler_init\n");

	adding = false;
	next_pid = 0;
	
	for (i = 0; i < MAX_PROCS; i++)
		procs[i].state = PROC_stopped;

	current = procs;
	procs->next = nil;	
	procs->state = PROC_running;
	proc_init_regs(procs, &__idle__, nil);
	
	proc_create(&kmain, nil);
}

struct proc_machine *
schedule(void)
{
	struct proc *p;
	
	if (adding) {
		return &(current->machine);
	}
	
	p = current;
	do {
		p = p->next;
		if (p == nil) {
			/* Go to start of list or break if empty. */
			if ((p = procs->next) == nil)
				break;
		}

		if (p->state == PROC_running) {
			break;
		}
	} while (p != current);

	/* No processes to run. */
	if (p == nil || p->state != PROC_running) {
		kprintf("nothing to run\n");
		current = procs;
	} else {
		current = p;
	}
	
	return &(current->machine);
}

static struct proc *
find_and_add_proc_space()
{
	int i;
	struct proc *p, *pp;
	
	p = nil;
	for (i = 0; i < MAX_PROCS; i++) {
		if (procs[i].state == PROC_stopped) {
			p = &procs[i];
			break;
		}
	}
	
	if (p == nil)
		return nil;
	
	p->state = PROC_running;
	p->next = nil;
	
	for (pp = procs; pp->next; pp = pp->next);
	pp->next = p;
	
	return p;
}

struct proc *
proc_create(void (*func)(void *), void *arg)
{
	struct proc *p;

	kprintf("proc_create\n");
	
	while (adding);
	adding = true;

	p = find_and_add_proc_space();
	if (p == nil) {
		kprintf("Max process count reached\n");
		adding = false;
		return nil;
	}
	
	kprintf("init regs\n");
	
	p->state = PROC_running;
	p->pid = next_pid++;
	proc_init_regs(p, func, arg);
	
	kprintf("added\n");
	
	adding = false;
		
	return p;
}

void
proc_remove(struct proc *p)
{
	struct proc *pp;
	
	for (pp = procs; pp->next != p; pp = pp->next);
	pp->next = p->next;
}
