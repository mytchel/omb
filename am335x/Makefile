#
#  Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
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
#  included in all copies or substantial kernions of the Software.
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

.SUFFIXES:
.SUFFIXES: .c .S .h .o .a .elf .bin .list .umg

include config.mk

loadaddr=0x82000000

.PHONY: all
all: am335x.list am335x.umg
CLEAN += am335x.umg am335x.list

KSRC_A := \
        vectors.S               \
        asm.S


KSRC_C := \
        main.c                  \
        trap.c                  \
        mem.c                   \
        mmu.c                   \
        uart.c                  \
        fork.c                  \
        timer.c                 \
        memproc.c               \
        mainproc.c              \
        ../kern/mem.c           \
        ../kern/message.c       \
        ../kern/string.c        \
        ../kern/printf.c        \
        ../kern/proc.c

KOBJECTS := \
	$(KSRC_A:%.S=%.o) \
	$(KSRC_C:%.c=%.o)


CLEAN += $(KOBJECTS)


.c.o .S.o:
	$(CC) $(CFLAGS) -c $< -o $@


.o.a:
	$(AR) rcs $@ $<


.elf.bin:
	@$(OBJCOPY) -Obinary $< $@


.elf.list:
	@$(OBJDUMP) -S $< > $@


CLEAN += $(TARGET).elf $(TARGET).list
am335x.elf: kernel.ld $(KOBJECTS)
	$(LD) $(LDFLAGS) \
		-T kernel.ld -Ttext $(loadaddr) \
		-o $@ $(KOBJECTS) -lgcc


CLEAN += am335x.elf am335x.bin
am335x.umg: am335x.bin
	../tools/mkuboot/mkuboot -a arm \
		-e $(loadaddr) -l $(loadaddr) -o linux \
		am335x.bin am335x.umg


.PHONY: clean 
clean: 
	rm -f $(CLEAN)
