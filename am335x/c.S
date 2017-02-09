#
#  Copyright (c) 2017 Mytchel Hammond <mytchel@openmailbox.org>
#  
#  Permission is hereby granted, free of charge, to any person
#  obtaining a copy of this software and associated documentation
#  files (the "Software"), to deal in the Software without
#  restriction, including without limitation the rights to use,
#  copy, modify, merge, publish, distribute, sublicense, and/or
#  sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following
#  conditions:
#  
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#  OTHER DEALINGS IN THE SOFTWARE
#

#include <syscalls.h>

.section .text

.global _start

@ System calls 
.global exit
.global fork
.global getpid
.global sendnb
.global send
.global recvnb
.global recv

@ Useful assembly functions
.global cas

_start:
	bl main
	@ fall through to exit

exit:
	push {r12, lr}
	mov r12, #SYSCALL_EXIT
	svc 0
	pop {r12, pc}


fork:
	push {r12, lr}
	mov r12, #SYSCALL_FORK
	svc 0
	pop {r12, pc}

	
getpid:
	push {r12, lr}
	mov r12, #SYSCALL_GETPID
	svc 0
	mov pc, lr
	pop {r12, pc}


sendnb:
	push {r12, lr}
	mov r12, #SYSCALL_SENDNB
	svc 0
	pop {r12, pc}


send:
	push {r12, lr}
	mov r12, #SYSCALL_SEND
	svc 0
	pop {r12, pc}


recvnb:
	push {r12, lr}
	mov r12, #SYSCALL_RECVNB
	svc 0
	pop {r12, pc}


recv:
	push {r12, lr}
	mov r12, #SYSCALL_RECV
	svc 0
	pop {r12, pc}

		
@bool
@cas(void *addr, void *old, void *new);
cas:
	ldrex r3, [r0]
	cmp r3, r1
	movne r0, #0
	movne pc, lr
	
	strex r3, r2, [r0]
	cmp r3, #1
	moveq r0, #0
	movne r0, #1
	
	mov pc, lr

