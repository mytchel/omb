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

#ifndef _FNS_H
#define _FNS_H

#define AP_NO_NO	0
#define AP_RW_NO	1
#define AP_RW_RO	2
#define AP_RW_RW	3

uint32_t
fsr_status(void);

reg_t
fault_addr(void);

void
intc_add_handler(uint32_t irq,
                 void (*func)(uint32_t));

void
intc_reset(void);

void
mmu_invalidate(void);

void
mmu_enable(void);

void
mmu_disable(void);

void
imap(void *, void *, int, bool);

void
mmu_load_ttb(uint32_t *);

reg_t
get_io_page(reg_t addr);

reg_t
get_ram_page(void);

reg_t
get_space_page(reg_t addr, space_t s);

void
add_addr(reg_t start, reg_t end);

void
give_proc0_world(proc_t p);

/* Initialisation functions */

void
init_intc(void);

void
init_memory(void);

void
init_watchdog(void);

void
init_timers(void);

proc_t
init_proc0(void);

#endif

