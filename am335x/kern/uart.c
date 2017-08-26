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

#include "head.h"
#include "fns.h"
#include <stdarg.h>
#include <uart.h>

static uart_regs_t uart = (uart_regs_t) UART0;

void
putc(char c)
{
  if (c == '\n')
    putc('\r');
	
	while ((uart->lsr & (1 << 5)) == 0)
		;
	
	uart->hr = c;
}

void
puts(const char *c)
{
  while (*c)
    putc(*c++);
}

int
debug(const char *fmt, ...)
{
	char str[128];
	va_list ap;
	size_t i;
	
	va_start(ap, fmt);
	i = vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);
	
	if (i > 0) {
		puts(str);
	}
	
	return i;
}

void
panic(const char *fmt, ...)
{
	char str[128];
	va_list ap;
	size_t i;
	
	va_start(ap, fmt);
	i = vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);
	
	if (i > 0) {
		puts(str);
	}
	
	while (true)
		;
}
