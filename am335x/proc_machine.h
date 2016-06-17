#ifndef __PROC_MACHINE
#define __PROC_MACHINE

#define STACK_SIZE 1024

struct proc_machine {
	uint32_t psr, sp, pc, lr;
	uint32_t regs[13];
};

#endif