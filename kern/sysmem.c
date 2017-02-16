/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
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

#include <head.h>

reg_t
sysmgrant(void *start, size_t len, int flags)
{
  printf("%i grant from 0x%h, %i with flags %i\n", up->pid,
	 start, len, flags);
  
  return ERR;
}

reg_t
sysmmap(int from, int code, void *start, va_list ap)
{
  size_t len;
  int flags;

  len = va_arg(ap, size_t);
  flags = va_arg(ap, int);

  printf("%i trying to mmap from %i, with code %i, to 0x%h, %i, with flags %i\n",
	 up->pid, from, code, start, len, flags);

  return ERR;
}

reg_t
sysmunmap(void *start, size_t len)
{
  printf("%i unmap from 0x%h, %i\n", up->pid, start, len);
  return ERR;
}

