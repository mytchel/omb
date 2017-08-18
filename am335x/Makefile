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

loadaddr=0x82000000
TARGET = am335x

CROSS = arm-none-eabi-

CC = $(CROSS)gcc
LD = $(CROSS)ld
AR = $(CROSS)ar
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump

CFLAGS = -std=c89 -O3 \
	-Wall -Werror \
	-mcpu=cortex-a8 \
	-nostdinc -ffreestanding \
	-Iinclude -I../include -Ikern/.

LDFLAGS = -nostdlib -nodefaultlibs -static \
	-L/usr/local/lib/gcc/arm-none-eabi/6.3.1/

HOSTCC = gcc
HOSTLD = ld
HOSTAR = ar
HOSTOBJCOPY = objcopy
HOSTOBJDUMP = objdump

HOSTCFLAGS = -std=c89 -O3 \
	-Wall -Werror

HOSTLDFLAGS = 


.PHONY: all
all: am335x.list am335x.umg

CLEAN += am335x.umg am335x.list init.list

ISRC_A := \
        ../lib/libc/sys.S

ISRC_C := \
				init/main.c               \
        ../lib/libc/util.c        \
        ../lib/libstring/conv.c   \
        ../lib/libstring/scanf.c  \
        ../lib/libstring/string.c
				

IOBJECTS := \
	$(ISRC_A:%.S=%.o) \
	$(ISRC_C:%.c=%.o)

CLEAN += $(IOBJECTS)


KSRC_A := \
        kern/vectors.S          \
        kern/asm.S


KSRC_C := \
        kern/main.c               \
        kern/trap.c               \
        kern/timer.c              \
        kern/mem.c                \
        kern/proc.c               \
        kern/proc0.c              \
        kern/uart.c               \
        ../kern/proc.c            \
        ../kern/sys.c             \
        ../lib/libstring/conv.c   \
        ../lib/libstring/scanf.c  \
        ../lib/libstring/string.c \
        ../lib/libc/util.c


KOBJECTS := \
	$(KSRC_A:%.S=%.o) \
	$(KSRC_C:%.c=%.o)


CLEAN += $(KOBJECTS)


.c.o .S.o:
	$(CC) $(CFLAGS) -c $< -o $@


.o.a:
	$(AR) rcs $@ $<


.elf.bin:
	$(OBJCOPY) -Obinary $< $@


.elf.list:
	$(OBJDUMP) -S $< > $@


init.elf: init/init.ld $(IOBJECTS)
	$(LD) $(LDFLAGS) \
		-T init/init.ld -Ttext 0x1000 \
		-o $@ $(IOBJECTS) \
		-lgcc
		

CLEAN += init.o init.bin init.elf
init.o: init.elf
	$(OBJCOPY) --rename-section .text=.init_text \
	           --rename-section .data=.init_data \
	           --rename-section .bss=.init_bss \
	           -x -X -S \
	            $> $@
	

CLEAN += $(TARGET).elf $(TARGET).list
am335x.elf: kern/kernel.ld init.o $(KOBJECTS)
	$(LD) $(LDFLAGS) \
		-T kern/kernel.ld -Ttext $(loadaddr) \
		-o $@ $(KOBJECTS) init.o \
		-lgcc


CLEAN += am335x.elf am335x.bin
am335x.umg: am335x.bin
	../tools/mkuboot/mkuboot -a arm \
		-e $(loadaddr) -l $(loadaddr) -o linux \
		am335x.bin am335x.umg


.PHONY: clean 
clean: 
	rm -f $(CLEAN)

