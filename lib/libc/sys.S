#
#  Copyright (c) 2017 Mytchel Hammond <mytch@lackname.org>
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

.global start
.global get_proc_page
.global send
.global recv
.global reply
.global reply_recv
.global section_create
.global section_grant
.global section_map
.global section_revoke


start:
	bl main
	b .


get_proc_page:
	push {r12, lr}
	mov r12, #SYSCALL_GET_PROC_PAGE
	svc 0
	pop {r12, pc}

send:
	push {r12, lr}
	mov r12, #SYSCALL_SEND
	svc 0
	pop {r12, pc}
	
	
recv:
	push {r12, lr}
	mov r12, #SYSCALL_RECV
	svc 0
	pop {r12, pc}
	
	
reply:
	push {r12, lr}
	mov r12, #SYSCALL_REPLY
	svc 0
	pop {r12, pc}


reply_recv:
	push {r12, lr}
	mov r12, #SYSCALL_REPLY_RECV
	svc 0
	pop {r12, pc}


section_create:
	push {r12, lr}
	mov r12, #SYSCALL_SECTION_CREATE
	svc 0
	pop {r12, pc}

section_grant:
	push {r12, lr}
	mov r12, #SYSCALL_SECTION_GRANT
	svc 0
	pop {r12, pc}

section_map:
	push {r12, lr}
	mov r12, #SYSCALL_SECTION_MAP
	svc 0
	pop {r12, pc}

section_revoke:
	push {r12, lr}
	mov r12, #SYSCALL_SECTION_REVOKE
	svc 0
	pop {r12, pc}
