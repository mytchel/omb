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
#include "fns.h"

static int
mainprocfunc(void *arg)
{
  uint8_t buf[] = "This is a test.";
  size_t len = 32;
  int i = 0;
  
  while (true) {
    if (ksend(0, buf, len) != OK) {
      continue;
    }

    if (i > 50) {
      len = (len + 64) % 4096;
      printf("bumping len up to %i\n", len);
      i = 0;
    } else {
      i++;
    }
  }

  return OK;
}

void
mainprocinit(void)
{
  struct page *info, *kstack, *mbox;
  struct proc *p;
  
  info = getrampage();
  kstack = getrampage();
  mbox = getrampage();

  p = procnew(info, kstack, mbox);

  forkfunc(p, &mainprocfunc, nil);
  procready(p);
}

