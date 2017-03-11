/*
 *
 * Copyright (c) 2017 Mytchel Hammond <mytchel@openmailbox.org>
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

#include <c.h>
#include <com.h>
#include <mem.h>

int
getmem(void *addr, size_t len)
{
  struct memresp resp;
  struct memreq req;

  req.type = COM_MEMREQ;
  req.len = len;
  req.flags = MEM_r|MEM_w;
  req.pa = 0;

  if (send(1, (struct message *) &req) != OK) {
    exit();
  }

  while (true) {
    if (recv((struct message *) &resp) == OK) {
      if (resp.type == COM_MEMRESP) {
	break;
      }
    } else {
      return ERR;
    }
  }  

  return mmap(MEM_r|MEM_w, addr, len);
}

int
main(void)
{
  struct message m;
  int f;

  m.type = COM_TEST;
  for (f = 0; f < 10; f++) {
    if (send(1, &m) != OK) {
      return ERR;
    }

    recv(&m);
  }
  
  if (getmem((void *) 0x4000, 0x2000) != OK) {
    return ERR;
  }

  if (getmem((void *) 0x20000, 0x4000) != OK) {
    return ERR;
  }

  if (getmem((void *) 0x30000, 0x1000) != OK) {
    return ERR;
  }

  if (getmem((void *) 0x0ffff000, 0x1000) != OK) {
    return ERR;
  }
  
  f = fork((void *) 0x4000, (void *) 0x5000,
	   (void *) 0x20000, 0x4000,
	   (void *) 0x30000, 0x1000,
	   (void *) 0x1000, (void *) 0x1000,
	   (void *) 0x20000000, (void *) 1234);

  if (f < 0) {
    return ERR;
  } else {
    m.type = 4;
    while (send(f, &m) == OK)
      ;
  }
       
  return OK;
}

